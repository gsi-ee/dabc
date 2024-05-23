// $Id$

/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <ctime>

#include "dogma/api.h"
#include "dabc/Url.h"
#include "dabc/api.h"

int usage(const char* errstr = nullptr)
{
   if (errstr)
      printf("Error: %s\n\n", errstr);

   printf("Utility for printing DOGMA data. 15.03.2024. S.Linev\n");
   printf("   dogmaprint source [args]\n");
   printf("Following sources are supported:\n");
   printf("   file.bin                    - DABC binary file reading\n");
   printf("   dabcnode                    - DABC stream server\n");
   printf("   dabcnode:port               - DABC stream server with custom port\n");
   printf("Arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -maxage value           - maximal age time for events, if expired queue are cleaned (default off)\n");
   printf("   -num number             - number of events to print, 0 - all events (default 10)\n");
   printf("   -all                    - print all events (equivalent to -num 0)\n");
   printf("   -skip number            - number of events to skip before start printing\n");
   printf("   -raw                    - printout of raw data (default false)\n");
   printf("   -rate                   - display only events and data rate\n");

   return errstr ? 1 : 0;
}

bool printraw = false, printsub = false, showrate = false, reconnect = false, dostat = false, dominsz = false, domaxsz = false, autoid = false;
unsigned idrange = 0xff, onlytdc = 0, onlynew = 0, onlyraw = 0, hubmask = 0, fullid = 0, adcmask = 0, onlymonitor = 0;


void print_tu(dogma::DogmaTu *tu, const char *prefix = "")
{
   printf("%stu addr: %04x type: %02x trignum: %06x time: %08x local: %08x err: %02x frame: %02x paylod: %04x size: %u\n", prefix,
          (unsigned)tu->GetAddr(), (unsigned)tu->GetTrigType(), (unsigned)tu->GetTrigNumber(),
          (unsigned)tu->GetTrigTime(), (unsigned)tu->GetLocalTrigTime(),
          (unsigned)tu->GetErrorBits(), (unsigned)tu->GetFrameBits(), (unsigned)tu->GetPayloadLen(), (unsigned) tu->GetSize());

   if (printraw) {
      unsigned len = tu->GetPayloadLen();
      for (unsigned i = 0; i < len; ++i) {
         printf("   %08x", (unsigned) tu->GetPayload(i));
         if ((i == len - 1) || ((i % 8 == 0) && (i > 0)))
            printf("\n");
      }
   }
}

void print_evnt(dogma::DogmaEvent *evnt)
{
   printf("Event seqid: %lu\n", (long unsigned) evnt->GetSeqId());
   auto tu = evnt->FirstSubevent();

   while(tu) {
      print_tu(tu, "   ");
      tu = evnt->NextSubevent(tu);
   }
}

int main(int argc, char* argv[])
{
   if ((argc < 2) || !strcmp(argv[1], "-help") || !strcmp(argv[1], "?"))
      return usage();

   long number = 10, skip = 0, nagain = 0;
   double tmout = -1., maxage = -1.;

   int n = 1;
   while (++n < argc) {
      if ((strcmp(argv[n], "-num") == 0) && (n + 1 < argc)) {
         dabc::str_to_lint(argv[++n], &number);
      } else if (strcmp(argv[n], "-all") == 0) {
         number = 0;
      } else if (strcmp(argv[n], "-rate") == 0) {
         showrate = true;
         number = 0;
      } else if ((strcmp(argv[n], "-skip") == 0) && (n + 1 < argc)) {
         dabc::str_to_lint(argv[++n], &skip);
      } else if ((strcmp(argv[n], "-tmout") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &tmout);
      } else if ((strcmp(argv[n], "-maxage") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &maxage);
      } else if (strcmp(argv[n], "-raw") == 0) {
         printraw = true;
      } else if (strcmp(argv[n], "-sub") == 0) {
         printsub = true;
      } else if (strcmp(argv[n], "-stat") == 0) {
         dostat = true;
      } else
         return usage("Unknown option");
   }

   printf("Try to open %s\n", argv[1]);

   if (tmout < 0) tmout = 5.;

   bool isfile = false;
   std::string src = argv[1];

   if ((src.find(".dld") != std::string::npos) && (src.find("dld://") != 0)) {
      src = std::string("dld://") + src;
      isfile = true;
   } else if (src.find("dld://") == 0) {
      isfile = 0;
   } else if ((src.find(".bin") != std::string::npos) && (src.find("bin://") != 0)) {
      src = std::string("bin://") + src;
      isfile = true;
   } else if ((src.find("bin://") == 0) || (src.find(".bin") != std::string::npos)) {
      isfile = true;
   }

   if (!isfile) {

      dabc::Url url(src);

      if (url.IsValid()) {
         if (url.GetProtocol().empty())
            src = std::string("mbss://") + src;

         if (reconnect && !url.HasOption("reconnect")) {
           if (url.GetOptions().empty())
              src += "?reconnect";
           else
              src += "&reconnect";
         }
      }
   }

   dogma::DogmaTu *tu = nullptr;
   dogma::DogmaEvent *evnt = nullptr;

   long cnt = 0, cnt0 = 0, lastcnt = 0, printcnt = 0, mincnt = -1, maxcnt = -1;
   uint32_t mintrignr = 0, maxtrignr = 0;
   uint64_t lastsz = 0, currsz = 0, minsz = 10000000000, numminsz = 0, maxsz = 0, nummaxsz = 0;
   dabc::TimeStamp last, first, lastevtm;

   dogma::ReadoutHandle ref;

   dabc::InstallSignalHandlers();

   while (nagain-- >= 0) {

   ref = dogma::ReadoutHandle::Connect(src);

   if (ref.null()) return 1;

   cnt = cnt0 = lastcnt = printcnt = 0;
   lastsz = currsz = 0;
   last = first = lastevtm = dabc::Now();

   while (!dabc::CtrlCPressed()) {

      unsigned data_kind = ref.NextPortion(maxage > 0 ? maxage/2. : 1., maxage);

      tu = nullptr;
      evnt = nullptr;

      if (data_kind == 1)
         tu = ref.GetTu();
      else if (data_kind == 2)
         evnt = ref.GetEvent();

      cnt0++;

      // if (debug_delay > 0) dabc::Sleep(debug_delay);

      dabc::TimeStamp curr = dabc::Now();

      uint32_t sz = 0, trignr = 0;

      if (tu) {
         // ignore events which are not match with specified id
         if ((fullid != 0) && (tu->GetAddr() != fullid)) continue;
         sz = tu->GetSize();
         trignr = tu->GetTrigNumber();
      } else if (evnt) {
         sz = evnt->GetEventLen();
         trignr = evnt->GetTrigNumber();

      } else if (curr - lastevtm > tmout) {
         /*printf("TIMEOUT %ld\n", cnt0);*/
         break;
      }

      if (dominsz) {
         if (sz < minsz) {
            minsz = sz;
            mintrignr = trignr;
            mincnt = cnt;
            numminsz = 1;
         } else if (sz == minsz) {
            numminsz++;
         }
      }

      if (domaxsz) {
         if (sz > maxsz) {
            maxsz = sz;
            maxtrignr = trignr;
            maxcnt = cnt;
            nummaxsz = 1;
         } else if (sz == maxsz) {
            nummaxsz++;
         }
      }

      cnt++;
      currsz += sz;
      lastevtm = curr;

      if (showrate) {

         double tm = curr - last;

         if (tm >= 0.3) {
            printf("\rTm:%6.1fs  Ev:%8ld  Rate:%8.2f Ev/s  %6.2f MB/s", first.SpentTillNow(), cnt, (cnt-lastcnt)/tm, (currsz-lastsz)/tm/1024./1024.);
            fflush(stdout);
            last = curr;
            lastcnt = cnt;
            lastsz = currsz;
         }

         // when showing rate, only with statistic one need to analyze event
         if (!dostat) continue;
      }

      if (!tu && !evnt) continue;

      if (skip > 0) { skip--; continue; }

      printcnt++;

      if (tu)
         print_tu(tu);
      else if (evnt)
         print_evnt(evnt);

      if ((number > 0) && (printcnt >= number)) break;
   }

   if (showrate) {
      printf("\n");
      fflush(stdout);
   }

   ref.Disconnect();

   if (dostat) {
      printf("Statistic: %ld events analyzed\n", printcnt);
   }

   if (dominsz && mincnt >= 0)
      printf("Event #0x%08x (-skip %ld) has minimal size %lu cnt %lu\n", (unsigned) mintrignr, mincnt, (long unsigned) minsz, (long unsigned) numminsz);

   if (domaxsz && maxcnt >= 0)
      printf("Event #0x%08x (-skip %ld) has maximal size %lu cnt %lu\n", (unsigned) maxtrignr, maxcnt, (long unsigned) maxsz, (long unsigned) nummaxsz);

   if (dabc::CtrlCPressed()) break;

   } // ngain--

   return 0;
}

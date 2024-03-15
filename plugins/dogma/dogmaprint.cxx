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

   dogma::DogmaTu *evnt = nullptr;

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

      evnt = ref.NextTu(maxage > 0 ? maxage/2. : 1., maxage);

      cnt0++;

      // if (debug_delay > 0) dabc::Sleep(debug_delay);

      dabc::TimeStamp curr = dabc::Now();

      if (evnt) {

         // ignore events which are not match with specified id
         if ((fullid != 0) && (evnt->GetAddr() != fullid)) continue;

         if (dominsz) {
            if (evnt->GetSize() < minsz) {
               minsz = evnt->GetSize();
               mintrignr = evnt->GetTrigNumber();
               mincnt = cnt;
               numminsz = 1;
            } else if (evnt->GetSize() == minsz) {
               numminsz++;
            }
         }

         if (domaxsz) {
            if (evnt->GetSize() > maxsz) {
               maxsz = evnt->GetSize();
               maxtrignr = evnt->GetTrigNumber();
               maxcnt = cnt;
               nummaxsz = 1;
            } else if (evnt->GetSize() == maxsz) {
               nummaxsz++;
            }
         }

         cnt++;
         currsz += evnt->GetSize();
         lastevtm = curr;
      } else if (curr - lastevtm > tmout) {
         /*printf("TIMEOUT %ld\n", cnt0);*/
         break;
      }

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

      if (!evnt) continue;

      if (skip > 0) { skip--; continue; }

      printcnt++;

      printf("Event addr: %lu type: %lu trignum; %lu, time: %lu paylod: %lu\n",
            (long unsigned) evnt->GetAddr(), (long unsigned) evnt->GetTrigType(), (long unsigned) evnt->GetTrigNumber(), (long unsigned) evnt->GetTrigTime(), (long unsigned) evnt->GetPayloadLen());

      if (printraw) {
         unsigned len = evnt->GetPayloadLen() / 4;
         for (unsigned i = 0; i < len; ++i) {
            printf("   %08x", (unsigned) evnt->GetPayload(i));
            if ((i == len - 1) || ((i % 8 == 0) && (i > 0)))
               printf("\n");
         }
      }

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

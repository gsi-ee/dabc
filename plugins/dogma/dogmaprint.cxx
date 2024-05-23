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
   printf("   -stat                   - count statistics\n");
   printf("   -bw                     - disable colors\n");
   printf("   -tdc id                 - printout raw data as TDC subsubevent (default none)\n");
   printf("   -skipintdc nmsg         - skip in tdc first nmsgs (default 0)\n");
   printf("   -fulltime               - always print full time of timestamp (default prints relative to channel 0)\n");
   printf("   -ignorecalibr           - ignore calibration messages (default off)\n");
   printf("   -onlytdc tdcid          - printout raw data only of specified tdc subsubevent (default none)\n");
   printf("   -onlych chid            - print only specified TDC channel (default off)\n");
   printf("   -mhz value              - new design with arbitrary MHz, 12bit coarse, 9bit fine, min = 0x5, max = 0xc0\n");
   printf("   -fine-min value         - minimal fine counter value, used for liner time calibration (default 31)\n");
   printf("   -fine-max value         - maximal fine counter value, used for liner time calibration (default 491)\n");
   printf("   -fine-min4 value        - minimal fine counter value TDC v4, used for liner time calibration (default 28)\n");
   printf("   -fine-max4 value        - maximal fine counter value TDC v4, used for liner time calibration (default 350)\n");
   printf("   -tot boundary           - minimal allowed value for ToT (default 20 ns)\n");
   printf("   -stretcher value        - approximate stretcher length for falling edge (default 20 ns)\n");
   return errstr ? 1 : 0;
}

struct TuStat {
   int cnt{0}; // number of stats
   int64_t min_diff{0}; // minimal time diff
   int64_t max_diff{0}; // maximal time diff

   dogma::DogmaTu *tu{nullptr}; // current tu
};

bool printraw = false, printsub = false, showrate = false, reconnect = false, dostat = false, dominsz = false, domaxsz = false, autoid = false;
unsigned idrange = 0xff, onlynew = 0, onlyraw = 0, hubmask = 0, fullid = 0, adcmask = 0, onlymonitor = 0;

std::vector<unsigned> tdcs;


bool is_tdc(unsigned id)
{
   if (std::find(tdcs.begin(), tdcs.end(), id) != tdcs.end())
      return true;

   for (unsigned n = 0; n < tdcs.size(); n++)
      if (((id & idrange) <= (tdcs[n] & idrange)) && ((id & ~idrange) == (tdcs[n] & ~idrange)))
         return true;

   return false;
}


std::map<uint32_t, TuStat> tu_stats;
uint32_t ref_addr = 0;


#include "../hadaq/tdc_print_code.cxx"

void print_tu(dogma::DogmaTu *tu, const char *prefix = "")
{
   printf("%stu addr:%04x type:%02x trignum:%06x time:%08x local:%08x err:%02x frame:%02x paylod:%04x size:%u\n", prefix,
          (unsigned)tu->GetAddr(), (unsigned)tu->GetTrigType(), (unsigned)tu->GetTrigNumber(),
          (unsigned)tu->GetTrigTime(), (unsigned)tu->GetLocalTrigTime(),
          (unsigned)tu->GetErrorBits(), (unsigned)tu->GetFrameBits(), (unsigned)tu->GetPayloadLen(), (unsigned) tu->GetSize());

   unsigned len = tu->GetPayloadLen();

   if (printraw) {
      printf("%s", prefix);
      for (unsigned i = 0; i < len; ++i) {
         printf("  %08x", (unsigned) tu->GetPayload(i));
         if ((i == len - 1) || (i % 8 == 7))
            printf("\n%s", i < len-1 ? prefix : "");
      }
   } else if (is_tdc(tu->GetAddr())) {
      std::vector<uint32_t> data(len, 0);
      for (unsigned i = 0; i < len; ++i)
         data[i] = tu->GetPayload(i);
      unsigned errmask = 0;
      PrintTdcDataPlain(0, data, strlen(prefix) + 3, errmask);
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

void stat_evnt(dogma::DogmaEvent *evnt)
{
   if (!evnt)
      return;

   for (auto &pairs : tu_stats)
      pairs.second.tu = nullptr;

   auto tu = evnt->FirstSubevent();
   dogma::DogmaTu *ref = nullptr;
   while(tu) {
      auto &entry = tu_stats[tu->GetAddr()];
      entry.tu = tu;
      if (!ref_addr) ref_addr = tu->GetAddr();

      if (ref_addr == tu->GetAddr())
         ref = tu;

      tu = evnt->NextSubevent(tu);
   }

   for (auto &pairs : tu_stats) {

      auto &entry = pairs.second;
      if (!entry.tu) continue;

      entry.cnt++;

      if (!ref || (pairs.first == ref_addr))
         continue;

      int64_t diff = entry.tu->GetTrigTime();
      diff -= ref->GetTrigTime();

      if (entry.cnt == 1) {
         entry.min_diff = diff;
         entry.max_diff = diff;
      } else {
         if (entry.min_diff < diff)
            entry.min_diff = diff;
         if (entry.max_diff > diff)
            entry.max_diff = diff;
      }
   }
}

void print_stat()
{
   for (auto &pairs : tu_stats) {
      printf("  addr:%04x cnt:%d min_diff:%ld max_diff:%ld\n", (unsigned) pairs.first, pairs.second.cnt, (long) pairs.second.min_diff, (long) pairs.second.max_diff);
   }

}

int main(int argc, char* argv[])
{
   if ((argc < 2) || !strcmp(argv[1], "-help") || !strcmp(argv[1], "?"))
      return usage();

   long number = 10, skip = 0, nagain = 0;
   double tmout = -1., maxage = -1., mhz = 200.;
   unsigned tdcmask = 0;

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
      } else if (strcmp(argv[n], "-bw") == 0) {
         use_colors = false;
      } else if (strcmp(argv[n], "-ignorecalibr") == 0) {
         use_calibr = false;
      } else if (strcmp(argv[n], "-fulltime") == 0) {
         print_fulltime = true;
      } else if ((strcmp(argv[n], "-tdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &tdcmask);
         tdcs.emplace_back(tdcmask);
      } else if ((strcmp(argv[n], "-onlytdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &onlytdc);
      } else if ((strcmp(argv[n], "-onlych") == 0) && (n + 1 < argc)) {
         dabc::str_to_int(argv[++n], &onlych);
      } else if ((strcmp(argv[n], "-skipintdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &skip_msgs_in_tdc);
      } else if ((strcmp(argv[n], "-mhz") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &mhz);
         use_400mhz = true;
         coarse_tmlen = 1000. / mhz;
         fine_min = 0x5;
         fine_max = 0xc0;
      } else if (strcmp(argv[n], "-340") == 0) {
         use_400mhz = true;
         coarse_tmlen = 1000. / 340.;
         fine_min = 0x5;
         fine_max = 0xc0;
      } else if (strcmp(argv[n], "-400") == 0) {
         use_400mhz = true;
         coarse_tmlen = 1000. / 400.;
         fine_min = 0x5;
         fine_max = 0xc0;
      } else if ((strcmp(argv[n], "-tot") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &tot_limit);
      } else if ((strcmp(argv[n], "-stretcher") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &tot_shift);
      } else
         return usage("Unknown option");
   }

   printf("Try to open %s\n", argv[1]);

   bool isfile = false;
   std::string src = argv[1];

   if ((src.find(".dld") != std::string::npos) && (src.find("dld://") != 0)) {
      src = std::string("dld://") + src;
      isfile = true;
   } else if (src.find("dld://") == 0) {
      isfile = false;
   } else if ((src.find(".bin") != std::string::npos) && (src.find("bin://") != 0)) {
      src = std::string("bin://") + src;
      isfile = true;
   } else if ((src.find("bin://") == 0) || (src.find(".bin") != std::string::npos)) {
      isfile = true;
   }

   if (tmout < 0) tmout = isfile ? 0.1 : 5.;

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

      if (tu || evnt) {
         cnt++;
         currsz += sz;
         lastevtm = curr;
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

      if (!tu && !evnt) continue;

      if (skip > 0) { skip--; continue; }

      printcnt++;

      if (dostat) {
         stat_evnt(evnt);
      } else if (tu)
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
      print_stat();
   }

   if (dominsz && mincnt >= 0)
      printf("Event #0x%08x (-skip %ld) has minimal size %lu cnt %lu\n", (unsigned) mintrignr, mincnt, (long unsigned) minsz, (long unsigned) numminsz);

   if (domaxsz && maxcnt >= 0)
      printf("Event #0x%08x (-skip %ld) has maximal size %lu cnt %lu\n", (unsigned) maxtrignr, maxcnt, (long unsigned) maxsz, (long unsigned) nummaxsz);

   if (dabc::CtrlCPressed()) break;

   } // ngain--

   return 0;
}

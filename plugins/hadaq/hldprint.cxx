// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <ctime>

#include "hadaq/api.h"
#include "dabc/Url.h"
#include "dabc/api.h"

#include "./tdc_print_code.cxx"


int usage(const char *errstr = nullptr)
{
   if (errstr)
      printf("Error: %s\n\n", errstr);

   printf("Utility for printing HLD events. 15.10.2024. S.Linev\n");
   printf("   hldprint source [args]\n");
   printf("Following sources are supported:\n");
   printf("   hld://path/file.hld         - HLD file reading\n");
   printf("   file.hld                    - HLD file reading (file extension MUST be '.hld')\n");
   printf("   file.hll                    - list of HLD files (file extension MUST be '.hll')\n");
   printf("   dabcnode                    - DABC stream server\n");
   printf("   dabcnode:port               - DABC stream server with custom port\n");
   printf("   mbss://dabcnode/Transport   - DABC transport server\n");
   printf("   lmd://path/file.lmd         - LMD file reading\n");
   printf("Arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -maxage value           - maximal age time for events, if expired queue are cleaned (default off)\n");
   printf("   -buf sz                 - buffer size in MB, default 4\n");
   printf("   -num number             - number of events to print, 0 - all events (default 10)\n");
   printf("   -all                    - print all events (equivalent to -num 0)\n");
   printf("   -skip number            - number of events to skip before start printing\n");
   printf("   -event id               - search for given event id before start printing\n");
   printf("   -find id                - search for given trigger id before start printing\n");
   printf("   -sub                    - try to scan for subsub events (default false)\n");
   printf("   -stat                   - accumulate different kinds of statistics (default false)\n");
   printf("   -minsz                  - find sequence id of event with minimum size\n");
   printf("   -maxsz                  - find sequence id of event with maximum size\n");
   printf("   -raw                    - printout of raw data (default false)\n");
   printf("   -onlyerr                - printout only TDC data with errors\n");
   printf("   -cts id                 - printout raw data as CTS subsubevent\n");
   printf("   -tdc id                 - printout raw data as TDC subsubevent\n");
   printf("   -ctdc id                - printout raw data as CTDC subsubevent\n");
   printf("   -new id                 - printout raw data as new TDC subsubevent\n");
   printf("   -adc id                 - printout raw data as ADC subsubevent\n");
   printf("   -mdc id                 - printout raw data as MDC TDC subsubevent\n");
   printf("   -hub id                 - identify hub inside subevent (default none)\n");
   printf("   -auto                   - automatically assign ID for TDCs (0x0zzz or 0x1zzz) and HUBs (0x8zzz) (default false)\n");
   printf("   -range mask             - select bits which are used to detect TDC or ADC (default 0xff)\n");
   printf("   -onlyctdc subsubid      - printout of CTDC data only for specified subsubevent\n");
   printf("   -onlyraw subsubid       - printout of raw data only for specified subsubevent\n");
   printf("   -onlynew subsubid       - printout raw data only for specified TDC4 subsubevent\n");
   printf("   -onlymonitor id         - printout only event/subevent created by hadaq::Monitor module (default off) \n");
   printf("   -onlymdc id             - printout of MDC data only for specified subsubevent\n");
   printf("   -fullid value           - printout only events with specified fullid (default all)\n");
   printf("   -rate                   - display only events and data rate\n");
   printf("   -allepoch               - epoch should be provided for each channel (default off)\n");
   print_tdc_arguments();
   printf("   -again [N=1]            - repeat same printout N times, only for debug purposes\n\n");
   printf("Example - display only data from TDC 0x1226:\n\n");
   printf("   hldprint localhost:6789 -num 1 -auto -onlytdc 0x1226\n\n");
   printf("Show statistic over all events in HLD file:\n\n");
   printf("   hldprint file.hld -all -stat\n");

   return errstr ? 1 : 0;
}

enum TrbDecodeKind {
   decode_SingleSubev = 0x8       // subevent contains single sub-sub event
};


struct SubevStat {
   long unsigned num{0};               // number of subevent seen
   long unsigned sizesum{0};           // sum of all subevents sizes
   bool          istdc{false};         // indicate if it is TDC subevent
   std::vector<long unsigned> tdcerr;  // tdc errors
   unsigned      maxch{0};             // maximal channel ID

   double aver_size() { return num>0 ? sizesum / (1.*num) : 0.; }
   double tdcerr_rel(unsigned n) { return (n < tdcerr.size()) && (num>0) ? tdcerr[n] / (1.*num) : 0.; }

   SubevStat() = default;
   SubevStat(const SubevStat& src) : num(src.num), sizesum(src.sizesum), istdc(src.istdc), tdcerr(src.tdcerr), maxch(src.maxch) {}

   void accumulate(unsigned sz)
   {
      num++;
      sizesum += sz;
   }

   void IncTdcError(unsigned id)
   {
      if (tdcerr.empty())
         tdcerr.assign(NumTdcErr, 0);
      if (id < tdcerr.size()) tdcerr[id]++;
   }

};

void PrintTdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix, unsigned& errmask, SubevStat *substat = nullptr, bool as_ver4 = false)
{
   if (len == 0) return;

   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());
   if (ix >= sz) return;

   std::vector<uint32_t> data(len, 0);
   for (unsigned i = 0; i < len; ++i)
      data[i] = sub->Data(i + ix);

   unsigned maxch = PrintTdcDataPlain(ix, data, prefix, errmask, as_ver4);

   if (substat) {
      if (substat->maxch < maxch)
         substat->maxch = maxch;
   }
}

void PrintCtsData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if ((ix >= sz) || (len == 0)) return;
   if ((len == 0) || (ix + len > sz)) len = sz - ix;

   unsigned data = sub->Data(ix++); len--;
   unsigned trigtype = (data & 0xFFFF);

   unsigned nInputs = (data >> 16) & 0xf;
   unsigned nITCInputs = (data >> 20) & 0xf;
   unsigned bIdleDeadTime = (data >> 25) & 0x1;
   unsigned bTriggerStats = (data >> 26) & 0x1;
   unsigned bIncludeTimestamp = (data >> 27) & 0x1;
   unsigned nExtTrigFlag = (data >> 28) & 0x3;

   printf("%*sITC status bitmask: 0x%04x \n", prefix, "", trigtype);
   printf("%*sNumber of inputs counters: %u \n", prefix, "", nInputs);
   printf("%*sNumber of ITC counters: %u \n", prefix, "", nITCInputs);
   printf("%*sIdle/dead counter: %s\n", prefix, "", bIdleDeadTime ? "yes" : "no");
   printf("%*sTrigger statistic: %s\n", prefix, "", bTriggerStats ? "yes" : "no");
   printf("%*sTimestamp: %s\n", prefix, "", bIncludeTimestamp ? "yes" : "no");

   // printing of inputs
   for (unsigned ninp=0;ninp<nInputs;++ninp) {
      unsigned wrd1 = sub->Data(ix++); len--;
      unsigned wrd2 = sub->Data(ix++); len--;
      printf("%*sInput %u level 0x%08x edge 0x%08x\n", prefix, "", ninp, wrd1, wrd2);
   }

   for (unsigned ninp=0;ninp<nITCInputs;++ninp) {
      unsigned wrd1 = sub->Data(ix++); len--;
      unsigned wrd2 = sub->Data(ix++); len--;
      printf("%*sITC Input %u level 0x%08x edge 0x%08x\n", prefix, "", ninp, wrd1, wrd2);
   }

   if (bIdleDeadTime) {
      unsigned wrd1 = sub->Data(ix++); len--;
      unsigned wrd2 = sub->Data(ix++); len--;
      printf("%*sIdle 0x%08x dead 0x%08x time\n", prefix, "", wrd1, wrd2);
   }

   if (bTriggerStats) {
      unsigned wrd1 = sub->Data(ix++); len--;
      unsigned wrd2 = sub->Data(ix++); len--;
      unsigned wrd3 = sub->Data(ix++); len--;
      printf("%*sTrigger stats 0x%08x 0x%08x 0x%08x\n", prefix, "", wrd1, wrd2, wrd3);
   }

   if (bIncludeTimestamp) {
      unsigned wrd1 = sub->Data(ix++); len--;
      printf("%*sTimestamp 0x%08x\n", prefix, "", wrd1);
   }

   printf("%*sExternal trigger flag: 0x%x ", prefix, "", nExtTrigFlag);
   if(nExtTrigFlag == 0x1) {
      data = sub->Data(ix++); len--;
      unsigned fTrigSyncId = (data & 0xFFFFFF);
      unsigned fTrigSyncIdStatus = data >> 24; // untested
      printf("MBS VULOM syncid 0x%06x status 0x%02x\n", fTrigSyncId, fTrigSyncIdStatus);
   } else if(nExtTrigFlag == 0x2) {
      // ETM sends four words, is probably a Mainz A2 recv
      data = sub->Data(ix++); len--;
      unsigned fTrigSyncId = data; // full 32bits is trigger number
      // get status word
      data = sub->Data(ix++); len--;
      unsigned fTrigSyncIdStatus = data;
      // word 3+4 are 0xdeadbeef i.e. not used at the moment, so skip it
      ix += 2;
      len -= 2;

      printf("MAINZ A2 recv syncid 0x%08x status 0x%08x\n", fTrigSyncId, fTrigSyncIdStatus);
   } else if(nExtTrigFlag == 0x0) {

      printf("SYNC ");

      if (sub->Data(ix) == 0xabad1dea) {
         // [1]: D[31:16] -> sync pulse number
         //      D[15:0]  -> absolute time D[47:32]
         // [2]: D[31:0]  -> absolute time D[31:0]
         // [3]: D[31:0]  -> period of sync pulse, in 10ns units
         // [4]: D[31:0]  -> length of sync pulse, in 10ns units

         unsigned fTrigSyncId = (sub->Data(ix+1) >> 16) & 0xffff;
         long unsigned fTrigTm = (((uint64_t) (sub->Data(ix+1) & 0xffff)) << 32) | sub->Data(ix+2);
         unsigned fSyncPulsePeriod = sub->Data(ix+3);
         unsigned fSyncPulseLength = sub->Data(ix+4);

         printf(" id 0x%04x tm %lu period %u lentgh %u\n", fTrigSyncId, fTrigTm, fSyncPulsePeriod, fSyncPulseLength);

         ix += 5;
         len -= 5;
      } else {
         printf("unknown word 0x%08x found, expects 0xabad1dea\n", sub->Data(ix));
      }
   } else {
      printf("  NOT RECOGNIZED!\n");
   }

   if ((len > 1) && ((sub->Data(ix) & tdckind_Header) == tdckind_Header)) {
      unsigned errmask = 0;
      PrintTdcData(sub, ix, len, prefix, errmask);
   }
}

void PrintCtdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if ((ix >= sz) || (len == 0)) return;
   if ((len == 0) || (ix + len > sz)) len = sz - ix;

   unsigned wlen = (sz > 999) ? 4 : (sz > 99 ? 3 : 2);

   signed reference = 0;

   for (unsigned cnt = 0; cnt < len; cnt++, ix++) {
      unsigned data = sub->Data(ix);
      if (prefix > 0) {
         printf("%*s[%*u] %08x  ", prefix, "", wlen, ix, data);
         if (cnt == 0) {
            reference = data & 0x1FFF;
            printf("ref:%5.1f\n", reference * 0.4);
         } else if ((data >> 26) & 1) {
            printf("ch:%02u ERROR\n", (unsigned) data >> 27);
         } else {
            unsigned channel = (unsigned) data >> 27;
            signed risingHit = (data >> 13) & 0x1fff;
            signed fallingHit = (data >> 0) & 0x1fff;

            float timeDiff = ((risingHit - reference)) * 0.4;
            if (timeDiff < -1250)
               timeDiff += 8192 * 0.4;
            if (timeDiff > 1250)
               timeDiff -= 8192 * 0.4;

            float timeToT = (fallingHit - risingHit) * 0.4;
            if (timeToT < 0)
               timeToT += 8192 * 0.4;
            printf("ch:%02u rising:%5.1f ToT:%5.1f\n", channel, timeDiff, timeToT);
         }
      }
   }
}

void PrintMdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if ((ix >= sz) || (len == 0))
      return;
   if ((len == 0) || (ix + len > sz))
      len = sz - ix;

   unsigned wlen = (sz > 999) ? 4 : (sz > 99 ? 3 : 2),
            reference = 0;

   for (unsigned cnt = 0; cnt < len; cnt++, ix++) {
      unsigned data = sub->Data(ix);
      if (prefix > 0)
         printf("%*s[%*u] %08x  ", prefix, "", wlen, ix, data);
      if (cnt == 0) {
         reference = data & 0x1FFF;
      } else if ((data >> 26) & 1) {
         unsigned channel = data >> 27;
         if (prefix > 0)
            printf("ch: %2u %sError%s", channel, getCol(col_RED), getCol(col_RESET));
      } else {
         unsigned channel = data >> 27;
         signed risingHit = (data >> 13) & 0x1fff;
         signed fallingHit = (data >> 0) & 0x1fff;

         float timeDiff = ((risingHit - reference)) * 0.4;
         if (timeDiff < -2100)
            timeDiff += 8192 * 0.4;
         if (timeDiff > 300)
            timeDiff -= 8192 * 0.4;

         float timeDiff1 = ((fallingHit - reference)) * 0.4;
         if (timeDiff1 < -2100)
            timeDiff1 += 8192 * 0.4;
         if (timeDiff1 > 300)
            timeDiff1 -= 8192 * 0.4;

         float timeToT = (fallingHit - risingHit) * 0.4;
         if (timeToT < 0)
            timeToT += 8192 * 0.4;

         if (prefix > 0)
            printf("ch: %2u  TO: %5.1f  T1: %5.1f  ToT: %4.1f", channel, timeDiff, timeDiff1, timeToT);
      }

      printf("\n");
   }
}


void PrintAdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if ((ix >= sz) || (len == 0)) return;
   if ((len == 0) || (ix + len > sz)) len = sz - ix;

   unsigned wlen = (sz > 999) ? 4 : (sz > 99 ? 3 : 2);

   for (unsigned cnt = 0; cnt < len; cnt++, ix++) {
      unsigned msg = sub->Data(ix);
      if (prefix > 0)
         printf("%*s[%*u] %08x  ", prefix, "", wlen, ix, msg);
      printf("\n");
   }
}

void PrintMonitorData(hadaq::RawSubevent *sub)
{
   unsigned trbSubEvSize = sub->GetSize() / 4 - 4, ix = 0;

   int cnt = 0;
   while (ix < trbSubEvSize) {
      unsigned addr1 = sub->Data(ix++);
      unsigned addr2 = sub->Data(ix++);
      unsigned value = sub->Data(ix++);
      printf("       %3d: %04x %04x = %08x\n", cnt++, addr1, addr2, value);
   }
}

bool printraw = false, printsub = false, showrate = false, reconnect = false, dostat = false,
     dominsz = false, domaxsz = false, autoid = false, only_errors = false;
unsigned idrange = 0xff, onlynew = 0, onlyctdc = 0, onlyraw = 0, onlymdc = 0, hubmask = 0, fullid = 0, adcmask = 0, onlymonitor = 0;
std::vector<unsigned> hubs, tdcs, ctdcs, ctsids, newtdcs, mdcs;
int buffer_size = 4;

bool is_cts(unsigned id)
{
   return std::find(ctsids.begin(), ctsids.end(), id) != ctsids.end();
}

bool is_hub(unsigned id)
{
   if (std::find(hubs.begin(), hubs.end(), id) != hubs.end()) return true;

   if (!autoid || ((id & 0xF000) != 0x8000)) return false;

   hubs.emplace_back(id);

   return true;
}

bool is_ctdc(unsigned id)
{
   return (std::find(ctdcs.begin(), ctdcs.end(), id) != ctdcs.end());
}


bool is_tdc(unsigned id)
{
   if (std::find(tdcs.begin(), tdcs.end(), id) != tdcs.end()) return true;

   if (autoid) {
      if (((id & 0xF000) == 0x0000) || ((id & 0xF000) == 0x1000)) {
         tdcs.emplace_back(id);
         return true;
      }
   }

   for (unsigned n=0;n<tdcs.size();n++)
      if (((id & idrange) <= (tdcs[n] & idrange)) && ((id & ~idrange) == (tdcs[n] & ~idrange)))
         return true;

   return false;
}

bool is_newtdc(unsigned id)
{
   if (std::find(newtdcs.begin(), newtdcs.end(), id) != newtdcs.end())
      return true;

   for (unsigned n=0;n<newtdcs.size();n++)
      if (((id & idrange) <= (newtdcs[n] & idrange)) && ((id & ~idrange) == (newtdcs[n] & ~idrange)))
         return true;

   return false;
}

bool is_mdc(unsigned id)
{
   if (std::find(mdcs.begin(), mdcs.end(), id) != mdcs.end())
      return true;

   for (auto entry : mdcs)
      if (((id & idrange) <= (entry & idrange)) && ((id & ~idrange) == (entry & ~idrange)))
         return true;

   return false;
}


bool is_adc(unsigned id)
{
   return ((adcmask != 0) && ((id & idrange) <= (adcmask & idrange)) && ((id & ~idrange) == (adcmask & ~idrange)));
}

int main(int argc, char* argv[])
{
   if ((argc < 2) || !strcmp(argv[1], "-help") || !strcmp(argv[1], "?"))
      return usage();

   long number = 10, skip = 0, nagain = 0;
   unsigned find_trigid = 0, find_eventid = 0;
   double tmout = -1., maxage = -1., debug_delay = -1;
   bool dofind = false;
   unsigned tdcmask = 0, ctsid = 0;

   int n = 1;
   while (++n < argc) {
      if ((strcmp(argv[n], "-num") == 0) && (n + 1 < argc)) {
         dabc::str_to_lint(argv[++n], &number);
      } else if ((strcmp(argv[n], "-buf") == 0) && (n + 1 < argc)) {
         dabc::str_to_int(argv[++n], &buffer_size);
      } else if (strcmp(argv[n], "-all") == 0) {
         number = 0;
      } else if ((strcmp(argv[n], "-skip") == 0) && (n + 1 < argc)) {
         dabc::str_to_lint(argv[++n], &skip);
      } else if ((strcmp(argv[n], "-event") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &find_eventid);
         dofind = true;
      } else if ((strcmp(argv[n], "-find") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &find_trigid);
         dofind = true;
      } else if ((strcmp(argv[n], "-cts") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &ctsid);
         ctsids.emplace_back(ctsid);
      } else if ((strcmp(argv[n], "-tdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &tdcmask);
         tdcs.emplace_back(tdcmask);
      } else if ((strcmp(argv[n], "-ctdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &tdcmask);
         ctdcs.emplace_back(tdcmask);
      } else if ((strcmp(argv[n], "-new") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &tdcmask);
         newtdcs.emplace_back(tdcmask);
      } else if ((strcmp(argv[n], "-range") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &idrange);
      } else if ((strcmp(argv[n], "-onlyctdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &onlyctdc);
      } else if ((strcmp(argv[n], "-onlynew") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &onlynew);
      } else if ((strcmp(argv[n], "-onlyraw") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &onlyraw);
      } else if ((strcmp(argv[n], "-mdc") == 0) && (n + 1 < argc)) {
         unsigned mdc = 0;
         dabc::str_to_uint(argv[++n], &mdc);
         mdcs.emplace_back(mdc);
      } else if ((strcmp(argv[n], "-onlymdc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &onlymdc);
      } else if ((strcmp(argv[n], "-adc") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &adcmask);
      } else if ((strcmp(argv[n], "-fullid") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &fullid);
      } else if ((strcmp(argv[n], "-hub") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &hubmask);
         hubs.emplace_back(hubmask);
      } else if ((strcmp(argv[n], "-onlymonitor") == 0) && (n + 1 < argc)) {
         dabc::str_to_uint(argv[++n], &onlymonitor);
      } else if ((strcmp(argv[n], "-tmout") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &tmout);
      } else if ((strcmp(argv[n], "-maxage") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &maxage);
      } else if ((strcmp(argv[n], "-delay") == 0) && (n + 1 < argc)) {
         dabc::str_to_double(argv[++n], &debug_delay);
      } else if (strcmp(argv[n], "-onlyerr") == 0) {
         only_errors = true;
      } else if (strcmp(argv[n], "-raw") == 0) {
         printraw = true;
      } else if (strcmp(argv[n], "-sub") == 0) {
         printsub = true;
      } else if (strcmp(argv[n], "-auto") == 0) {
         autoid = true;
         printsub = true;
      } else if (strcmp(argv[n], "-stat") == 0) {
         dostat = true;
      } else if (strcmp(argv[n], "-minsz") == 0) {
         dominsz = true;
      } else if (strcmp(argv[n], "-maxsz") == 0) {
         domaxsz = true;
      } else if (strcmp(argv[n], "-rate") == 0) {
         showrate = true;
         reconnect = true;
      } else if ((strcmp(argv[n], "-help") == 0) || (strcmp(argv[n], "?") == 0))
         return usage();
      else if (strcmp(argv[n], "-again") == 0) {
         if ((n + 1 < argc) && (argv[n + 1][0] != '-'))
            dabc::str_to_lint(argv[++n], &nagain);
         else
            nagain++;
      } else if (!scan_tdc_arguments(n, argc, argv))
         return usage("Unknown option");
   }

   if ((adcmask != 0) || !tdcs.empty() || !ctdcs.empty() || (onlytdc != 0) || (onlyctdc != 0) || (onlynew != 0) || (onlyraw != 0) || (onlymdc != 0)) { printsub = true; }

   printf("Try to open %s\n", argv[1]);

   bool ishld = false;
   std::string src = argv[1];
   if (((src.find(".hld") != std::string::npos) || (src.find(".hll") != std::string::npos)) && (src.find("hld://") != 0)) {
      src = std::string("hld://") + src;
      ishld = true;
   } else if ((src.find("hld://") == 0) || (src.find(".hld") != std::string::npos) || (src.find(".hll") != std::string::npos)) {
      ishld = true;
   }

   if (tmout < 0) tmout = ishld ? 0.5 : 5.;

   if (!ishld) {

      dabc::Url url(src);

      if (url.IsValid()) {
         if (url.GetProtocol().empty())
            src = std::string("mbss://") + src;

         if (reconnect && !url.HasOption("reconnect")) {
           if (url.GetOptions().empty())
              src+="?reconnect";
           else
              src+="&reconnect";
         }
      }
   }


   hadaq::RawEvent *evnt = nullptr;

   std::map<unsigned,SubevStat> idstat;     // events id statistic
   std::map<unsigned,SubevStat> substat;    // sub-events statistic
   std::map<unsigned,SubevStat> subsubstat; // sub-sub-events statistic
   long cnt = 0, cnt0 = 0, maxcnt = -1, mincnt = -1, lastcnt = 0, printcnt = 0;
   uint32_t minseqnr = 0, maxseqnr = 0;
   uint64_t lastsz = 0, currsz = 0, minsz = 10000000000, numminsz = 0, maxsz = 0, nummaxsz = 0;
   dabc::TimeStamp last, first, lastevtm;

   hadaq::ReadoutHandle ref;

   dabc::InstallSignalHandlers();

   while (nagain-- >= 0) {

   ref = hadaq::ReadoutHandle::Connect(src, buffer_size);

   if (ref.null()) return 1;

   idstat.clear();
   substat.clear();
   subsubstat.clear();
   cnt = cnt0 = lastcnt = printcnt = 0;
   lastsz = currsz = 0;
   last = first = lastevtm = dabc::Now();

   while (!dabc::CtrlCPressed()) {

      evnt = ref.NextEvent(maxage > 0 ? maxage/2. : 1., maxage);

      cnt0++;

      if (debug_delay > 0) dabc::Sleep(debug_delay);

      dabc::TimeStamp curr = dabc::Now();

      if (evnt) {

         if (dostat)
            idstat[evnt->GetId()].accumulate(evnt->GetSize());

         // ignore events which are not match with specified id
         if ((fullid != 0) && (evnt->GetId() != fullid)) continue;

         if (dominsz) {
            if (evnt->GetSize() < minsz) {
               minsz = evnt->GetSize();
               minseqnr = evnt->GetSeqNr();
               mincnt = cnt;
               numminsz = 1;
            } else if (evnt->GetSize() == minsz) {
               numminsz++;
            }
         }

         if (domaxsz) {
            if (evnt->GetSize() > maxsz) {
               maxsz = evnt->GetSize();
               maxseqnr = evnt->GetSeqNr();
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

      if (dofind) {
         if (find_eventid) {
            if (evnt->GetSeqNr() != find_eventid) continue;
         } else {
            auto *sub = evnt->NextSubevent(nullptr);
            if (!sub || (sub->GetTrigNr() != find_trigid)) continue;
         }
         dofind = false; // disable finding
      }

      printcnt++;

      bool print_header = false;

      if (!showrate && !dostat && !only_errors && (onlymonitor == 0)) {
         print_header = true;
         evnt->Dump();
      }

      char errbuf[100];

      hadaq::RawSubevent* sub = nullptr;
      while ((sub = evnt->NextSubevent(sub)) != nullptr) {
         bool print_sub_header = false;
         if ((onlytdc == 0) && (onlyctdc == 0) && (onlynew == 0) && (onlyraw == 0) && (onlymdc == 0) && (onlymonitor == 0) && !showrate && !dostat && !only_errors) {
            sub->Dump(printraw && !printsub);
            print_sub_header = true;
         }

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4, ix = 0,
                  maxhublen = 0, lasthubid = 0, lasthublen = 0,
                  maxhhublen = 0, lasthhubid = 0, lasthhublen = 0,
                  data, datalen, datakind;

         bool standalone_subevnt = (sub->GetDecoding() & hadaq::EvtDecoding_AloneSubevt) != 0;

         if (dostat)
            substat[sub->GetId()].accumulate(sub->GetSize());

         if (onlymonitor != 0) {
            if (sub->GetId() == onlymonitor) {
               evnt->Dump();
               sub->Dump(printraw);
               if (!printraw)
                  PrintMonitorData(sub);
            }
            break;
         }

         while ((ix < trbSubEvSize) && (printsub || dostat)) {

            if (standalone_subevnt && (ix == 0)) {
               data = 0; // unused
               datalen = trbSubEvSize > 2 ? trbSubEvSize - 2 : 0; // whole subevent beside last 2 words with 0x5555 id
               datakind = sub->GetId();
            } else {
               data = sub->Data(ix++);
               datalen = (data >> 16) & 0xFFFF;
               datakind = data & 0xFFFF;
            }

            errbuf[0] = 0;
            if (maxhhublen > 0) {
               if (datalen >= maxhhublen) datalen = maxhhublen-1;
               maxhhublen -= (datalen+1);
            } else {
               lasthhubid = 0;
            }

            bool as_raw = false, as_cts = false, as_tdc = false, as_ctdc = false, as_new = false, as_mdc = false, as_adc = false,
                 print_subsubhdr = (onlytdc == 0) && (onlyctdc == 0) && (onlynew == 0) && (onlyraw == 0) && (onlymdc == 0) && !only_errors;

            if (maxhublen > 0) {

               if (is_hub(datakind)) {
                  maxhublen--; // just decrement
                  if (dostat) {
                     subsubstat[datakind].accumulate(datalen);
                  } else if (!showrate && print_subsubhdr) {
                     printf("         *** HHUB size %3u id 0x%04x full %08x\n", datalen, datakind, data);
                  }
                  maxhhublen = datalen;
                  lasthhubid = datakind;
                  lasthhublen = datalen;
                  continue;
               }

               if (datalen >= maxhublen) {
                  snprintf(errbuf, sizeof(errbuf), " wrong format, want size 0x%04x", datalen);
                  datalen = maxhublen-1;
               }
               maxhublen -= (datalen+1);
            } else {
               lasthubid = 0;
            }

            if (is_tdc(datakind))
               as_tdc = !onlytdc && !onlyctdc && !onlynew && !onlyraw && !onlymdc;
            else if (is_ctdc(datakind))
               as_ctdc = !onlytdc && !onlyctdc && !onlynew && !onlyraw && !onlymdc;
            else if (is_newtdc(datakind))
               as_new = !onlytdc && !onlyctdc && !onlynew && !onlyraw && !onlymdc;

            if (!as_tdc) {
               if ((onlytdc != 0) && (datakind == onlytdc)) {
                  as_tdc = true;
                  print_subsubhdr = true;
               } else if ((onlyctdc != 0) && (datakind == onlyctdc)) {
                  as_ctdc = true;
                  print_subsubhdr = true;
               } else if ((onlynew != 0) && (datakind == onlynew)) {
                  as_new = true;
                  print_subsubhdr = true;
               } else if (is_cts(datakind)) {
                  as_cts = true;
               } else if (is_mdc(datakind)) {
                  as_mdc = true;
               } else if (is_adc(datakind)) {
                  as_adc = true;
               } else if ((maxhublen == 0) && is_hub(datakind)) {
                  // this is hack - skip hub header, inside is normal subsub events structure
                  if (dostat) {
                     subsubstat[datakind].accumulate(datalen);
                  } else if (!showrate && print_subsubhdr) {
                     printf("      *** HUB size %3u id 0x%04x full %08x\n", datalen, datakind, data);
                  }
                  maxhublen = datalen;
                  lasthubid = datakind;
                  lasthublen = datalen;
                  continue;
               } else if ((onlyraw != 0) && (datakind == onlyraw)) {
                  as_raw = true;
                  print_subsubhdr = true;
               } else if ((onlymdc != 0) && (datakind == onlymdc)) {
                  as_mdc = true;
                  print_subsubhdr = true;
               } else if (printraw) {
                  as_raw = !onlytdc && !onlyctdc && !onlynew && !onlyraw && !onlymdc;
               }
            }

            if (!dostat && !showrate) {
               // do raw printout when necessary

               unsigned errmask = 0;

               if (only_errors) {
                  // only check data without printing
                  if (as_tdc) PrintTdcData(sub, ix, datalen, 0, errmask);
                  // no errors - no print
                  if (errmask == 0) { ix += datalen; continue; }

                  print_subsubhdr = true;
                  errmask = 0;
               }

               if (as_raw || as_tdc || as_ctdc || as_new || as_mdc || as_adc) {
                  if (!print_header) {
                     print_header = true;
                     evnt->Dump();
                  }

                  if (!print_sub_header) {
                     sub->Dump(false);
                     if (lasthubid != 0)
                        printf("      *** HUB size %3u id 0x%04x\n", lasthublen, lasthubid);
                     if (lasthhubid != 0)
                        printf("         *** HHUB size %3u id 0x%04x\n", lasthhublen, lasthhubid);
                     print_sub_header = true;
                  }
               }

               unsigned prefix = 9;
               if (lasthhubid != 0)
                  prefix = 15;
               else if (lasthubid != 0)
                  prefix = 12;

               // when print raw selected with autoid, really print raw
               if (printraw && autoid && as_tdc) { as_tdc = false; as_raw = true; }

               if (print_subsubhdr) {
                  const char *kind = "Subsubevent";
                  if (as_tdc)
                     kind = "TDC ";
                  else if (as_ctdc)
                     kind = "CTDC ";
                  else if (as_new)
                     kind = "TDC ";
                  else if (as_cts)
                     kind = "CTS ";
                  else if (as_mdc)
                     kind = "MDC ";
                  else if (as_adc)
                     kind = "ADC ";

                  printf("%*s*** %s size %3u id 0x%04x", prefix-3, "", kind, datalen, datakind);
                  if(standalone_subevnt && (ix == 0))
                     printf(" alone");
                  else
                     printf(" full %08x", data);
                  printf("%s\n", errbuf);
               }

               if (as_tdc)
                  PrintTdcData(sub, ix, datalen, prefix, errmask);
               else if (as_ctdc)
                  PrintCtdcData(sub, ix, datalen, prefix);
               else if (as_new)
                  PrintTdcData(sub, ix, datalen, prefix, errmask, nullptr, true);
               else if (as_mdc)
                  PrintMdcData(sub, ix, datalen, prefix);
               else if (as_adc)
                  PrintAdcData(sub, ix, datalen, prefix);
               else if (as_cts)
                  PrintCtsData(sub, ix, datalen, prefix);
               else if (as_raw)
                  sub->PrintRawData(ix, datalen, prefix);

               if (errmask != 0) {
                  printf("         %s!!!! TDC errors:%s", getCol(col_RED), getCol(col_RESET));
                  unsigned mask = 1;
                  for (int k = 0; k < NumTdcErr; k++,mask*=2)
                     if (errmask & mask) printf(" err_%s", TdcErrName(k));
                  printf("\n");
               }
            } else
            if (dostat) {
               auto &substat2 = subsubstat[datakind];

               substat2.num++;
               substat2.sizesum+=datalen;
               if (as_tdc) {
                  substat2.istdc = true;
                  unsigned errmask = 0;
                  PrintTdcData(sub, ix, datalen, 0, errmask, &substat2);
                  unsigned mask = 1;
                  for (int k = 0; k < NumTdcErr; k++,mask*=2)
                     if (errmask & mask) substat2.IncTdcError(k);
               }
            }

            ix += datalen;

            if (standalone_subevnt) break;
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

      int width = 3;
      if (printcnt > 1000) width = 6;

      printf("  Events ids:\n");
      for (auto &entry : idstat)
         printf("   0x%04x : cnt %*lu averlen %6.1f\n", entry.first, width, entry.second.num, entry.second.aver_size());

      printf("  Subevents ids:\n");
      for (auto &entry : substat)
         printf("   0x%04x : cnt %*lu averlen %6.1f\n", entry.first, width, entry.second.num, entry.second.aver_size());

      printf("  Subsubevents ids:\n");
      for (auto &entry : subsubstat) {
         auto &substat2 = entry.second;

         printf("   0x%04x : cnt %*lu averlen %6.1f", entry.first, width, substat2.num, substat2.aver_size());

         if (substat2.istdc) {
            printf(" TDC ch:%2u", substat2.maxch);
            for (unsigned k = 0; k < substat2.tdcerr.size(); k++)
               if (substat2.tdcerr[k] > 0) {
                  printf(" %s=%lu (%3.1f%s)", TdcErrName(k), substat2.tdcerr[k], substat2.tdcerr_rel(k) * 100., "\%");
               }
         }

         printf("\n");
      }
   }

   if (dominsz && mincnt >= 0)
      printf("Event #0x%08x (-skip %ld) has minimal size %lu cnt %lu\n", (unsigned) minseqnr, mincnt, (long unsigned) minsz, (long unsigned) numminsz);

   if (domaxsz && maxcnt >= 0)
      printf("Event #0x%08x (-skip %ld) has maximal size %lu cnt %lu\n", (unsigned) maxseqnr, maxcnt, (long unsigned) maxsz, (long unsigned) nummaxsz);

   if (dabc::CtrlCPressed()) break;

   } // ngain--

   return 0;
}

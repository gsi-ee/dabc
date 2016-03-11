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

#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include <algorithm>
#include <time.h>

#include "hadaq/api.h"
#include "dabc/string.h"
#include "dabc/Url.h"
#include "dabc/api.h"

int usage(const char* errstr = 0)
{
   if (errstr!=0) {
      printf("Error: %s\n\n", errstr);
   }

   printf("utility for printing HLD events\n");
   printf("hldprint source [args]\n\n");
   printf("Following source kinds are supported:\n");
   printf("   hld://path/file.hld         - HLD file reading\n");
   printf("   file.hld                    - HLD file reading (file extension MUST be '.hld')\n");
   printf("   dabcnode                    - DABC stream server\n");
   printf("   dabcnode:port               - DABC stream server with custom port\n");
   printf("   mbss://dabcnode/Transport   - DABC transport server\n");
   printf("   lmd://path/file.lmd         - LMD file reading\n");
   printf("Additional arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -maxage value           - maximal age time for events, if expired queue are cleaned (default off)\n");
   printf("   -num number             - number of events to print, 0 - all events (default 10)\n");
   printf("   -skip number            - number of events to skip before start printing\n");
   printf("   -sub                    - try to scan for subsub events (default false)\n");
   printf("   -stat                   - accumulate different kinds of statistics (default false)\n");
   printf("   -raw                    - printout of raw data (default false)\n");
   printf("   -onlyraw subsubid       - printout of raw data only for specified subsubevent\n");
   printf("   -tdc mask               - printout raw data as TDC subsubevent (default none) \n");
   printf("   -range mask             - select bits which are used to detect TDC or ADC (default 0xff) \n");
   printf("   -onlytdc tdcid          - printout raw data only of specified tdc subsubevent (default none) \n");
   printf("   -tot boundary           - printout only events with TOT less than specified boundary, should be combined with -onlytdc (default disabled) \n");
   printf("   -adc mask               - printout raw data as ADC subsubevent (default none) \n");
   printf("   -fullid value           - printout only events with specified fullid (default all) \n");
   printf("   -hub value              - identify hub inside subevent to printout raw data inside (default none) \n");
   printf("   -rate                   - display only events rate\n");
   printf("   -fine-min value         - minimal fine counter value, used for liner time calibration (default 31) \n");
   printf("   -fine-max value         - maximal fine counter value, used for liner time calibration (default 491) \n");
   printf("   -bubble                 - display TDC data as bubble, require 19 words in TDC subevent\n");

   return errstr ? 1 : 0;
}

enum TdcMessageKind {
   tdckind_Reserved = 0x00000000,
   tdckind_Header   = 0x20000000,
   tdckind_Debug    = 0x40000000,
   tdckind_Epoch    = 0x60000000,
   tdckind_Mask     = 0xe0000000,
   tdckind_Hit      = 0x80000000,
   tdckind_Hit1     = 0xa0000000,
   tdckind_Calibr   = 0xe0000000
};

enum { NumTdcErr = 4 };

enum TdcErrorsKind {
   tdcerr_MissHeader  = 0x0001,
   tdcerr_MissCh0     = 0x0002,
   tdcerr_MissEpoch   = 0x0004,
   tdcerr_NoData      = 0x0008
};

const char* TdcErrName(int cnt) {
   switch(cnt) {
      case 0: return "header";
      case 1: return "ch0";
      case 2: return "epoch";
      case 3: return "nodata";
   }
   return "unknown";
}

struct SubevStat {

   long unsigned num;        // number of subevent seen
   long unsigned sizesum;    // sum of all subevents sizes
   bool          istdc;      // indicate if it is TDC subevent
   long unsigned tdcerr[NumTdcErr];  // tdc errors

   double aver_size() { return num>0 ? sizesum / (1.*num) : 0.; }
   double tdcerr_rel(unsigned n) { return (n < NumTdcErr) && (num>0) ? tdcerr[n] / (1.*num) : 0.; }

   SubevStat() : num(0), sizesum(0), istdc(false)
      {  for (int n=0;n<NumTdcErr;n++) tdcerr[n] = 0; }
   SubevStat(const SubevStat& src) : num(src.num), sizesum(src.sizesum), istdc(src.istdc)
      {  for (int n=0;n<NumTdcErr;n++) tdcerr[n] = src.tdcerr[n]; }

};


double tot_limit(-1);
unsigned fine_min(31), fine_max(491);
bool bubble_mode = false;

const char* debug_name[32] = {
      "Number of valid triggers",
      "Number of release signals send",
      "Number of valid timing triggers received",
      "Valid NOtiming trigger number",
      "Invalid trigger number",
      "Multi timing trigger number",
      "Spurious trigger number",
      "Wrong readout number",
      "Spike number",
      "Idle time",
      "Wait time",
      "Total empty channels",
      "Readout time",
      "Timeout number",
      "Temperature",
      "RESERVED",
      "Compile time 1",
      "Compile time 2",
      "debug 0x10010",
      "debug 0x10011",
      "debug 0x10100",
      "debug 0x10101",
      "debug 0x10110",
      "debug 0x10111",
      "debug 0x11000",
      "debug 0x11001",
      "debug 0x11010",
      "debug 0x11011",
      "debug 0x11100",
      "debug 0x11101",
      "debug 0x11110",
      "debug 0x11111"
};

bool PrintBubbleData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix) {
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if (ix>=sz) return false;
   if ((len==0) || (ix + len > sz)) len = sz - ix;

   if (prefix==0) return false;

   unsigned lastch = 0xFFFF;

   for (unsigned cnt=0;cnt<len;cnt++,ix++) {
      unsigned msg = sub->Data(ix);

      if ((msg & tdckind_Mask) != tdckind_Hit) continue;

      unsigned chid = (msg >> 22) & 0x7F;

      if (chid != lastch) {
         if (lastch < 0xFFFF) printf("\n");
         printf("%*s ch%02u: ",  prefix, "", chid);
         lastch = chid;
      }
      unsigned data = msg & 0xFFFF, swap_data = 0;
      for (int n=0;n<16;n++) {
         swap_data = (swap_data >> 1) | ((data & 0x8000) ? 0x8000 : 0);
         data = data << 1;
      }

      printf("%04x", swap_data);
   }

   if (lastch<0xFFFF) printf("\n");

   return true;

}


bool PrintTdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix, unsigned& errmask, bool check_conditions = false)
{
   if (bubble_mode) return PrintBubbleData(sub, ix, len, prefix);

   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if (ix>=sz) return false;
   if ((len==0) || (ix + len > sz)) len = sz - ix;

   unsigned wlen = 2;
   if (sz>99) wlen = 3; else
   if (sz>999) wlen = 4;

   unsigned epoch(0);
   double tm, ch0tm(0);

   errmask = 0;

   bool haschannel0(false);
   unsigned channel(0), fine(0), ndebug(0), nheader(0), isrising(0), dkind(0), dvalue(0), rawtime(0);
   int epoch_channel(-11); // -11 no epoch, -1 - new epoch, 0..127 - epoch assigned with specified channel

   double last_rising[65], last_falling[65];
   for (int n=0;n<66;n++) {
      last_rising[n] = 0;
      last_falling[n] = 0;
   }

   char sbuf[100];
   unsigned calibr[2] = { 0xffff, 0xffff };
   int ncalibr = 2;

   for (unsigned cnt=0;cnt<len;cnt++,ix++) {
      unsigned msg = sub->Data(ix);
      if (prefix>0) printf("%*s[%*u] %08x  ",  prefix, "", wlen, ix, msg);

      if ((cnt==0) && ((msg & tdckind_Mask) != tdckind_Header)) errmask |= tdcerr_MissHeader;

      switch (msg & tdckind_Mask) {
         case tdckind_Reserved:
            if (prefix>0) printf("tdc trailer ttyp:0x%01x rnd:0x%02x err:0x%04x\n", (msg >> 24) & 0xF,  (msg >> 16) & 0xFF, msg & 0xFFFF);
            break;
         case tdckind_Header:
            nheader++;
            if (prefix>0) printf("tdc header\n");
            break;
         case tdckind_Debug:
            ndebug++;
            dkind = (msg >> 24) & 0x1F;
            dvalue = msg & 0xFFFFFF;
            sbuf[0] = 0;
            if (dkind == 0x10) rawtime = dvalue; else
            if (dkind == 0x11) {
               rawtime += (dvalue << 16);
               time_t t = (time_t) rawtime;
               sprintf(sbuf, "  design 0x%08x %s", rawtime, ctime(&t));
               int len = strlen(sbuf);
               if (sbuf[len-1]==10) sbuf[len-1] = 0;
            } else
            if (dkind == 0xE) sprintf(sbuf, " %3.1fC", dvalue/16.);

            if (prefix>0)
               printf("tdc debug 0x%02x: 0x%06x %s%s\n", dkind, dvalue, debug_name[dkind], sbuf);
            break;
         case tdckind_Epoch:
            epoch = msg & 0xFFFFFFF;
            tm = (epoch << 11) *5.;
            epoch_channel = -1; // indicate that we have new epoch
            if (prefix>0) printf("epoch %u tm %6.3f ns\n", msg & 0xFFFFFFF, tm);
            break;
         case tdckind_Calibr:
            calibr[0] = msg & 0x3fff;
            calibr[1] = (msg >> 14) & 0x3fff;
            ncalibr = 0;
            if (prefix>0) printf("tdc calibr v1 0x%04x v2 0x%04x\n", calibr[0], calibr[1]);
            break;
         case tdckind_Hit:
         case tdckind_Hit1:
            channel = (msg >> 22) & 0x7F;
            if (channel == 0) haschannel0 = true;
            if (epoch_channel==-1) epoch_channel = channel;
            isrising = (msg >> 11) & 0x1;

            if ((epoch_channel == -11) || (epoch_channel != (int) channel)) errmask |= tdcerr_MissEpoch;

            tm = ((epoch << 11) + (msg & 0x7FF)) * 5.; // coarse time
            fine = (msg >> 12) & 0x3FF;
            if (fine<0x3ff) {
               if ((msg & tdckind_Mask) == tdckind_Hit1) {
                  if (isrising) {
                     tm -= fine*5e-3; // calibrated time, 5 ps/bin
                  } else {
                     tm -= (fine & 0x1FF)*10e-3; // for falling edge 10 ps binning is used
                     if (fine & 0x200) tm -= 0x800 * 5.; // in rare case time correction leads to epoch overflow
                  }
               } else
               if (ncalibr<2) {
                  // calibrated time, 5 ns correspond to value 0x3ffe or about 0.30521 ps/bin
                  double corr = calibr[ncalibr++]*5./0x3ffe;
                  if (!isrising) corr*=10.; // for falling edge correction 50 ns range is used
                  tm -= corr;
               } else {
                  tm -= 5.*(fine > fine_min ? fine - fine_min : 0) / (0. + fine_max - fine_min); // simple approx of fine time from range 31-491
               }
            }

            sbuf[0] = 0;
            if (isrising) {
               last_rising[channel] = tm;
            } else {
               last_falling[channel] = tm;
               if (last_rising[channel] > 0) {
                  double tot = last_falling[channel] - last_rising[channel];
                  bool cond = (tot_limit > 0) && (tot < tot_limit);
                  sprintf(sbuf," tot:%6.3f ns%s", tot,cond ? " !!!BINGO!!!" : "");
                  last_rising[channel] = 0;
                  if (check_conditions && cond) return true;
               }
            }

            if (prefix>0) printf("%s ch:%2u isrising:%u tc:0x%03x tf:0x%03x tm:%6.3f ns%s\n",
                                 ((msg & tdckind_Mask) == tdckind_Hit1) ? "hit1" : "hit ",
                                 channel, isrising, (msg & 0x7FF), fine, tm - ch0tm, sbuf);
            if ((channel==0) && (ch0tm==0)) ch0tm = tm;
            break;
         default:
            if (prefix>0) printf("undefined\n");
            break;
      }
   }

   if (len<2) { if (nheader!=1) errmask |= tdcerr_NoData; } else
   if (!haschannel0 && (ndebug==0)) errmask |= tdcerr_MissCh0;

   return false;
}

void PrintAdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if ((ix>=sz) || (len==0)) return;
   if ((len==0) || (ix + len > sz)) len = sz - ix;

   unsigned wlen = 2;
   if (sz>99) wlen = 3; else
   if (sz>999) wlen = 4;

   for (unsigned cnt=0;cnt<len;cnt++,ix++) {
      unsigned msg = sub->Data(ix);
      if (prefix>0) printf("%*s[%*u] %08x  ",  prefix, "", wlen, ix, msg);
      printf("\n");
   }
}

int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   long number(10), skip(0);
   double tmout(-1.), maxage(-1.), debug_delay(-1);
   bool printraw(false), printsub(false), showrate(false), reconnect(false), dostat(false);
   unsigned tdcmask(0), idrange(0xff), onlytdc(0), onlyraw(0), hubmask(0), fullid(0), adcmask(0);
   std::vector<unsigned> hubs;
   std::vector<unsigned> tdcs;

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &number); } else
      if ((strcmp(argv[n],"-skip")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &skip); } else
      if ((strcmp(argv[n],"-tdc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &tdcmask); tdcs.push_back(tdcmask); } else
      if ((strcmp(argv[n],"-range")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &idrange); } else
      if ((strcmp(argv[n],"-onlytdc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &onlytdc); } else
      if ((strcmp(argv[n],"-fine-min")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fine_min); } else
      if ((strcmp(argv[n],"-fine-max")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fine_max); } else
      if ((strcmp(argv[n],"-tot")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tot_limit); } else
      if ((strcmp(argv[n],"-onlyraw")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &onlyraw); } else
      if ((strcmp(argv[n],"-adc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &adcmask); } else
      if ((strcmp(argv[n],"-fullid")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fullid); } else
      if ((strcmp(argv[n],"-hub")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &hubmask); hubs.push_back(hubmask); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if ((strcmp(argv[n],"-maxage")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &maxage); } else
      if ((strcmp(argv[n],"-delay")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &debug_delay); } else
      if (strcmp(argv[n],"-bubble")==0) { bubble_mode = true; } else
      if (strcmp(argv[n],"-raw")==0) { printraw = true; } else
      if (strcmp(argv[n],"-sub")==0) { printsub = true; } else
      if (strcmp(argv[n],"-stat")==0) { dostat = true; } else
      if (strcmp(argv[n],"-rate")==0) { showrate = true; reconnect = true; } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   if ((adcmask!=0) || (tdcs.size()!=0) || (onlytdc!=0) || (onlyraw!=0)) { printsub = true; printraw = true; }

   printf("Try to open %s\n", argv[1]);

   bool ishld = false;
   std::string src = argv[1];
   if ((src.find(".hld") != std::string::npos) && (src.find("hld://") != 0)) {
      src = std::string("hld://") + src;
      ishld = true;
   }

   if ((src.find("hld://") == 0) || (src.find(".hld") != std::string::npos)) ishld = true;

   if (tmout<0) tmout = ishld ? 0.5 : 5.;

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

   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect(src.c_str());

   if (ref.null()) return 1;

   hadaq::RawEvent* evnt(0);

   std::map<unsigned,SubevStat> idstat; // events id counter
   std::map<unsigned,SubevStat> stat;   // subevents statistic
   long cnt(0), cnt0(0), lastcnt(0), printcnt(0);
   dabc::TimeStamp last = dabc::Now();
   dabc::TimeStamp first = last;
   dabc::TimeStamp lastevtm = last;

   dabc::InstallCtrlCHandler();

   while (!dabc::CtrlCPressed()) {

      evnt = ref.NextEvent(maxage > 0 ? maxage/2. : 1., maxage);

      cnt0++;

      if (debug_delay>0) dabc::Sleep(debug_delay);

      dabc::TimeStamp curr = dabc::Now();

      if (evnt!=0) {

         if (dostat) {
            idstat[evnt->GetId()].num++;
            idstat[evnt->GetId()].sizesum+=evnt->GetSize();
         }

         // ignore events which are nor match with specified id
         if ((fullid!=0) && (evnt->GetId()!=fullid)) continue;

         cnt++;
         lastevtm = curr;
      } else
      if (curr - lastevtm > tmout) { /*printf("TIMEOUT %ld\n", cnt0);*/ break; }

      if (showrate) {

         double tm = curr - last;

         if (tm>=0.3) {
            printf("\rTm:%6.1fs  Ev:%8ld  Rate:%8.2f Ev/s", first.SpentTillNow(), cnt, (cnt-lastcnt)/tm);
            fflush(stdout);
            last = curr;
            lastcnt = cnt;
         }

         // when showing rate, only with statistic one need to analyze event
         if (!dostat) continue;
      }

      if (evnt==0) continue;

      if (skip>0) { skip--; continue; }

      if ((onlytdc!=0) && (tot_limit>0)) {
         hadaq::RawSubevent* sub = 0;
         bool found(false), cond_res(false);
         while (!found && ((sub=evnt->NextSubevent(sub))!=0)) {

            unsigned trbSubEvSize = sub->GetSize() / 4 - 4;
            unsigned ix(0);

            while (ix < trbSubEvSize) {
               unsigned data = sub->Data(ix++);

               unsigned datalen = (data >> 16) & 0xFFFF;
               unsigned datakind = data & 0xFFFF;

               if (std::find(hubs.begin(), hubs.end(), datakind) != hubs.end()) continue;

               if (datakind == onlytdc) {
                  found = true;
                  unsigned errmask(0);
                  cond_res = PrintTdcData(sub, ix, datalen, 0, errmask, true);
                  break;
               }

               ix+=datalen;
            }
         }
         if (!cond_res) continue;
      }

      printcnt++;

      if (!showrate && !dostat)
         evnt->Dump();

      hadaq::RawSubevent* sub = 0;
      while ((sub = evnt->NextSubevent(sub)) != 0) {

         bool print_sub_header(false);
         if ((onlytdc==0) && (onlyraw==0) && !showrate && !dostat) {
            sub->Dump(printraw && !printsub);
            print_sub_header = true;
         }

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;
         unsigned ix = 0;

         while ((ix < trbSubEvSize) && (printsub || dostat)) {
            unsigned data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned datakind = data & 0xFFFF;

            bool as_raw(false), as_tdc(false), as_adc(false);

            for (unsigned n=0;n<tdcs.size();n++)
               if (((datakind & idrange) <= (tdcs[n] & idrange)) && ((datakind & ~idrange) == (tdcs[n] & ~idrange)))
                  as_tdc = true;

            if (!as_tdc) {
               if ((onlytdc!=0) && (datakind==onlytdc)) {
                  as_tdc = true;
               } else
               if ((adcmask!=0) && ((datakind & idrange) <= (adcmask & idrange)) && ((datakind & ~idrange) == (adcmask & ~idrange))) {
                  as_adc = true;
               } else
               if (std::find(hubs.begin(), hubs.end(), datakind) != hubs.end()) {
                  // this is hack - skip hub header, inside is normal subsub events structure
                  if (dostat) {
                     stat[datakind].num++;
                     stat[datakind].sizesum+=datalen;
                  } else
                  if (!showrate) {
                     printf("      *** HUB size %3u id 0x%04x full %08x\n", datalen, datakind, data);
                  }
                  continue;
               } else
               if ((onlyraw!=0) && (datakind==onlyraw)) {
                  as_raw = true;
               } else
               if (printraw) {
                  as_raw = (onlytdc==0) && (onlyraw==0);
               }
            }

            if (!dostat && !showrate) {
               // do raw printout when necessary

               if (as_raw || as_tdc || as_adc) {
                  if (!print_sub_header) {
                     sub->Dump(false);
                     print_sub_header = true;
                  }
               }

               printf("      *** Subsubevent size %3u id 0x%04x full %08x\n", datalen, datakind, data);

               unsigned errmask(0);

               if (as_tdc) PrintTdcData(sub, ix, datalen, 9, errmask, false); else
               if (as_adc) PrintAdcData(sub, ix, datalen, 9); else
               if (as_raw) sub->PrintRawData(ix, datalen, 9);

               if (errmask!=0) {
                  printf("         !!!! TDC errors detected:");
                  unsigned mask = 1;
                  for (int n=0;n<NumTdcErr;n++,mask*=2)
                     if (errmask & mask) printf(" err_%s", TdcErrName(n));
                  printf("\n");
               }

            } else
            if (dostat) {
               stat[datakind].num++;
               stat[datakind].sizesum+=datalen;
               if (as_tdc) {
                  stat[datakind].istdc = true;
                  unsigned errmask(0);
                  PrintTdcData(sub, ix, datalen, 0, errmask, false);
                  unsigned mask = 1;
                  for (int n=0;n<NumTdcErr;n++,mask*=2)
                     if (errmask & mask) stat[datakind].tdcerr[n]++;
               }
            }

            ix+=datalen;
         }
      }

      if ((number>0) && (printcnt>=number)) break;
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
      for (std::map<unsigned,SubevStat>::iterator iter = idstat.begin(); iter!=idstat.end(); iter++)
         printf("   0x%04x : cnt %*lu averlen %5.1f\n", iter->first, width, iter->second.num, iter->second.aver_size());

      printf("  Subevents ids:\n");
      for (std::map<unsigned,SubevStat>::iterator iter = stat.begin(); iter!=stat.end(); iter++) {
         printf("   0x%04x : cnt %*lu averlen %5.1f", iter->first, width, iter->second.num, iter->second.aver_size());

         if (iter->second.istdc) {
            printf(" TDC");
            for (int n=0;n<NumTdcErr;n++)
               if (iter->second.tdcerr[n]>0) {
                  printf(" %s=%lu (%3.1f%s)", TdcErrName(n), iter->second.tdcerr[n], iter->second.tdcerr_rel(n) * 100., "\%");
               }
         }

         printf("\n");
      }

   }

   return 0;
}

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

   printf("Utility for printing HLD events. 22.10.2018. S.Linev\n");
   printf("   hldprint source [args]\n");
   printf("Following sources are supported:\n");
   printf("   hld://path/file.hld         - HLD file reading\n");
   printf("   file.hld                    - HLD file reading (file extension MUST be '.hld')\n");
   printf("   dabcnode                    - DABC stream server\n");
   printf("   dabcnode:port               - DABC stream server with custom port\n");
   printf("   mbss://dabcnode/Transport   - DABC transport server\n");
   printf("   lmd://path/file.lmd         - LMD file reading\n");
   printf("Arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -maxage value           - maximal age time for events, if expired queue are cleaned (default off)\n");
   printf("   -num number             - number of events to print, 0 - all events (default 10)\n");
   printf("   -all                    - print all events (equivalent to -num 0)\n");
   printf("   -skip number            - number of events to skip before start printing\n");
   printf("   -find id                - search for given event id before start analysis\n");
   printf("   -sub                    - try to scan for subsub events (default false)\n");
   printf("   -stat                   - accumulate different kinds of statistics (default false)\n");
   printf("   -raw                    - printout of raw data (default false)\n");
   printf("   -onlyraw subsubid       - printout of raw data only for specified subsubevent\n");
   printf("   -onlyerr                - printout only TDC data with errors\n");
   printf("   -tdc id                 - printout raw data as TDC subsubevent (default none) \n");
   printf("   -adc id                 - printout raw data as ADC subsubevent (default none) \n");
   printf("   -hub id                 - identify hub inside subevent (default none) \n");
   printf("   -auto                   - automatically assign ID for TDCs (0x0zzz or 0x1zzz) and HUBs (0x8zzz) (default false)\n");
   printf("   -range mask             - select bits which are used to detect TDC or ADC (default 0xff) \n");
   printf("   -onlytdc tdcid          - printout raw data only of specified tdc subsubevent (default none) \n");
   printf("   -tot boundary           - minimal allowed value for ToT (default 20 ns) \n");
   printf("   -stretcher value        - approximate stretcher length for falling edge (default 20 ns) \n");
   printf("   -ignorecalibr           - ignore calibration messages (default off)\n");
   printf("   -fullid value           - printout only events with specified fullid (default all) \n");
   printf("   -rate                   - display only events and data rate\n");
   printf("   -bw                     - disable colors\n");
   printf("   -allepoch               - epoch should be provided for each channel (default off)\n");
   printf("   -fine-min value         - minimal fine counter value, used for liner time calibration (default 31) \n");
   printf("   -fine-max value         - maximal fine counter value, used for liner time calibration (default 491) \n");
   printf("   -bubble                 - display TDC data as bubble, require 19 words in TDC subevent\n\n");
   printf("Example - display only data from TDC 0x1226:\n\n");
   printf("   hldprint localhost:6789 -num 1 -auto -onlytdc 0x1226\n\n");
   printf("Show statistic over all events in HLD file:\n\n");
   printf("   hldprint file.hld -all -stat\n");

   return errstr ? 1 : 0;
}

enum TdcMessageKind {
   tdckind_Reserved = 0x00000000,
   tdckind_Header   = 0x20000000,
   tdckind_Debug    = 0x40000000,
   tdckind_Epoch    = 0x60000000,
   tdckind_Mask     = 0xe0000000,
   tdckind_Hit      = 0x80000000, // normal hit message
   tdckind_Hit1     = 0xa0000000, // hardware- corrected hit message, instead of 0x3ff
   tdckind_Hit2     = 0xc0000000, // special hit message with regular fine time
   tdckind_Calibr   = 0xe0000000  // extra calibration message for hits
};

enum { NumTdcErr = 6 };

enum TdcErrorsKind {
   tdcerr_MissHeader  = 0x0001,
   tdcerr_MissCh0     = 0x0002,
   tdcerr_MissEpoch   = 0x0004,
   tdcerr_NoData      = 0x0008,
   tdcerr_Sequence    = 0x0010,
   tdcerr_ToT         = 0x0020
};


const char *col_RESET   = "\033[0m";
const char *col_BLACK   = "\033[30m";      /* Black */
const char *col_RED     = "\033[31m";      /* Red */
const char *col_GREEN   = "\033[32m";      /* Green */
const char *col_YELLOW  = "\033[33m";      /* Yellow */
const char *col_BLUE    = "\033[34m";      /* Blue */
const char *col_MAGENTA = "\033[35m";      /* Magenta */
const char *col_CYAN    = "\033[36m";      /* Cyan */
const char *col_WHITE   = "\033[37m";      /* White */

const char* TdcErrName(int cnt) {
   switch (cnt) {
      case 0: return "header";
      case 1: return "ch0";
      case 2: return "epoch";
      case 3: return "nodata";
      case 4: return "seq";
      case 5: return "tot";
   }
   return "unknown";
}

struct SubevStat {
   long unsigned num;        // number of subevent seen
   long unsigned sizesum;    // sum of all subevents sizes
   bool          istdc;      // indicate if it is TDC subevent
   std::vector<long unsigned> tdcerr;  // tdc errors
   unsigned      maxch;      // maximal channel ID

   double aver_size() { return num>0 ? sizesum / (1.*num) : 0.; }
   double tdcerr_rel(unsigned n) { return (n < tdcerr.size()) && (num>0) ? tdcerr[n] / (1.*num) : 0.; }

   SubevStat() : num(0), sizesum(0), istdc(false), tdcerr(), maxch(0) {}
   SubevStat(const SubevStat& src) : num(src.num), sizesum(src.sizesum), istdc(src.istdc), tdcerr(src.tdcerr), maxch(src.maxch) {}

   void IncTdcError(unsigned id)
   {
      if (tdcerr.size() == 0)
         tdcerr.assign(NumTdcErr, 0);
      if (id < tdcerr.size()) tdcerr[id]++;
   }

};


double tot_limit(20.), tot_shift(20.);
unsigned fine_min(31), fine_max(491);
bool bubble_mode = false, only_errors = false, use_colors = true, epoch_per_channel = false, use_calibr = true;

const char *getCol(const char *col_name)
{
   return use_colors ? col_name : "";
}

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

unsigned BUBBLE_SIZE = 19;

unsigned BubbleCheck(unsigned* bubble, int &p1, int &p2) {
   p1 = 0; p2 = 0;

   unsigned pos = 0, last = 1, nflip = 0;

   int b1 = 0, b2 = 0;

   std::vector<unsigned> fliparr(BUBBLE_SIZE*16);

   for (unsigned n=0;n<BUBBLE_SIZE; n++) {
      unsigned data = bubble[n] & 0xFFFF;
      if (n < BUBBLE_SIZE-1) data = data | ((bubble[n+1] & 0xFFFF) << 16); // use word to recognize bubble

      // this is error - first bit always 1
      if ((n==0) && ((data & 1) == 0)) { return -1; }

      for (unsigned b=0;b<16;b++) {
         if ((data & 1) != last) {
            if (last==1) {
               if (p1==0) p1 = pos; // take first change from 1 to 0
            } else {
               p2 = pos; // use last change from 0 to 1
            }
            nflip++;
         }

         fliparr[pos] = nflip; // remember flip counts to analyze them later

         // check for simple bubble at the beginning 1101000 or 0x0B in swapped order
         // set position on last 1 ? Expecting following sequence
         //  1110000 - here pos=4
         //     ^
         //  1110100 - here pos=5
         //      ^
         //  1111100 - here pos=6
         //       ^
         if ((data & 0xFF) == 0x0B) b1 = pos+3;

         // check for simple bubble at the end 00001011 or 0xD0 in swapped order
         // set position of 0 in bubble, expecting such sequence
         //  0001111 - here pos=4
         //     ^
         //  0001011 - here pos=5
         //      ^
         //  0000011 - here pos=6
         //       ^
         if ((data & 0xFF) == 0xD0) b2 = pos+5;

         // simple bubble at very end 00000101 or 0xA0 in swapped order
         // here not enough space for two bits
         if (((pos == BUBBLE_SIZE*16 - 8)) && (b2 == 0) && ((data & 0xFF) == 0xA0))
            b2 = pos + 6;


         last = (data & 1);
         data = data >> 1;
         pos++;
      }
   }

   if (nflip == 2) return 0; // both are ok

   if ((nflip == 4) && (b1>0) && (b2==0)) { p1 = b1; return 0x10; } // bubble in the begin

   if ((nflip == 4) && (b1==0) && (b2>0)) { p2 = b2; return 0x01; } // bubble at the end

   if ((nflip == 6) && (b1>0) && (b2>0)) { p1 = b1; p2 = b2; return 0x11; } // bubble on both side

   // up to here was simple errors, now we should do more complex analysis

   if (p1 < p2 - 8) {
      // take flip count at the middle and check how many transitions was in between
      int mid = (p2+p1)/2;
      // hard error in the beginning
      if (fliparr[mid] + 1 == fliparr[p2]) return 0x20;
      // hard error in begin, bubble at the end
      if ((fliparr[mid] + 3 == fliparr[p2]) && (b2>0)) { p2 = b2; return 0x21; }

      // hard error at the end
      if (fliparr[p1] == fliparr[mid]) return 0x02;
      // hard error at the end, bubble at the begin
      if ((fliparr[p1] + 2 == fliparr[mid]) && (b1>0)) { p1 = b1; return 0x12; }
   }

   return 0x22; // mark both as errors, should analyze better
}

void PrintBubble(unsigned* bubble, unsigned len = 0) {
   // print in original order, time from right to left
   // for (unsigned d=BUBBLE_SIZE;d>0;d--) printf("%04x",bubble[d-1]);

   if (len==0) len = BUBBLE_SIZE;
   // print in reverse order, time from left to right
   for (unsigned d=0;d<len;d++) {
      unsigned origin = bubble[d], swap = 0;
      for (unsigned dd = 0;dd<16;++dd) {
         swap = (swap << 1) | (origin & 1);
         origin = origin >> 1;
      }
      printf("%04x",swap);
   }
}

void PrintBubbleBinary(unsigned* bubble, int p1 = -1, int p2 = -1) {
   if (p1<0) p1 = 0;
   if (p2<=p1) p2 = BUBBLE_SIZE*16;

   int pos = 0;
   char sbuf[1000];
   char* ptr  = sbuf;

   for (unsigned d=0;d<BUBBLE_SIZE;d++) {
      unsigned origin = bubble[d];
      for (unsigned dd = 0;dd<16;++dd) {
         if ((pos>=p1) && (pos<=p2))
            *ptr++ = (origin & 0x1) ? '1' : '0';
         origin = origin >> 1;
         pos++;
      }
   }

   *ptr++ = 0;
   printf(sbuf);
}


bool PrintBubbleData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if (ix>=sz) return false;
   if ((len==0) || (ix + len > sz)) len = sz - ix;

   if (prefix==0) return false;

   unsigned lastch = 0xFFFF;
   unsigned bubble[190];
   unsigned bcnt = 0, msg = 0, chid = 0;
   int p1 = 0, p2 = 0;

   for (unsigned cnt=0;cnt<=len;cnt++,ix++) {
      chid = 0xFFFF; msg = 0;
      if (cnt<len) {
         msg = sub->Data(ix);
         if ((msg & tdckind_Mask) != tdckind_Hit) continue;
         chid = (msg >> 22) & 0x7F;
      }

      if (chid != lastch) {
         if (lastch != 0xFFFF) {
            printf("%*s ch%02u: ", prefix, "", lastch);
            if (bcnt==BUBBLE_SIZE) {

               PrintBubble(bubble);

               int chk = BubbleCheck(bubble, p1, p2);
               int left = p1-2;
               int right = p2+1;
               if ((chk & 0xF0) == 0x10) left--;
               if ((chk & 0x0F) == 0x01) right++;

               if (chk==0) printf(" norm"); else
               if (chk==0x22) {
                  printf(" corr "); PrintBubbleBinary(bubble, left, right);
               } else
               if (((chk & 0xF0) < 0x20) && ((chk & 0x0F) < 0x02)) {
                  printf(" bubb "); PrintBubbleBinary(bubble, left, right);
               } else {
                  printf(" mixe "); PrintBubbleBinary(bubble, left, right);
               }

            } else {
               printf("bubble data error length = %u, expected %u", bcnt, BUBBLE_SIZE);
            }

            printf("\n");
         }
         lastch = chid; bcnt = 0;
      }

      bubble[bcnt++] = msg & 0xFFFF;
      // printf("here\n");
   }

   return true;
}


void PrintTdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix, unsigned& errmask, SubevStat *substat = 0)
{
   if (len == 0) return;

   if (bubble_mode) {
      PrintBubbleData(sub, ix, len, prefix);
      return;
   }

   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if (ix>=sz) return;
   // here when len was 0 - rest of subevent was printed
   if ((len==0) || (ix + len > sz)) len = sz - ix;

   unsigned wlen = 2;
   if (sz>99) wlen = 3; else
   if (sz>999) wlen = 4;

   unsigned long long epoch(0);
   double tm, ch0tm(0);

   errmask = 0;

   bool haschannel0(false);
   unsigned channel(0), maxch(0), fine(0), ndebug(0), nheader(0), isrising(0), dkind(0), dvalue(0), rawtime(0);
   int epoch_channel(-11); // -11 no epoch, -1 - new epoch, 0..127 - epoch assigned with specified channel

   static unsigned NumCh = 66;

   double last_rising[NumCh], last_falling[NumCh];
   int leading_trailing[NumCh], num_leading[NumCh], num_trailing[NumCh];
   bool seq_err[NumCh];
   for (unsigned n=0;n<NumCh;n++) {
      last_rising[n] = 0;
      last_falling[n] = 0;
      leading_trailing[n] = 0;
      num_leading[n] = 0;
      num_trailing[n] = 0;
      seq_err[n] = false;
   }

   unsigned bubble[100];
   int bubble_len = -1, nbubble = 0;
   unsigned bubble_ix = 0, bubble_ch = 0, bubble_eix = 0;

   char sbuf[100], sfine[100];
   unsigned calibr[2] = { 0xffff, 0xffff };
   int ncalibr = 2;
   const char* hdrkind = "";
   bool with_calibr = false;

   for (unsigned cnt=0;cnt<len;cnt++,ix++) {
      unsigned msg = sub->Data(ix);
      if (bubble_len>=0) {
         bool israw = (msg & tdckind_Mask) == tdckind_Calibr;
         if (israw) {
            channel = (msg >> 22) & 0x7F;
            if (bubble_len==0) { bubble_eix = bubble_ix = ix; bubble_ch = channel; }
            if (bubble_ch == channel) { bubble[bubble_len++] = msg & 0xFFFF; bubble_eix = ix; }
         }
         if ((bubble_len >= 100) || (cnt==len-1) || (channel!=bubble_ch) || (!israw && (bubble_len > 0))) {
            if (prefix>0) {
               printf("%*s[%*u..%*u] Ch:%02x bubble: ",  prefix, "", wlen, bubble_ix, wlen, bubble_eix, bubble_ch);
               PrintBubble(bubble, (unsigned) bubble_len);
               printf("\n");
               nbubble++;
            }
            bubble_len = 0; bubble_eix = bubble_ix = ix;
            if (bubble_ch != channel) {
               bubble_ch = channel;
               bubble[bubble_len++] = msg & 0xFFFF;
            }
         }
         if (israw) continue;
         bubble_len = -1; // no bubbles
      }

      if (prefix>0) printf("%*s[%*u] %08x  ",  prefix, "", wlen, ix, msg);

      if ((cnt==0) && ((msg & tdckind_Mask) != tdckind_Header)) errmask |= tdcerr_MissHeader;

      switch (msg & tdckind_Mask) {
         case tdckind_Reserved:
            if (prefix>0) printf("tdc trailer ttyp:0x%01x rnd:0x%02x err:0x%04x\n", (msg >> 24) & 0xF,  (msg >> 16) & 0xFF, msg & 0xFFFF);
            break;
         case tdckind_Header:
            nheader++;
            switch ((msg >> 24) & 0x0F) {
               case 0x01: hdrkind = "double edges"; break;
               case 0x0F: hdrkind = "bubbles"; bubble_len = 0; break;
               default: hdrkind = "normal"; break;
            }

            if (prefix>0)
               printf("tdc header fmt:%x %s\n", ((msg >> 24) & 0x0F), hdrkind);
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
            if (use_calibr) ncalibr = 0;
            if (prefix>0) printf("tdc calibr v1 0x%04x v2 0x%04x\n", calibr[0], calibr[1]);
            break;
         case tdckind_Hit:
         case tdckind_Hit1:
         case tdckind_Hit2:
            channel = (msg >> 22) & 0x7F;
            if (channel == 0) haschannel0 = true;
            if (epoch_channel==-1) epoch_channel = channel;
            isrising = (msg >> 11) & 0x1;
            if (maxch<channel) maxch = channel;
            if (channel < NumCh) {
               if (isrising) {
                  num_leading[channel]++;
                  if (++leading_trailing[channel] > 1) seq_err[channel] = true;
               } else {
                  if (--leading_trailing[channel] < 0) seq_err[channel] = true;
                  num_trailing[channel]++;
                  leading_trailing[channel] = 0;
               }
            }

            if ((epoch_channel == -11) || (epoch_per_channel && (epoch_channel != (int) channel))) errmask |= tdcerr_MissEpoch;

            tm = ((epoch << 11) + (msg & 0x7FF)) * 5.; // coarse time
            fine = (msg >> 12) & 0x3FF;
            with_calibr = false;
            if (fine<0x3ff) {
               if ((msg & tdckind_Mask) == tdckind_Hit2) {
                  if (isrising) {
                     tm -= fine*5e-3; // calibrated time, 5 ps/bin
                  } else {
                     tm -= (fine & 0x1FF)*10e-3; // for falling edge 10 ps binning is used
                     if (fine & 0x200) tm -= 0x800 * 5.; // in rare case time correction leads to epoch overflow
                  }
                  with_calibr = true;
               } else if (ncalibr<2) {
                  // calibrated time, 5 ns correspond to value 0x3ffe or about 0.30521 ps/bin
                  double corr = calibr[ncalibr++]*5./0x3ffe;
                  if (!isrising) corr*=10.; // for falling edge correction 50 ns range is used
                  tm -= corr;
                  with_calibr = true;
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
                  bool cond = with_calibr ? ((tot >= 0) && (tot < tot_limit)) : ((tot >= tot_shift) && (tot < tot_shift + tot_limit));
                  if (!cond) errmask |= tdcerr_ToT;
                  snprintf(sbuf, sizeof(sbuf), " tot:%s%6.3f ns%s", getCol(cond ? col_GREEN : col_RED), tot, getCol(col_RESET));
                  last_rising[channel] = 0;
               }
            }

            if ((fine >= 600) && (fine != 0x3ff))
               snprintf(sfine, sizeof(sfine), "%s0x%03x%s", getCol(col_RED), fine, getCol(col_RESET));
            else
               snprintf(sfine, sizeof(sfine), "0x%03x", fine);

            if (prefix>0) printf("%s ch:%2u isrising:%u tc:0x%03x tf:%s tm:%6.3f ns%s\n",
                                 ((msg & tdckind_Mask) == tdckind_Hit) ? "hit " : (((msg & tdckind_Mask) == tdckind_Hit1) ? "hit1" : "hit2"),
                                 channel, isrising, (msg & 0x7FF), sfine, tm - ch0tm, sbuf);
            if ((channel==0) && (ch0tm==0)) ch0tm = tm;
            break;
         default:
            if (prefix>0) printf("undefined\n");
            break;
      }
   }

   if (len<2) { if (nheader!=1) errmask |= tdcerr_NoData; } else
   if (!haschannel0 && (ndebug==0) && (nbubble==0)) errmask |= tdcerr_MissCh0;

   for (unsigned n=1;n<NumCh;n++)
      if ((num_leading[n] > 0) && (num_trailing[n] > 0))
         if (seq_err[n] || (num_leading[n]!=num_trailing[n]))
            errmask |= tdcerr_Sequence;

   if (substat) {
      if (substat->maxch < maxch) substat->maxch = maxch;
   }
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

bool printraw(false), printsub(false), showrate(false), reconnect(false), dostat(false), autoid(false);
unsigned tdcmask(0), idrange(0xff), onlytdc(0), onlyraw(0), hubmask(0), fullid(0), adcmask(0);
std::vector<unsigned> hubs;
std::vector<unsigned> tdcs;

bool is_hub(unsigned id)
{
   if (std::find(hubs.begin(), hubs.end(), id) != hubs.end()) return true;

   if (!autoid || ((id & 0xF000) != 0x8000)) return false;

   hubs.push_back(id);

   return true;
}

bool is_tdc(unsigned id)
{
   if (std::find(tdcs.begin(), tdcs.end(), id) != tdcs.end()) return true;

   if (autoid) {
      if (((id & 0xF000) == 0x0000) || ((id & 0xF000) == 0x1000)) {
         tdcs.push_back(id);
         return true;
      }
   }

   for (unsigned n=0;n<tdcs.size();n++)
      if (((id & idrange) <= (tdcs[n] & idrange)) && ((id & ~idrange) == (tdcs[n] & ~idrange)))
         return true;

   return false;
}

bool is_adc(unsigned id)
{
   return ((adcmask!=0) && ((id & idrange) <= (adcmask & idrange)) && ((id & ~idrange) == (adcmask & ~idrange)));
}

int main(int argc, char* argv[])
{
   if ((argc<2) || !strcmp(argv[1],"-help") || !strcmp(argv[1],"?")) return usage();

   long number(10), skip(0);
   unsigned findid(0);
   double tmout(-1.), maxage(-1.), debug_delay(-1);
   bool dofind = false;

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &number); } else
      if (strcmp(argv[n],"-all")==0) { number = 0; } else
      if ((strcmp(argv[n],"-skip")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &skip); } else
      if ((strcmp(argv[n],"-find")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &findid); dofind = true; } else
      if ((strcmp(argv[n],"-tdc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &tdcmask); tdcs.push_back(tdcmask); } else
      if ((strcmp(argv[n],"-range")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &idrange); } else
      if ((strcmp(argv[n],"-onlytdc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &onlytdc); } else
      if ((strcmp(argv[n],"-fine-min")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fine_min); } else
      if ((strcmp(argv[n],"-fine-max")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fine_max); } else
      if ((strcmp(argv[n],"-tot")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tot_limit); } else
      if ((strcmp(argv[n],"-stretcher")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tot_shift); } else
      if ((strcmp(argv[n],"-onlyraw")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &onlyraw); } else
      if ((strcmp(argv[n],"-adc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &adcmask); } else
      if ((strcmp(argv[n],"-fullid")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fullid); } else
      if ((strcmp(argv[n],"-hub")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &hubmask); hubs.push_back(hubmask); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if ((strcmp(argv[n],"-maxage")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &maxage); } else
      if ((strcmp(argv[n],"-delay")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &debug_delay); } else
      if (strcmp(argv[n],"-bubble")==0) { bubble_mode = true; } else
      if (strcmp(argv[n],"-bubb18")==0) { bubble_mode = true; BUBBLE_SIZE = 18; } else
      if (strcmp(argv[n],"-bubb19")==0) { bubble_mode = true; BUBBLE_SIZE = 19; } else
      if (strcmp(argv[n],"-onlyerr")==0) { only_errors = true; } else
      if (strcmp(argv[n],"-raw")==0) { printraw = true; } else
      if (strcmp(argv[n],"-sub")==0) { printsub = true; } else
      if (strcmp(argv[n],"-auto")==0) { autoid = true; printsub = true; } else
      if (strcmp(argv[n],"-stat")==0) { dostat = true; } else
      if (strcmp(argv[n],"-rate")==0) { showrate = true; reconnect = true; } else
      if (strcmp(argv[n],"-bw")==0) { use_colors = false; } else
      if (strcmp(argv[n],"-sub")==0) { printsub = true; } else
      if (strcmp(argv[n],"-ignorecalibr")==0) { use_calibr = false; } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   if ((adcmask!=0) || (tdcs.size()!=0) || (onlytdc!=0) || (onlyraw!=0)) { printsub = true; }

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

   hadaq::RawEvent *evnt = nullptr;

   std::map<unsigned,SubevStat> idstat; // events id counter
   std::map<unsigned,SubevStat> stat;   // subevents statistic
   long cnt(0), cnt0(0), lastcnt(0), printcnt(0);
   uint64_t lastsz{0}, currsz{0};
   dabc::TimeStamp last = dabc::Now();
   dabc::TimeStamp first = last;
   dabc::TimeStamp lastevtm = last;

   dabc::InstallSignalHandlers();

   while (!dabc::CtrlCPressed()) {

      evnt = ref.NextEvent(maxage > 0 ? maxage/2. : 1., maxage);

      cnt0++;

      if (debug_delay>0) dabc::Sleep(debug_delay);

      dabc::TimeStamp curr = dabc::Now();

      if (evnt) {

         if (dostat) {
            idstat[evnt->GetId()].num++;
            idstat[evnt->GetId()].sizesum+=evnt->GetSize();
         }

         // ignore events which are nor match with specified id
         if ((fullid!=0) && (evnt->GetId()!=fullid)) continue;

         cnt++;
         currsz+=evnt->GetSize();
         lastevtm = curr;
      } else if (curr - lastevtm > tmout) {
         /*printf("TIMEOUT %ld\n", cnt0);*/
         break;
      }

      if (showrate) {

         double tm = curr - last;

         if (tm>=0.3) {
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

      if (skip>0) { skip--; continue; }

      if (dofind) {
         auto *sub = evnt->NextSubevent(nullptr);
         if (!sub || (sub->GetTrigNr() != findid)) continue;
         dofind = false; // disable
      }

      printcnt++;

      bool print_header(false);

      if (!showrate && !dostat && !only_errors) {
         print_header = true;
         evnt->Dump();
      }

      char errbuf[100];

      hadaq::RawSubevent* sub = nullptr;
      while ((sub = evnt->NextSubevent(sub)) != nullptr) {

         bool print_sub_header(false);
         if ((onlytdc==0) && (onlyraw==0) && !showrate && !dostat && !only_errors) {
            sub->Dump(printraw && !printsub);
            print_sub_header = true;
         }

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4, ix = 0,
                  maxhublen = 0, lasthubid = 0, lasthublen = 0,
                  maxhhublen = 0, lasthhubid = 0, lasthhublen = 0;

         while ((ix < trbSubEvSize) && (printsub || dostat)) {
            unsigned data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned datakind = data & 0xFFFF;

            errbuf[0] = 0;
            if (maxhhublen>0) {
               if (datalen >= maxhhublen) datalen = maxhhublen-1;
               maxhhublen -= (datalen+1);
            } else {
               lasthhubid = 0;
            }

            bool as_raw(false), as_tdc(false), as_adc(false), print_subsubhdr((onlytdc==0) && (onlyraw==0) && !only_errors);

            if (maxhublen>0) {

               if (is_hub(datakind)) {
                  maxhublen--; // just decrement
                  if (dostat) {
                     stat[datakind].num++;
                     stat[datakind].sizesum+=datalen;
                  } else
                  if (!showrate && print_subsubhdr) {
                     printf("         *** HHUB size %3u id 0x%04x full %08x\n", datalen, datakind, data);
                  }
                  maxhhublen = datalen;
                  lasthhubid = datakind;
                  lasthhublen = datalen;
                  continue;
               }

               if (datalen >= maxhublen) {
                  sprintf(errbuf," wrong format, want size 0x%04x", datalen);
                  datalen = maxhublen-1;
               }
               maxhublen -= (datalen+1);
            } else {
               lasthubid = 0;
            }

            if (is_tdc(datakind)) as_tdc = !onlytdc;

            if (!as_tdc) {
               if ((onlytdc!=0) && (datakind==onlytdc)) {
                  as_tdc = true;
                  print_subsubhdr = true;
               } else
               if (is_adc(datakind)) {
                  as_adc = true;
               } else
               if ((maxhublen==0) && is_hub(datakind)) {
                  // this is hack - skip hub header, inside is normal subsub events structure
                  if (dostat) {
                     stat[datakind].num++;
                     stat[datakind].sizesum+=datalen;
                  } else
                  if (!showrate && print_subsubhdr) {
                     printf("      *** HUB size %3u id 0x%04x full %08x\n", datalen, datakind, data);
                  }
                  maxhublen = datalen;
                  lasthubid = datakind;
                  lasthublen = datalen;
                  continue;
               } else
               if ((onlyraw!=0) && (datakind==onlyraw)) {
                  as_raw = true;
                  print_subsubhdr = true;
               } else
               if (printraw) {
                  as_raw = (onlytdc==0) && (onlyraw==0);
               }
            }

            if (!dostat && !showrate) {
               // do raw printout when necessary

               unsigned errmask(0);

               if (only_errors) {
                  // only check data without printing
                  if (as_tdc) PrintTdcData(sub, ix, datalen, 0, errmask);
                  // no errors - no print
                  if (errmask==0) { ix+=datalen; continue; }

                  print_subsubhdr = true;
                  errmask = 0;
               }

               if (as_raw || as_tdc || as_adc) {
                  if (!print_header) {
                     print_header = true;
                     evnt->Dump();
                  }

                  if (!print_sub_header) {
                     sub->Dump(false);
                     if (lasthubid!=0)
                        printf("      *** HUB size %3u id 0x%04x\n", lasthublen, lasthubid);
                     if (lasthhubid!=0)
                        printf("         *** HHUB size %3u id 0x%04x\n", lasthhublen, lasthhubid);
                     print_sub_header = true;
                  }
               }

               unsigned prefix(9);
               if (lasthhubid!=0) prefix = 15; else if (lasthubid!=0) prefix = 12;

               // when print raw selected with autoid, really print raw
               if (printraw && autoid && as_tdc) { as_tdc = false; as_raw = true; }

               if (print_subsubhdr) {
                  const char *kind = "Subsubevent";
                  if (as_tdc) kind = "TDC "; else
                  if (as_adc) kind = "ADC ";
                  printf("%*s*** %s size %3u id 0x%04x full %08x%s\n", prefix-3, "", kind, datalen, datakind, data, errbuf);
               }

               if (as_tdc) PrintTdcData(sub, ix, datalen, prefix, errmask); else
               if (as_adc) PrintAdcData(sub, ix, datalen, prefix); else
               if (as_raw) sub->PrintRawData(ix, datalen, prefix);

               if (errmask!=0) {
                  printf("         %s!!!! TDC errors:%s", getCol(col_RED), getCol(col_RESET));
                  unsigned mask = 1;
                  for (int n=0;n<NumTdcErr;n++,mask*=2)
                     if (errmask & mask) printf(" err_%s", TdcErrName(n));
                  printf("\n");
               }
            } else
            if (dostat) {
               SubevStat &substat = stat[datakind];

               substat.num++;
               substat.sizesum+=datalen;
               if (as_tdc) {
                  substat.istdc = true;
                  unsigned errmask(0);
                  PrintTdcData(sub, ix, datalen, 0, errmask, &substat);
                  unsigned mask = 1;
                  for (int n=0;n<NumTdcErr;n++,mask*=2)
                     if (errmask & mask) substat.IncTdcError(n);
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
         SubevStat &substat = iter->second;

         printf("   0x%04x : cnt %*lu averlen %5.1f", iter->first, width, substat.num, substat.aver_size());

         if (substat.istdc) {
            printf(" TDC ch:%2u", substat.maxch);
            for (unsigned n=0;n<substat.tdcerr.size();n++)
               if (substat.tdcerr[n] > 0) {
                  printf(" %s=%lu (%3.1f%s)", TdcErrName(n), substat.tdcerr[n], substat.tdcerr_rel(n) * 100., "\%");
               }
         }

         printf("\n");
      }

   }

   return 0;
}

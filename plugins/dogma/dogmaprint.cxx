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

bool printraw = false, printsub = false, showrate = false, reconnect = false, dostat = false, dominsz = false, domaxsz = false, autoid = false, use_colors = true;
unsigned idrange = 0xff, onlynew = 0, onlyraw = 0, hubmask = 0, fullid = 0, adcmask = 0, onlymonitor = 0;

// related to TDC print

unsigned fine_min = 31, fine_max = 491, fine_min4 = 28, fine_max4 = 350, skip_msgs_in_tdc = 0, onlytdc = 0;
double tot_limit = 20., tot_shift = 20., coarse_tmlen = 5.;
bool use_calibr = true, epoch_per_channel = false, use_400mhz = false, print_fulltime = false;
int onlych = -1;

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

unsigned BUBBLE_SIZE = 19;

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

const char *getCol(const char *col_name)
{
   return use_colors ? col_name : "";
}

enum {
   // with mask 1
   newkind_TMDT     = 0x80000000,
   // with mask 3
   newkind_Mask3    = 0xE0000000,
   newkind_HDR      = 0x20000000,
   newkind_TRL      = 0x00000000,
   newkind_EPOC     = 0x60000000,
   // with mask 4
   newkind_Mask4    = 0xF0000000,
   newkind_TMDS     = 0x40000000,
   // with mask 6
   newkind_Mask6    = 0xFC000000,
   newkind_TBD      = 0x50000000,
   // with mask 8
   newkind_Mask8    = 0xFF000000,
   newkind_HSTM     = 0x54000000,
   newkind_HSTL     = 0x55000000,
   newkind_HSDA     = 0x56000000,
   newkind_HSDB     = 0x57000000,
   newkind_CTA      = 0x58000000,
   newkind_CTB      = 0x59000000,
   newkind_TEMP     = 0x5A000000,
   newkind_BAD      = 0x5B000000,
   // with mask 9
   newkind_Mask9    = 0xFF800000,
   newkind_TTRM     = 0x5C000000,
   newkind_TTRL     = 0x5C800000,
   newkind_TTCM     = 0x5D000000,
   newkind_TTCL     = 0x5D800000,
   // with mask 7
   newkind_Mask7    = 0xFE000000,
   newkind_TMDR     = 0x5E000000
};




void PrintTdc4Data(unsigned ix, const std::vector<uint32_t> &data, unsigned prefix)
{
   unsigned len = data.size();

   unsigned wlen = len > 999 ? 4 : (len > 99 ? 3 : 2);

   unsigned ttype = 0;
   uint64_t lastepoch = 0;
   double coarse_unit = 1./2.8e8;
   double localtm0 = 0.;

   char sbeg[1000], sdata[1000];

   for (unsigned cnt = 0; cnt < len; cnt++, ix++) {
      unsigned msg = data[cnt];

      const char *kind = "unckn";

      sdata[0] = 0;

      if (prefix > 0)
         snprintf(sbeg, sizeof(sbeg), "%*s[%*u] %08x ",  prefix, "", wlen, ix, msg);
      if ((msg & newkind_TMDT) == newkind_TMDT) {
         kind = "TMDT";
         unsigned mode = (msg >> 27) & 0xF;
         unsigned channel = (msg >> 21) & 0x3F;
         unsigned coarse = (msg >> 9) & 0xFFF;
         unsigned fine = msg & 0x1FF;

         if ((onlych >= 0) && (channel != (unsigned) onlych)) continue;

         double localtm = ((lastepoch << 12) | coarse) * coarse_unit;
         if (fine > fine_max4)
            localtm -= coarse_unit;
         else if (fine > fine_min4)
            localtm -= (fine - fine_min4) / (0. + fine_max4 - fine_min4) * coarse_unit;

         snprintf(sdata, sizeof(sdata), "mode:0x%x ch:%u coarse:%u fine:%u tm0:%6.3fns", mode, channel, coarse, fine, (localtm - localtm0)*1e9);
      } else {
         unsigned hdr3 = msg & newkind_Mask3;
         unsigned hdr4 = msg & newkind_Mask4;
         unsigned hdr6 = msg & newkind_Mask6;
         unsigned hdr7 = msg & newkind_Mask7;
         unsigned hdr8 = msg & newkind_Mask8;
         unsigned hdr9 = msg & newkind_Mask9;
         if (hdr3 == newkind_HDR) {
            kind = "HDR";
            unsigned major = (msg >> 24) & 0xF;
            unsigned minor = (msg >> 20) & 0xF;
            ttype = (msg >> 16) & 0xF;
            unsigned trigger = msg & 0xFFFF;
            snprintf(sdata, sizeof(sdata), "version:%u.%u typ:0x%x  trigger:%u", major, minor, ttype, trigger);
         } else
         if (hdr3 == newkind_TRL) {

            switch (ttype) {
               case 0x4:
               case 0x6:
               case 0x7:
               case 0x8:
               case 0x9:
               case 0xE: {
                  kind = "TRLB";
                  unsigned eflags = (msg >> 24) & 0xF;
                  unsigned maxdc = (msg >> 20) & 0xF;
                  unsigned tptime = (msg >> 16) & 0xF;
                  unsigned freq = msg & 0xFFFF;
                  snprintf(sdata, sizeof(sdata), "eflags:0x%x maxdc:%u tptime:%u freq:%u", eflags, maxdc, tptime, freq);
                  break;
               }
               case 0xC: {
                  kind = "TRLC";
                  unsigned cpc = (msg >> 24) & 0x7;
                  unsigned ccs = (msg >> 20) & 0xF;
                  unsigned ccdiv = (msg >> 16) & 0xF;
                  unsigned freq = msg & 0xFFFF;
                  snprintf(sdata, sizeof(sdata), "cpc:0x%x ccs:0x%x ccdiv:%u freq:%5.3fMHz", cpc, ccs, ccdiv, freq*1e-2);
                  break;
               }
               case 0x0:
               case 0x1:
               case 0x2:
               case 0xf:
               default: {
                  kind = "TRLA";
                  unsigned platformid = (msg >> 20) & 0xff;
                  unsigned major = (msg >> 16) & 0xf;
                  unsigned minor = (msg >> 12) & 0xf;
                  unsigned sub2 = (msg >> 8) & 0xf;
                  unsigned numch = (msg & 0x7F) + 1;
                  snprintf(sdata, sizeof(sdata), "platform:0x%x version:%u.%u.%u numch:%u", platformid, major, minor, sub2, numch);
               }
            }

         } else
         if (hdr3 == newkind_EPOC) {
            kind = "EPOC";
            unsigned epoch = msg & 0xFFFFFFF;
            bool err = (msg & 0x10000000) != 0;
            snprintf(sdata, sizeof(sdata), "0x%07x%s", epoch, (err ? " errflag" : ""));
            lastepoch = epoch;
         } else
         if (hdr4 == newkind_TMDS) {
            kind = "TMDS";
            unsigned channel = (msg >> 21) & 0x7F;
            unsigned coarse = (msg >> 9) & 0xFFF;
            unsigned pattern = msg & 0x1FF;

            double localtm = ((lastepoch << 12) | coarse) * coarse_unit;
            unsigned mask = 0x100, cnt2 = 8;
            while (((pattern & mask) == 0) && (cnt2 > 0)) {
               mask = mask >> 1;
               cnt2--;
            }
            localtm -= coarse_unit/8*cnt2;

            snprintf(sdata, sizeof(sdata), "ch:%u coarse:%u pattern:0x%03x tm0:%5.1f", channel, coarse, pattern, (localtm - localtm0)*1e9);
         } else
         if (hdr6 == newkind_TBD) kind = "TBD"; else
         if (hdr8 == newkind_HSTM) kind = "HSTM"; else
         if (hdr8 == newkind_HSTL) kind = "HSTL"; else
         if (hdr8 == newkind_HSDA) kind = "HSDA"; else
         if (hdr8 == newkind_HSDB) kind = "HSDB"; else
         if (hdr8 == newkind_CTA) kind = "CTA"; else
         if (hdr8 == newkind_CTB) kind = "CTB"; else
         if (hdr8 == newkind_TEMP) kind = "TEMP"; else
         if (hdr8 == newkind_BAD) kind = "BAD"; else
         if (hdr9 == newkind_TTRM) kind = "TTRM"; else
         if (hdr9 == newkind_TTRL) kind = "TTRL"; else
         if (hdr9 == newkind_TTCM) kind = "TTCM"; else
         if (hdr9 == newkind_TTCL) kind = "TTCL"; else
         if (hdr7 == newkind_TMDR) {
            kind = "TMDR";
            unsigned mode = (msg >> 21) & 0xF;
            unsigned coarse = (msg >> 9) & 0xFFF;
            unsigned fine = msg & 0x1FF;
            bool isrising = (mode == 0) || (mode == 2);

            double localtm = ((lastepoch << 12) | coarse) * coarse_unit;
            if (fine > fine_max4)
               localtm -= coarse_unit;
            else if (fine > fine_min4)
               localtm -= (fine - fine_min4) / (0. + fine_max4 - fine_min4) * coarse_unit;

            if (isrising) localtm0 = localtm;

            if (onlych > 0) continue;

            snprintf(sdata, sizeof(sdata), "mode:0x%x coarse:%u fine:%u tm:%6.3fns", mode, coarse, fine, isrising ? localtm*1e9 : (localtm - localtm0)*1e9);
         }
      }

      if (prefix > 0) printf("%s%s %s\n", sbeg, kind, sdata);
   }
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


void PrintBubble(unsigned* bubble, unsigned len = 0) {
   // print in original order, time from right to left
   // for (unsigned d=BUBBLE_SIZE;d>0;d--) printf("%04x",bubble[d-1]);

   if (len == 0) len = BUBBLE_SIZE;
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

void PrintTdcData(unsigned ix, const std::vector<uint32_t> &data, unsigned prefix, unsigned &errmask)
{
   unsigned len = data.size();

   unsigned msg0 = data[0];
   if (((msg0 & tdckind_Mask) == tdckind_Header) && (((msg0 >> 24) & 0xF) == 0x4)) {
      PrintTdc4Data(ix, data, prefix);
      return;
   }

   unsigned wlen = len > 999 ? 4 : (len > 99 ? 3 : 2);

   unsigned long long epoch = 0;
   double tm, ch0tm = 0;

   errmask = 0;

   bool haschannel0 = false;
   unsigned channel = 0, maxch = 0, coarse = 0, fine = 0, ndebug = 0, nheader = 0, isrising = 0, dkind = 0, dvalue = 0, rawtime = 0;
   int epoch_channel = -11; // -11 no epoch, -1 - new epoch, 0..127 - epoch assigned with specified channel

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

   char sbuf[100], sfine[100], sbeg[100];
   unsigned calibr[2] = { 0xffff, 0xffff };
   unsigned skip = skip_msgs_in_tdc;
   int ncalibr = 2;
   const char* hdrkind = "";
   bool with_calibr = false, bad_fine = false;

   for (unsigned cnt = 0; cnt < len; cnt++, ix++) {
      auto msg = data[cnt];
      if (bubble_len >= 0) {
         bool israw = (msg & tdckind_Mask) == tdckind_Calibr;
         if (israw) {
            channel = (msg >> 22) & 0x7F;
            if (bubble_len == 0) { bubble_eix = bubble_ix = ix; bubble_ch = channel; }
            if (bubble_ch == channel) { bubble[bubble_len++] = msg & 0xFFFF; bubble_eix = ix; }
         }
         if ((bubble_len >= 100) || (cnt == len-1) || (channel!=bubble_ch) || (!israw && (bubble_len > 0))) {
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

      if (prefix > 0)
         snprintf(sbeg, sizeof(sbeg), "%*s[%*u] %08x ",  prefix, "", wlen, ix, msg);

      if (skip > 0) {
         skip--;
         continue;
      }

      if ((cnt==skip_msgs_in_tdc) && ((msg & tdckind_Mask) != tdckind_Header)) errmask |= tdcerr_MissHeader;

      switch (msg & tdckind_Mask) {
         case tdckind_Reserved:
            if (prefix>0) printf("%s tdc trailer ttyp:0x%01x rnd:0x%02x err:0x%04x\n", sbeg, (msg >> 24) & 0xF,  (msg >> 16) & 0xFF, msg & 0xFFFF);
            break;
         case tdckind_Header:
            nheader++;
            switch ((msg >> 24) & 0x0F) {
               case 0x01: hdrkind = "double edges"; break;
               case 0x0F: hdrkind = "bubbles"; bubble_len = 0; break;
               default: hdrkind = "normal"; break;
            }

            if (prefix > 0)
               printf("%s tdc header fmt:0x01%x hwtyp:0x%02x %s\n", sbeg, ((msg >> 24) & 0x0F), ((msg >> 8) & 0xFF), hdrkind);
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
               snprintf(sbuf, sizeof(sbuf), "  design 0x%08x %s", rawtime, ctime(&t));
               int len2 = strlen(sbuf);
               if (sbuf[len2-1] == 10) sbuf[len2-1] = 0;
            } else if (dkind == 0xE)
               snprintf(sbuf, sizeof(sbuf), " %3.1fC", dvalue/16.);

            if (prefix > 0)
               printf("%s tdc debug 0x%02x: 0x%06x %s%s\n", sbeg, dkind, dvalue, debug_name[dkind], sbuf);
            break;
         case tdckind_Epoch:
            epoch = msg & 0xFFFFFFF;
            tm = (epoch << 11) *5.;
            epoch_channel = -1; // indicate that we have new epoch
            if (prefix > 0) printf("%s epoch %u tm %6.3f ns\n", sbeg, msg & 0xFFFFFFF, tm);
            break;
         case tdckind_Calibr:
            calibr[0] = msg & 0x3fff;
            calibr[1] = (msg >> 14) & 0x3fff;
            if (use_calibr) ncalibr = 0;
            if ((prefix > 0) && (onlych < 0))
               printf("%s tdc calibr v1 0x%04x v2 0x%04x\n", sbeg, calibr[0], calibr[1]);
            break;
         case tdckind_Hit:
         case tdckind_Hit1:
         case tdckind_Hit2:
            channel = (msg >> 22) & 0x7F;
            if (channel == 0) haschannel0 = true;
            if (epoch_channel == -1) epoch_channel = channel;
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

            bad_fine = false;

            coarse = (msg & 0x7FF);
            fine = (msg >> 12) & 0x3FF;

            if (use_400mhz) {
               coarse = (coarse << 1) | ((fine & 0x200) ? 1 : 0);
               fine = fine & 0x1FF;
               bad_fine = (fine == 0x1ff);
               tm = ((epoch << 12) | coarse) * coarse_tmlen; // coarse time
            } else {
               bad_fine = (fine == 0x3ff);
               tm = ((epoch << 11) | coarse) * coarse_tmlen; // coarse time
            }

            with_calibr = false;
            if (!bad_fine) {
               if ((msg & tdckind_Mask) == tdckind_Hit2) {
                  if (isrising) {
                     tm -= fine*5e-3; // calibrated time, 5 ps/bin
                  } else {
                     tm -= (fine & 0x1FF)*10e-3; // for falling edge 10 ps binning is used
                     if (fine & 0x200) tm -= 0x800 * 5.; // in rare case time correction leads to epoch overflow
                  }
                  with_calibr = true;
               } else if (ncalibr < 2) {
                  // calibrated time, 5 ns correspond to value 0x3ffe or about 0.30521 ps/bin
                  unsigned raw_corr = calibr[ncalibr++];
                  if (raw_corr != 0x3fff) {
                     double corr = raw_corr*5./0x3ffe;
                     if (!isrising) corr*=10.; // for falling edge correction 50 ns range is used
                     tm -= corr;
                     with_calibr = true;
                  }
               } else {
                  tm -= coarse_tmlen * (fine > fine_min ? fine - fine_min : 0) / (0. + fine_max - fine_min); // simple approx of fine time from range 31-491
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

            if ((prefix > 0) && ((onlych < 0) || ((unsigned) onlych == channel)))
               printf("%s %s ch:%2u isrising:%u tc:0x%03x tf:%s tm:%6.3f ns%s\n",
                      sbeg, ((msg & tdckind_Mask) == tdckind_Hit) ? "hit " : (((msg & tdckind_Mask) == tdckind_Hit1) ? "hit1" : "hit2"),
                      channel, isrising, coarse, sfine, print_fulltime ? tm : tm - ch0tm, sbuf);
            if ((channel == 0) && (ch0tm == 0)) ch0tm = tm;
            if ((onlych >= 0) && (channel > (unsigned) onlych))
               cnt = len; // stop processing when higher channel number seen
            break;
         default:
            if (prefix > 0) printf("%s undefined\n", sbeg);
            break;
      }
   }

   if (len < 2) {
      if (nheader != 1)
         errmask |= tdcerr_NoData;
   } else if (!haschannel0 && (ndebug == 0) && (nbubble == 0))
      errmask |= tdcerr_MissCh0;

   for (unsigned n = 1; n < NumCh; n++)
      if ((num_leading[n] > 0) && (num_trailing[n] > 0))
         if (seq_err[n] || (num_leading[n] != num_trailing[n]))
            errmask |= tdcerr_Sequence;

   //if (substat) {
   //   if (substat->maxch < maxch) substat->maxch = maxch;
   //}
}

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
      PrintTdcData(0, data, strlen(prefix) + 3, errmask);
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

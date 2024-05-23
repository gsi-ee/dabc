
// related to TDC print

unsigned fine_min = 31, fine_max = 491, fine_min4 = 28, fine_max4 = 350, skip_msgs_in_tdc = 0, onlytdc = 0;
double tot_limit = 20., tot_shift = 20., coarse_tmlen = 5.;
bool use_calibr = true, epoch_per_channel = false, use_400mhz = false, print_fulltime = false, use_colors = true;
int onlych = -1;

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

unsigned BUBBLE_SIZE = 19;

enum TdcErrorsKind {
   tdcerr_MissHeader  = 0x0001,
   tdcerr_MissCh0     = 0x0002,
   tdcerr_MissEpoch   = 0x0004,
   tdcerr_NoData      = 0x0008,
   tdcerr_Sequence    = 0x0010,
   tdcerr_ToT         = 0x0020
};

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

const char *col_RESET   = "\033[0m";
const char *col_BLACK   = "\033[30m";      /* Black */
const char *col_RED     = "\033[31m";      /* Red */
const char *col_GREEN   = "\033[32m";      /* Green */
const char *col_YELLOW  = "\033[33m";      /* Yellow */
const char *col_BLUE    = "\033[34m";      /* Blue */
const char *col_MAGENTA = "\033[35m";      /* Magenta */
const char *col_CYAN    = "\033[36m";      /* Cyan */
const char *col_WHITE   = "\033[37m";      /* White */

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

unsigned PrintTdc4DataPlain(unsigned ix, const std::vector<uint32_t> &data, unsigned prefix)
{
   unsigned len = data.size();

   unsigned wlen = len > 999 ? 4 : (len > 99 ? 3 : 2);

   unsigned ttype = 0;
   uint64_t lastepoch = 0;
   double coarse_unit = 1./2.8e8;
   double localtm0 = 0.;
   unsigned maxch = 0;

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

   return maxch;
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

unsigned PrintTdcDataPlain(unsigned ix, const std::vector<uint32_t> &data, unsigned prefix, unsigned &errmask)
{
   unsigned len = data.size();
   errmask = 0;

   unsigned msg0 = data[0];
   if (((msg0 & tdckind_Mask) == tdckind_Header) && (((msg0 >> 24) & 0xF) == 0x4))
      return PrintTdc4DataPlain(ix, data, prefix);

   unsigned wlen = len > 999 ? 4 : (len > 99 ? 3 : 2);

   unsigned long long epoch = 0;
   double tm, ch0tm = 0;

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

   return maxch;
}

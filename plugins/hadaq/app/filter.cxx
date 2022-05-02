#include "hadaq/HadaqTypeDefs.h"

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

static int debug_cnt = 0;

double coarse_tmlen = 5.;

bool use_400mhz = false;

unsigned fine_min = 20, fine_max = 500;

/** check if it is hub */
bool is_hub(unsigned id)
{
   return id == 0xFF99;
}

bool is_tdc(unsigned id)
{
   return (id >= 0x1300) && (id < 0x1500);
}

bool scan_tdc(hadaq::RawSubevent* sub, unsigned tdcid, unsigned ix, unsigned len)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());
   if (ix >= sz) return false;
   if ((len == 0) || (ix + len > sz)) len = sz - ix;

   int nheader = 0;

   unsigned epoch = 0, channel = 0, coarse = 0, fine = 0, numhit = 0;
   double tm = 0.;

   int ncalibr = 2;
   unsigned calibr[2];
   bool use_calibr = false, isrising = true, bad_fine = false, haschannel0 = false, with_calibr = false;

   for (unsigned cnt=0;cnt<len;cnt++,ix++) {
      unsigned msg = sub->Data(ix);

      switch (msg & tdckind_Mask) {
         case tdckind_Reserved:
            // if (prefix > 0) printf("%s tdc trailer ttyp:0x%01x rnd:0x%02x err:0x%04x\n", sbeg, (msg >> 24) & 0xF,  (msg >> 16) & 0xFF, msg & 0xFFFF);
            break;
         case tdckind_Header:
            nheader++;
            break;
         case tdckind_Debug:
            break;
         case tdckind_Epoch:
            epoch = msg & 0xFFFFFFF;
            tm = (epoch << 11) *5.;
            break;
         case tdckind_Calibr:
            calibr[0] = msg & 0x3fff;
            calibr[1] = (msg >> 14) & 0x3fff;
            if (use_calibr) ncalibr = 0;
            break;
         case tdckind_Hit:
         case tdckind_Hit1:
         case tdckind_Hit2:

            numhit++;
            channel = (msg >> 22) & 0x7F;
            if (channel == 0) haschannel0 = true;
            isrising = (msg >> 11) & 0x1;

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

            break;
         default:
            break;
      }

   }

   if (debug_cnt < 10)
      printf("TDC 0x%04x numhits %u\n", tdcid, numhit);


   // suppress compiler warnings
   (void) haschannel0;
   (void) with_calibr;

   return true;
}

extern "C" bool filter_func(hadaq::RawEvent *evnt)
{
   debug_cnt++;

   int nsub = 0, ntdc = 0;

   bool accept = true;

   hadaq::RawSubevent* sub = nullptr;
   while ((sub = evnt->NextSubevent(sub)) != nullptr) {
      nsub++;

      unsigned trbSubEvSize = sub->GetSize() / 4 - 4,
               ix = 0, maxhublen = 0, lasthubid = 0,
               data, datalen, datakind;

      while (ix < trbSubEvSize) {
         data = sub->Data(ix++);
         datalen = (data >> 16) & 0xFFFF;
         datakind = data & 0xFFFF;

         if (maxhublen > 0) {
            if (datalen >= maxhublen) {
               printf("wrong format within hub 0x%04x, want size 0x%04x", lasthubid, datalen);
               datalen = maxhublen - 1;
            }
            maxhublen -= (datalen+1);
         } else {
            lasthubid = 0;
         }

         if ((maxhublen == 0) && is_hub(datakind)) {
            maxhublen = datalen;
            lasthubid = datakind;
            continue;
         }

         if (is_tdc(datakind)) {
            ntdc++;

            if (!scan_tdc(sub, datakind, ix, datalen)) accept = false;
         }

         ix += datalen;
      }

   }

   if (debug_cnt < 10) printf("Did filtering numsub %d numtdc %d\n", nsub, ntdc);

   return accept;
}

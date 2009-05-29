//============================================================================
// Name        : rocsrc/Calibrate.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/Calibrate.h"

#include "roc/defines.h"
#include "dabc/logging.h"

#include "roc/UdpBoard.h"

#include <time.h>
#include <sys/time.h>

#include <stdexcept>

//-------------------------------------------------------------------------------
roc::Calibrate::Calibrate(roc::Board* board) : Peripheral(board)
{
   fFEB_Number = 0;
   fNX_number = 0;
   fLTS_Delay = 512;
   fsec = 0;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
roc::Calibrate::~Calibrate()
{
}
//-------------------------------------------------------------------------------


void roc::Calibrate::addFEB(roc::FEBboard* _feb)
{
   _febs.push_back(_feb);
}

roc::FEBboard& roc::Calibrate::FEB(int n)
{
   if ((n>=(int)_febs.size()) || (n<0)) throw std::out_of_range("FATAL ERROR: You tried to access a FEB Number, that is not available!\r\n");
   return *_febs[n];
}


//-------------------------------------------------------------------------------
void roc::Calibrate::my_start(void)
{
   struct timeval t;
   gettimeofday(&t, NULL);

   fsec = (t.tv_sec);
}
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
unsigned long int roc::Calibrate::my_stop(void)
{
   struct timeval t;
   gettimeofday(&t, NULL);

   return (t.tv_sec - fsec);
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
int roc::Calibrate::gui_prepare(int feb, int nx_number)
{
   fNX_number = nx_number;
   fFEB_Number = feb;

   for (int i=0; i<=15; i++) {
       FEB(feb).NX(nx_number).I2C().setRegister(i, 0xFF);
   }
   FEB(feb).NX(nx_number).I2C().setRegister(3, 0x0F);

   brd().setLTS_Delay(fLTS_Delay);

   roc::UdpBoard* udp = dynamic_cast<roc::UdpBoard*> (fBoard);
   if (udp) udp->BURST(0);
   return 0;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
int roc::Calibrate::prepare(int feb, int nx_number)
{  int i;

   if ((nx_number < 0) || (nx_number > 3)) return -1;
   if ((feb < 0) || (feb > 1)) return -1;

   fNX_number = nx_number;
   fFEB_Number = feb;

   brd().setLTS_Delay(fLTS_Delay);

   fBoard->GPIO_setActive(0, 0, 0, 0);

   if (verbose()) DOUT0(("Preparing NX %d ...", nx_number));

   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==0) brd().NX_setActive(1, 0, 0, 0);
   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==1) brd().NX_setActive(0, 1, 0, 0);
   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==2) brd().NX_setActive(0, 0, 1, 0);
   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==3) brd().NX_setActive(0, 0, 0, 1);

   FEB(feb).NX(nx_number).setChannelDelay(8);

   roc::UdpBoard* udp = dynamic_cast<roc::UdpBoard*> (fBoard);
   if (udp) udp->BURST(0);

   FEB(feb).NX(nx_number).I2C().setRegister(16,160);
   FEB(feb).NX(nx_number).I2C().setRegister(17,255);
   FEB(feb).NX(nx_number).I2C().setRegister(18,35);
   FEB(feb).NX(nx_number).I2C().setRegister(19,6);
   FEB(feb).NX(nx_number).I2C().setRegister(20,95);
   FEB(feb).NX(nx_number).I2C().setRegister(21,87);
   FEB(feb).NX(nx_number).I2C().setRegister(22,100);
   FEB(feb).NX(nx_number).I2C().setRegister(23,137);
   FEB(feb).NX(nx_number).I2C().setRegister(24,255);
   FEB(feb).NX(nx_number).I2C().setRegister(25,69);
   FEB(feb).NX(nx_number).I2C().setRegister(26,15);
   FEB(feb).NX(nx_number).I2C().setRegister(27,54);
   FEB(feb).NX(nx_number).I2C().setRegister(28,92);
   FEB(feb).NX(nx_number).I2C().setRegister(29,69);
   FEB(feb).NX(nx_number).I2C().setRegister(30,30);
   FEB(feb).NX(nx_number).I2C().setRegister(31,31);
   FEB(feb).NX(nx_number).I2C().setRegister(32, 0);
   FEB(feb).NX(nx_number).I2C().setRegister(33, 11);

   for (i=0; i<=15; i++) {
       FEB(feb).NX(nx_number).I2C().setRegister(i, 0xFF);
   }
   FEB(feb).NX(nx_number).I2C().setRegister(3, 0x0F);

   brd().NX_reset();
   brd().reset_FIFO();

   if (verbose()) DOUT0(("Ready for TestPulse..."));

   return 0;
}
//-------------------------------------------------------------------------------





//-------------------------------------------------------------------------------
int roc::Calibrate::autolatency(int range_min, int range_max)
{
   int latency, shift, sr_init, bufg, deadcount, ok;
   int avg, max, maxl(0), maxs(0), maxb(0), maxn, maxo, invalid(0), allinvalid(0);
   uint32_t nx, adc, id;
   int anz;
   nxyter::Data data;

   DOUT0(("Autolatency..."));

   max=0xFFFF;
   for (latency = range_min*8; latency <= range_max*8; latency+=8){
      for (shift = 0; shift <= 15; shift++){
         for (bufg = 0; bufg <= 1; bufg++){
            sr_init = 255;
            sr_init = sr_init << shift;
            sr_init += sr_init >> 16;
            sr_init &= 0xffff;

            FEB(fFEB_Number).ADC().setSR_INIT(sr_init);
            FEB(fFEB_Number).ADC().setBUFG(bufg);
            FEB(fFEB_Number).ADC().setChannelLatency(FEB(fFEB_Number).NX(fNX_number).getNumber()+1, latency);

            avg = 0;
            anz = 0;
            deadcount=0;
            ok = 0;
            invalid=0;

            FEB(fFEB_Number).NX(fNX_number).I2C().setRegister(32, 1);
            brd().reset_FIFO();

            Start_DAQ();
            brd().TestPulse (2000, 5000);

            my_start();

            while (anz < 2500) {

                if (fBoard->getNextData(data, 0.1)&&data.isHitMsg()) {
                   adc = data.getAdcValue();
                   nx = data.getNxNumber();
                   id = data.getId();
                   if (((int) nx == FEB(fFEB_Number).NX(fNX_number).getNumber()) && (id==31)) {
                     avg+=adc;
                     anz++;
                   } else {
                     invalid++;
                     allinvalid++;
                     //printf("invalid -- NX:%d\tID:%d\r\n", nx, id);
                   }
                } else {
                   //printf("timeout\r\n");
                }

                if (my_stop()>=5) {
                   EOUT(("ERROR: Not enough Testpulse data! (Recieved %d values)", anz));
                   Stop_DAQ();
                   return -1;
                }
            }

            Stop_DAQ();

            if (anz>0) avg = avg / anz;
            else avg = 0;
            if (verbose()) DOUT0(("%d --- %d, %d, %d --- avg: %X (invalid: % d)", anz, latency, shift, bufg, avg, invalid));
            if (avg<max) {
               max=avg;
               maxl = latency;
               maxs = shift;
               maxb = bufg;
            }
         }
      }
   }
   if (verbose()) DOUT0(("Measured a minimum ADC value of 0x%X, using a latency of %d, a shift of %d and bufg_sel of %d", max, maxl, maxs, maxb));
   if (allinvalid>64000) DOUT0(("WARNING: Due to the high noise (~ %d%%) these values are not very significant!", 100*allinvalid/(2500*32*(range_max-range_min+1))));
   else DOUT0(("INFO: Noise ~%d%%", 100*allinvalid/(2500*32*(range_max-range_min+1))));
   if (verbose()) DOUT0((""));

   FEB(fFEB_Number).ADC().setSHIFT((maxs<<1)+maxb);



   if (verbose()) DOUT0(("Edge detection..."));
   maxo=0xFFFF; maxn=0xFFFF;
   for (latency = maxl; (maxn <= maxo + 200) && (latency>=0); latency--){
      FEB(fFEB_Number).ADC().setChannelLatency(FEB(fFEB_Number).NX(fNX_number).getNumber()+1, latency);
      avg = 0;
      anz = 0;
      deadcount=0;
      ok = 0;

      FEB(fFEB_Number).NX(fNX_number).I2C().setRegister(32, 1);
      brd().reset_FIFO();

      Start_DAQ();
      brd().TestPulse (2000, 5000);

      my_start();

      while (anz < 500) {

          if (fBoard->getNextData(data, 0.1)&&data.isHitMsg()) {
             adc = data.getAdcValue();
             nx = data.getNxNumber();
             id = data.getId();
             if (((int) nx == FEB(fFEB_Number).NX(fNX_number).getNumber()) && (id==31)) {
               avg+=adc;
               anz++;
             } else {
               invalid++;
               //printf("invalid -- NX:%d\tID:%d\r\n", nx, id);
             }
          } else {
             //printf("timeout\r\n");
          }
          if (my_stop()>=5) {
             EOUT(("ERROR: Not enough Testpulse data!"));
             return -1;
          }
      }
      Stop_DAQ();

      if (anz>0) avg = avg / anz;
      else avg = 0;
      if (verbose()) DOUT0(("%d --- %d --- avg: %X", anz, latency, avg));
      maxo=maxn;
      maxn=avg;
   }

   maxl = latency + 5;

   DOUT0(("Latency: %d", maxl));
   DOUT0(("sr_init: %d", maxs));
   DOUT0(("bufg   : %d", maxb));

   FEB(fFEB_Number).ADC().setChannelLatency(FEB(fFEB_Number).NX(fNX_number).getNumber()+1, maxl);
   if (verbose()) DOUT0(("OK.", maxl));

   return 0;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
int roc::Calibrate::autodelay()
{
   uint32_t deadcount, anz, min, max;
   uint32_t val, nx, id, ts, TS, low, high, delay;
   nxyter::Data data;

   FEB(fFEB_Number).NX(fNX_number).setChannelDelay(8);
   brd().NX_reset();

   FEB(fFEB_Number).NX(fNX_number).I2C().setRegister(32, 1);

   DOUT0(("Autodelay..."));
   if (verbose()) DOUT0(("Writing Pulse ..."));

   brd().TestPulse (2000, 5000);

   min=fLTS_Delay;
   anz=0;
   deadcount=0;

   if (verbose()) DOUT0(("Receiving Data ..."));

   my_start();

   while (anz<500) {

       if (brd().getFIFO_empty()) break;

        brd().get(ROC_ADC_DATA, val);
        brd().get(ROC_NX_DATA, nx);
        brd().get(ROC_LT_LOW, low);
        brd().get(ROC_LT_HIGH, high);
        brd().get(ROC_AUX_DATA_LOW, val);
        brd().get(ROC_AUX_DATA_HIGH, val);

        id = (nx & 0x00007F00)>>8;
        ts = ((nx & 0x7F000000)>>17)+((nx & 0x007F0000)>>16);
        TS = ungray(ts^(0x3FFF), 14);
        delay = (((low>>3)&0x1FF) - (TS>>5)) & 0x1FF;

        if (((nx & 0x80000000)>>31) && (((nx & 0x18)>>3)== (uint32_t) FEB(fFEB_Number).NX(fNX_number).getNumber()) && (ungray(id, 7)==31)) {
          if (delay < min) min = delay;
          anz++;
       } else {
          //printf("invalid -- DV:%d\tNX:%d\tID:%d\r\n", (nx & 0x80000000)>>31, (nx & 0x18)>>3, ungray(id, 7));
       }
       if (my_stop()>=5) {
          EOUT(("ERROR: Not enough Testpulse data!"));
          return -1;
       }
   }

   if (verbose()) DOUT0(("Measured Values: %d", anz));

   if (anz < 500) {
      EOUT(("ERROR: NOT ENOUGH DATA!\r\nAutodelay expects min. 500 Samples.", anz));
      return -1;
   }

   if (verbose()) DOUT0(("Minimum Delay: %d", min));

   max = fLTS_Delay - min;

   delay = max*8 + 16;

   FEB(fFEB_Number).NX(fNX_number).setChannelDelay(delay);
   brd().NX_reset();

   DOUT0(("Set new delay of %d.", delay));
   if (verbose()) DOUT0(("OK."));

   return 0;

}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
int roc::Calibrate::Start_DAQ()
{
   nxyter::Data data;

   //Start DAQ:
   if (!fBoard->startDaq(30)) {
       EOUT(("Start DAQ fails"));
       return -1;
   }

   my_start();
   while (my_stop()<5)
      if (fBoard->getNextData(data, 0.1))
        if (data.isStartDaqMsg()) break;

   if (my_stop()>=5) {
      EOUT(("Start DAQ acknowledge failed"));
      return -1;
   }

   return 0;
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
int roc::Calibrate::Stop_DAQ()
{
   nxyter::Data data;

   //Finish DAQ
   if (fBoard->stopDaq()) {
       return 0;
    } else {
       EOUT(("Stop DAQ fails"));
       return -1;
    }
    return 0;
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
uint32_t roc::Calibrate::ungray(int x, int n)
{
   int i;
   int b=0;
   uint32_t o=0;

   for (i=n; i>0; i--){
     b = b ^ ((x & (1<<(i-1)))>>(i-1));
     o = o | (b << (i-1));
   }

   return o;
}
//-------------------------------------------------------------------------------





































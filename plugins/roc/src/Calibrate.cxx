//============================================================================
// Name        : rocsrc/Calibrate.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/Calibrate.h"

#include "roc/UdpBoard.h"
#include "roc/defines.h"

#include <time.h>
#include <sys/time.h>

#include "dabc/logging.h"


#include <stdexcept>

#include <iostream>
using namespace std;


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
   if ((n>=_febs.size()) || (n<0)) throw std::out_of_range("FATAL ERROR: You tried to access a FEB Number, that is not available!\r\n");
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

   for (int i=0; i<=15; i++)
       FEB(feb).NX(nx_number).I2C().setRegister(i, 0xFF);

   FEB(feb).NX(nx_number).I2C().setRegister(3, 0x0F);

   brd().setLTS_Delay(fLTS_Delay);

   roc::UdpBoard* udp = dynamic_cast<roc::UdpBoard*> (getBoard());
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

   //fBoard->GPIO_setActive(0, 0, 0, 0);
   fBoard->GPIO_setCONFIG(0);

   if (verbose()) DOUT0(("Preparing NX %d ...\n", nx_number));

   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==0) brd().NX_setActive(1, 0, 0, 0);
   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==1) brd().NX_setActive(0, 1, 0, 0);
   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==2) brd().NX_setActive(0, 0, 1, 0);
   if (FEB(fFEB_Number).NX(fNX_number).getNumber()==3) brd().NX_setActive(0, 0, 0, 1);

   FEB(feb).NX(nx_number).setChannelDelay(8);

   roc::UdpBoard* udp = dynamic_cast<roc::UdpBoard*> (getBoard());
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

   for (i=0; i<=15; i++)
       FEB(feb).NX(nx_number).I2C().setRegister(i, 0xFF);

   FEB(feb).NX(nx_number).I2C().setRegister(3, 0x0F);

   brd().NX_reset();
   brd().reset_FIFO();

   if (verbose()) DOUT0(("Ready for TestPulse...\n"));

   return 0;
}
//-------------------------------------------------------------------------------





//-------------------------------------------------------------------------------
int roc::Calibrate::autolatency(int range_min, int range_max)
{
   int latency, shift, sr_init, bufg, deadcount, ok;
   int avg, max, maxl=0, maxs=0, maxb=0, maxn, maxo, invalid=0, allinvalid=0;
   uint32_t nx, adc, id;
   int anz;
   nxyter::Data data;

   cout << "Autolatency..." << endl;

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
                   if ((nx==FEB(fFEB_Number).NX(fNX_number).getNumber()) && (id==31)) {
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
                   printf("ERROR: Not enough Testpulse data! (Recieved %d values)\r\n", anz);
                   Stop_DAQ();
                   return -1;
                }
            }

            Stop_DAQ();

            if (anz>0) avg = avg / anz;
            else avg = 0;
            if (verbose()) DOUT0(("%d --- %d, %d, %d --- avg: %X (invalid: % d)\r\n", anz, latency, shift, bufg, avg, invalid));
            if (avg<max) {
               max=avg;
               maxl = latency;
               maxs = shift;
               maxb = bufg;
            }
         }
      }
   }
   if (verbose()) DOUT0(("Measured a minimum ADC value of 0x%X, using a latency of %d, a shift of %d and bufg_sel of %d\r\n", max, maxl, maxs, maxb));
   if (allinvalid>64000) DOUT0(("WARNING: Due to the high noise (~ %d%%) these values are not very significant!\r\n", 100*allinvalid/(2500*32*(range_max-range_min+1))));
   else DOUT0(("INFO: Noise ~%d%%\r\n", 100*allinvalid/(2500*32*(range_max-range_min+1))));
   if (verbose()) DOUT0(("\n"));

   FEB(fFEB_Number).ADC().setSHIFT((maxs<<1)+maxb);

   if (verbose()) DOUT0(("Edge detection...\r\n"));
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
             if ((nx==FEB(fFEB_Number).NX(fNX_number).getNumber()) && (id==31)) {
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
             printf("ERROR: Not enough Testpulse data!\r\n");
             return -1;
          }
      }
      Stop_DAQ();

      if (anz>0) avg = avg / anz;
      else avg = 0;
      if (verbose()) DOUT0(("%d --- %d --- avg: %X\r\n", anz, latency, avg));
      maxo=maxn;
      maxn=avg;
   }

   maxl = latency + 5;

   printf ("Latency: %d\r\n", maxl);
   printf ("sr_init: %d\r\n", maxs);
   printf ("bufg   : %d\r\n", maxb);

   FEB(fFEB_Number).ADC().setChannelLatency(FEB(fFEB_Number).NX(fNX_number).getNumber()+1, maxl);
   if (verbose()) DOUT0(("OK.\r\n", maxl));

   return 0;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
int roc::Calibrate::autodelay()
{
   uint32_t deadcount, anz, min, i, max;
   uint32_t val, nx, id, ts, TS, low, high, delay;
   nxyter::Data data;

   brd().DEBUG_MODE(1);
   FEB(fFEB_Number).NX(fNX_number).setChannelDelay(8);
   brd().NX_reset();

   FEB(fFEB_Number).NX(fNX_number).I2C().setRegister(32, 1);

   cout << "Autodelay..." << endl;
   if (verbose()) DOUT0(("Writing Pulse ...\n"));

   brd().TestPulse (2000, 5000);

   min=fLTS_Delay;
   anz=0;
   deadcount=0;

   if (verbose()) DOUT0(("Receiving Data ...\n"));

   my_start();

   while (anz<500) {

       brd().TestPulse (2000, 5000);

       if (brd().getFIFO_empty()) break;

        brd().get(ROC_ADC_DATA, val);
        brd().get(ROC_NX_DATA, nx);
        brd().get(ROC_LT_LOW, low);
        brd().get(ROC_LT_HIGH, high);

        //printf("read -- ADC:%X\tNX:%X\tLT%X.%X\r\n", val, nx, high, low);

        id = (nx & 0x00007F00)>>8;
        ts = ((nx & 0x7F000000)>>17)+((nx & 0x007F0000)>>16);
        TS = ungray(ts^(0x3FFF), 14);
        delay = (((low>>3)&0x1FF) - (TS>>5)) & 0x1FF;

        if (((nx & 0x80000000)>>31) && (((nx & 0x18)>>3)==FEB(fFEB_Number).NX(fNX_number).getNumber()) && (ungray(id, 7)==31)) {
          //printf("valid -- DV:%d\tNX:%d\tID:%d\tNX-TS:%X\tLTS:%X\tdelay:%d\r\n", (nx & 0x80000000)>>31, (nx & 0x18)>>3, ungray(id, 7), TS, low, delay);
          if (delay < min) min = delay;
          anz++;
       } else {
          //printf("invalid -- DV:%d\tNX:%d\tID:%d\r\n", (nx & 0x80000000)>>31, (nx & 0x18)>>3, ungray(id, 7));
       }
       if (my_stop()>=5) {
          printf("ERROR: Not enough Testpulse data!\r\n");
          brd().DEBUG_MODE(0);
          return -1;
       }
   }

   if (verbose()) DOUT0(("Measured Values: %d\r\n", anz));

   if (anz < 500) {
      printf("ERROR: NOT ENOUGH DATA!\r\nAutodelay expects min. 500 Samples.\r\n", anz);
      brd().DEBUG_MODE(0);
      return -1;
   }

   if (verbose()) DOUT0(("Minimum Delay: %d\r\n", min));

   max = fLTS_Delay - min;

   delay = max*8 + 16;

   brd().DEBUG_MODE(0);
   FEB(fFEB_Number).NX(fNX_number).setChannelDelay(delay);
   brd().NX_reset();

   printf("Set new delay of %d.\r\n", delay);
   if (verbose()) DOUT0(("OK.\r\n"));

   return 0;

}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
int roc::Calibrate::Start_DAQ() {
   nxyter::Data data;

   //Start DAQ:
   //cout << "starting readout..." << endl;
   if (!fBoard->startDaq(30)) {
       cout << "Start DAQ fails" << endl;
       return -1;
   }

   my_start();
   while (my_stop()<5)
      if (fBoard->getNextData(data, 0.1))
        if (data.isStartDaqMsg()) break;
   //cout << "Start DAQ Message received" << endl;

   if (my_stop()>=5) {
      printf("Start DAQ acknowledge failed");
      return -1;
   }

   return 0;
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
int roc::Calibrate::Stop_DAQ() {
   nxyter::Data data;

   //Finish DAQ
   if (fBoard->stopDaq()) {
       //cout << "Stop DAQ done" << endl;
       return 0;
    } else {
       cout << "Stop DAQ fails" << endl;
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





































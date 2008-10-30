#include "syscoreshell.h"

#include "sc_console.h"

/************************************************
 * private global variables
 ************************************************/

#ifndef BOOLEAN_DEF
#define BOOLEAN_DEF
   #define FALSE 0
   #define false 0
   #define TRUE 1
   #define true 1
#endif

/*******************************************************
 * private globals 
 *******************************************************/
struct sc_config gC = {
   //syscoreId = 
   0,
   //verbose
   0,
   //etherAddrSc[] 
   {0x02, 0xac, 0x3b, 0x33, 0x05, 0x71},
   //ipAddrSc[]
   {192, 168, 1, 131},
   //ipNetmask[]
   {255, 255, 255, 0},
   //ctrlPort
   ROC_DEFAULT_CTRLPORT,
   //dataPort
   ROC_DEFAULT_DATAPORT,
   //flushTimeout
   1000,   
   //burstLoopCnt
   100,
   // ocmLoopCnt
   300,
   //rocNumber
   1,
   //prefetchActive
   0,
   //activateLowLevel
   0,
   //rocLTSDelay
   512,
   //rocNX1Delay
   276,
   //rocNX2Delay
   276,
   //rocNX3Delay
   276,
   //rocNX4Delay
   276,
   //rocSrInit1
   0xff00,
   //rocBufgSelect1
   0,
   //rocSrInit2
   0xff00,
   //rocBufgSelect2
   0,
    //rocAdcLatency1
    65,
   //rocAdcLatency2
    65,
    //rocAdcLatency3
    65,
   //rocAdcLatency4
    65,
    //NX_Number
    0,
    //rocFEB4nx
    0,
    //global_consolemode
    0,
    //testpatternGenerator
    false
};

NX_type NX_attribute[4];

struct sc_data gD;

struct sc_statistic gCnt; // statistic 

char defConfigFileName[] = "/roc1.cfg";      

/************************************************
 * function implementations
 ************************************************/

int read_I2C(int i2cn, int reg)
{
   int r;
   
   if (i2cn==1) {
      XIo_Out32(I2C1_REGISTER, reg);
      r = XIo_In32(I2C1_DATA);
      if (XIo_In32(I2C1_ERROR)==1) sc_printf("\tI2C Timeout Error!\t");
   } else {
      XIo_Out32(I2C2_REGISTER, reg);
      r = XIo_In32(I2C2_DATA);
      if (XIo_In32(I2C2_ERROR)==1) sc_printf("\tI2C Timeout Error!\t");
   }
   return r;
}   

void NX(int reg, int val) 
{
   int nx = NX_attribute[gC.NX_Number].Connector;
   
   if (nx==1) {
      XIo_Out32(I2C1_REGISTER, reg); 
      XIo_Out32(I2C1_DATA, val);         
   } else {
      XIo_Out32(I2C2_REGISTER, reg); 
      XIo_Out32(I2C2_DATA, val);         
   }   
}

void set_nx (int nxn, int mode)
{
   if (nxn<4) {
      if (NX_attribute[nxn].active!=1) 
         sc_printf("Warning: NX = %d is set inactive.\r\n", nxn);
      gC.NX_Number = nxn;
      if (NX_attribute[gC.NX_Number].Connector==1) 
         XIo_Out32(I2C1_SLAVEADDR, NX_attribute[gC.NX_Number].I2C_SlaveAddr);
      else 
      if (NX_attribute[gC.NX_Number].Connector==2) 
         XIo_Out32(I2C2_SLAVEADDR, NX_attribute[gC.NX_Number].I2C_SlaveAddr);

      if (mode==1) {
         sc_printf("Changed NX to Number %d.\r\n", gC.NX_Number);
         sc_printf("Connector: %d\r\n", NX_attribute[gC.NX_Number].Connector);
         sc_printf("I2C_SlaveAddr: %d\r\n", NX_attribute[gC.NX_Number].I2C_SlaveAddr);
      }     
   } else {
      sc_printf("Error: %d Not a valid NX-Number.\r\n", nxn);
   }
}

void testsetup() 
{
   int i, nx;

   nx = NX_attribute[gC.NX_Number].Connector;

   NX(0, 0x0F);
    for (i=1; i<=15; i++) {
      NX(i, 0xFF);
   }
   
   NX(16,160); NX(17,255); 
   //NX(18,25);
   NX(19,6);
   NX(20,95);  NX(21,87); NX(22,100); NX(23,137); NX(24,255);
   NX(25,69);  NX(26,15); NX(27,54);  NX(28,92);  NX(29,69);
   NX(30,30);  NX(31,31); NX(32, 0);  NX(33, 11);
   
   for (i=0; i<=45; i++){
         read_I2C(nx, i);
   }      
}

unsigned long ungray(int x, int n)
{
   int i;
   int b=0;
   unsigned long o=0;
   
   for (i=n; i>0; i--){
     b = b ^ ((x & (1<<(i-1)))>>(i-1));
     o = o | (b << (i-1));
   }
   
   return o;
}


void shiftit(Xuint32 shift) 
{
   int con = NX_attribute[gC.NX_Number].Connector;
   
   Xuint32 sr_init;
   Xuint32 bufg_select;
   
   bufg_select = shift & 0x01;
   shift = shift >> 1;

   sr_init = 255;
   sr_init = sr_init << shift;   

   sr_init += sr_init >> 16;
   sr_init &= 0xffff;

   sc_printf ("SR_INIT:     %X\r\n", sr_init);
   sc_printf ("BUFG_SELECT: %X\r\n", bufg_select);

   if (con==1) {
     XIo_Out32(SR_INIT, sr_init);
     XIo_Out32(BUFG_SELECT, bufg_select);
     gC.rocSrInit1 = sr_init;
     gC.rocBufgSelect1 = bufg_select;
   } else if (con==2) {
     XIo_Out32(SR_INIT2, sr_init);
     XIo_Out32(BUFG_SELECT2, bufg_select);
     gC.rocSrInit2 = sr_init;
     gC.rocBufgSelect2 = bufg_select;
   }
}


int nx_autocalibration() 
{
   int latency, shift, sr_init, bufg, deadcount, ok;
   int i, avg, max, maxl=0, maxs=0, maxb=0, maxn, maxo;
   Xuint32 nx, adc, id, dead;
   int anz;
    int con = NX_attribute[gC.NX_Number].Connector;
   
    testsetup();
   
   NX(32, 0);
    for (i=0; i<=15; i++) {
      NX(i, 0xFF);
   }
    NX(3, 0x0F);

    XIo_Out32(TP_TP_DELAY, 2000 - 1);
    XIo_Out32(TP_COUNT, 5000 * 2);
      
   max=0xFFFF;
   for (latency = 6*8; latency <= 9*8; latency+=8){
      for (bufg = 0; bufg <= 1; bufg++){
          for (shift = 0; shift <= 15; shift++){
               sr_init = 255;
               sr_init = sr_init << shift;   
               sr_init += sr_init >> 16;
               sr_init &= 0xffff;
               
               if (con==1) {
                  XIo_Out32(SR_INIT, sr_init);
                  usleep(2000);
                  XIo_Out32(BUFG_SELECT, bufg);
                  usleep(2000);
               } else if (con==2) {
                  XIo_Out32(SR_INIT2, sr_init);
                  usleep(2000);
                  XIo_Out32(BUFG_SELECT2, bufg);
                  usleep(2000);
            }
            
            if (gC.NX_Number==0) {
                        XIo_Out32(ADC_LATENCY1, latency);
            } else if (gC.NX_Number==1) {
                        XIo_Out32(ADC_LATENCY2, latency);
            } else if (gC.NX_Number==2) {
                        XIo_Out32(ADC_LATENCY3, latency);
            } else if (gC.NX_Number==3) {
                        XIo_Out32(ADC_LATENCY4, latency);
            }
            avg = 0;
            anz = 0;
            deadcount=0;
            ok = 0;
            
            while (ok==0) {
                NX(32, 0);
                XIo_Out32(FIFO_RESET, 1);
                XIo_Out32(FIFO_RESET, 0);
                NX(32, 1);
                XIo_Out32(TP_START, 1);
                dead=0;
                while (!XIo_In32(NX_FIFO_FULL)&&(dead<1000000)) dead++;
                NX(32, 0);
                if (dead >= 1000000) {
                   sc_printf("!");
                   deadcount++;
                }   else {
                   ok=1;
                }                   
                if (deadcount>20) {
                   sc_printf("\r\nERROR: There is no Testpulse-Data!\r\n");
                   return 100;
                }
            }
            
            for (i=0; i<500; i++) {
                
                adc=XIo_In32(ADC_DATA);
                nx=XIo_In32(NX_DATA);
                id = (nx & 0x00007F00)>>8;                
                XIo_In32(LT_LOW); XIo_In32(LT_HIGH);
                XIo_In32(AUX_DATA_LOW); XIo_In32(AUX_DATA_HIGH);
                
                if (((nx & 0x80000000)>>31) && (((nx & 0x18)>>3)==gC.NX_Number) && (ungray(id, 7)==31)) {
                   avg+=adc;
                   anz++;
                } else {
                   sc_printf("invalid: %d", ungray(id, 7));
                }
             
            }
            if (anz>0) avg = avg / anz;
            else avg = 0;
            sc_printf ("%d --- %d, %d, %d --- avg: %X\r\n", anz, latency, shift, bufg, avg);
            if (avg<max) {
               max=avg;
               maxl = latency;
               maxs = shift;
               maxb = bufg;
            }
           }
      }
   }

   shiftit(maxs*2+maxb);
   sc_printf("Edge detection...\r\n");

   maxo=0xFFFF; maxn=0xFFFF;
   for (latency = maxl; maxn <= maxo + 30; latency--){
      if (gC.NX_Number==0) {
                XIo_Out32(ADC_LATENCY1, latency);
      } else if (gC.NX_Number==1) {
                XIo_Out32(ADC_LATENCY2, latency);
      } else if (gC.NX_Number==2) {
                XIo_Out32(ADC_LATENCY3, latency);
      } else if (gC.NX_Number==3) {
                XIo_Out32(ADC_LATENCY4, latency);
      }
      avg = 0;
      anz = 0;
      deadcount=0;
      ok = 0;
            
      while (ok==0) {
            NX(32, 0);
         XIo_Out32(FIFO_RESET, 1);
         XIo_Out32(FIFO_RESET, 0);
            NX(32, 1);
         XIo_Out32(TP_START, 1);
         dead=0;
         while (!XIo_In32(NX_FIFO_FULL)&&(dead<1000000)) dead++;
         NX(32, 0);
         if (dead >= 1000000) {
             sc_printf("!");
            deadcount++;
         } else {
                  ok=1;
         }
        }                   
      if (deadcount>20) {
         sc_printf("\r\nERROR: There is no Testpulse-Data!\r\n");
         return 100;
      }
         
      for (i=0; i<500; i++) {
                
          adc=XIo_In32(ADC_DATA);
          nx=XIo_In32(NX_DATA);
          id = (nx & 0x00007F00)>>8;                
          XIo_In32(LT_LOW); XIo_In32(LT_HIGH);
          XIo_In32(AUX_DATA_LOW); XIo_In32(AUX_DATA_HIGH);
                
          if (((nx & 0x80000000)>>31) && (((nx & 0x18)>>3)==gC.NX_Number) && (ungray(id, 7)==31)) {
             avg+=adc;
            anz++;
          } else {
             sc_printf("invalid: %d", ungray(id, 7));
          }
             
      }
      if (anz>0) avg = avg / anz;
      else avg = 0;
      sc_printf ("%d --- %d --- avg: %X\r\n", anz, latency, avg);
        maxo=maxn;
      maxn=avg;
   }

    maxl = latency + 5;

   sc_printf ("Latency: %d\r\n", maxl);
   sc_printf ("sr_init: %d\r\n", maxs);
   sc_printf ("bufg   : %d\r\n", maxb);
   sc_printf ("max-adc: %X\r\n", max);
   
   sc_printf ("\r\nSetting the latency to %d ... \r\n", maxl);
    if (gC.NX_Number==0) {
      XIo_Out32(ADC_LATENCY1, maxl);
      gC.rocAdcLatency1 = maxl;
   } else if (gC.NX_Number==1) {
      XIo_Out32(ADC_LATENCY2, maxl);
      gC.rocAdcLatency2 = maxl;
   } else if (gC.NX_Number==2) {
      XIo_Out32(ADC_LATENCY3, maxl);
      gC.rocAdcLatency3 = maxl;
   } else if (gC.NX_Number==3) {
      XIo_Out32(ADC_LATENCY4, maxl);
      gC.rocAdcLatency4 = maxl;
    }
   sc_printf ("OK.\r\n", maxl);
      
   sc_printf ("\r\nUsing the measured shift of %d ... \r\n", maxs*2+maxb);
    shiftit(maxs*2+maxb);

    XIo_Out32(FIFO_RESET, 1);
   XIo_Out32(FIFO_RESET, 0);
   
   return (maxs*2+maxb);
}

void decode(int mode)
{
    int j,pari;
    Xuint32 low, high, delay, TS, adc, burst1=0, burst2=0, burst3=0, data, ts, id;
    Xuint32 b_ts, b_id, b_roc_ts, b_nx, b_roc, b_type, b_adc, b_pileup, b_overf, b_epoch;
    Xuint32 b2_ts, b2_id, b2_roc_ts, b2_nx, b2_roc, b2_type, b2_adc, b2_pileup, b2_overf, b2_epoch;
    Xuint32 d_epoch, error;
    Xuint32 data2, adc2, low2, high2;
    Xuint32 ocm1, ocm2, ocm3;
    Xuint32 Epoch, State, Channel, ROC, lead;
    
     if (gC.prefetchActive==0) {
        burst1 = XIo_In32(BURST1);
        burst2 = XIo_In32(BURST2);
        burst3 = XIo_In32(BURST3);
     }
     
     data = XIo_In32(DEBUG_NX);
     adc = XIo_In32(DEBUG_ADC);
     low = XIo_In32(DEBUG_LTL);
     high = XIo_In32(DEBUG_LTH);
     data2 = XIo_In32(DEBUG_NX2);
     adc2 = XIo_In32(DEBUG_ADC2);
     low2 = XIo_In32(DEBUG_LTL2);
     high2 = XIo_In32(DEBUG_LTH2);

     if (gC.prefetchActive==0) {
        ocm1 = XIo_In32(OCM_REG1);
        ocm2 = XIo_In32(OCM_REG2);
        ocm3 = XIo_In32(OCM_REG3);
     } else {
        burst1 = XIo_In32(OCM_REG1);
        burst2 = XIo_In32(OCM_REG2);
        burst3 = XIo_In32(OCM_REG3);
     }     

     b_ts = (burst1 >> 7) & 0x3fff;
     b_id = burst1 & 0x7F;
     b_roc_ts = (burst1 >> 21) & 0x7;
     b_nx = (burst1 >> 24) & 0x3;
     b_roc = (burst1 >> 26) & 0x7;
     b_type = (burst1 >> 29);
     b_adc = (burst2 >> 19) & 0xFFF;
     b_pileup = (burst2 >> 18) & 0x1;
     b_overf = (burst2 >> 17) & 0x1;
     b_epoch = (burst2 >> 16) & 0x1;
    
     b2_ts = ((burst2 & 0x1f) << 9) | (burst3 >> 23);
     b2_id = (burst3>>16) & 0x7F;
     b2_roc_ts = (burst2 >> 5) & 0x7;
     b2_nx = (burst2 >> 8) & 0x3;
     b2_roc = (burst2 >> 10) & 0x7;
     b2_type = (burst2 >> 13) & 0x7;
     b2_adc = (burst3 >> 3) & 0xFFF;
     b2_pileup = (burst3 >> 2) & 0x1;
     b2_overf = (burst3 >> 1) & 0x1;
     b2_epoch = (burst3) & 0x1;
      
     //sc_printf("Burst1: %X, OCM1: %X\r\n", burst1, ocm1);
     //sc_printf("Burst2: %X, OCM2: %X\r\n", burst2, ocm2);
     //sc_printf("Burst3: %X, OCM3: %X\r\n", burst3, ocm3);
   
     if (b_type==1) {
          ts = ((data & 0x7F000000)>>17)+((data & 0x007F0000)>>16);
          TS = ungray(ts^(0x3FFF), 14);
          id = (data & 0x00007F00)>>8;
          delay = (((low>>3)&0x1FF) - (TS>>5)) & 0x1FF;
      
          error=0;
          d_epoch=0;
   
          if (((low>>3)&0x1FF) < (TS>>5)) {
             d_epoch=1;
          }    
   
          pari=0;
          for (j=0; j<=2; j++)   pari = (pari + ((data>>j) & 1)) & 1;
          for (j=8; j<=14; j++)   pari = (pari + ((data>>j) & 1)) & 1;
          for (j=16; j<=21; j++) pari = (pari + (1 ^ ((data>>j) & 1))) & 1;
          for (j=24; j<=30; j++) pari = (pari + (1 ^ ((data>>j) & 1))) & 1;

         if (pari != 0) {sc_printf ("Parity ERROR!\r\n"); error=1;}
         if (b_ts != TS) {sc_printf ("TS ERROR!\r\n"); error=1;}
         if (b_id != ungray(id, 7)) {sc_printf ("ID ERROR!\r\n"); error=1;}
         if (b_adc != adc) {sc_printf ("ADC ERROR!\r\n"); error=1;}
         if (b_pileup != (data & 0x4)>>2) {sc_printf ("PileUp ERROR!\r\n"); error=1;}
         if (b_overf != (data & 0x2)>>1) {sc_printf ("OverF ERROR!\r\n"); error=1;}
         if (b_epoch != d_epoch) {sc_printf("Epoch ERROR!\r\n"); error=1;}
   
         if ((((low>>3)&0x1FF) < (TS>>5)) || (mode==0) || (error==1)) {
           //if ((error==1)) {
          
             sc_printf("Burst:\r\n");
             sc_printf("TS: %X, ID: %X, ROC-TS: %X, NX: %X, ROC: %X, Type: %X\r\n", b_ts, b_id, b_roc_ts, b_nx, b_roc, b_type);
             sc_printf("ADC: %X, PileUp: %X, OverF: %X, Parity: %X, LastEpoch: %X\r\n", b_adc, b_pileup, b_overf, (data&1), b_epoch);
             sc_printf("Debug:\r\n");
   
             if (d_epoch==1) {
                sc_printf("!!!");
                if (low<=0x800) high--;
                low-=0x800;
             }    

             sc_printf("Data: %X   --   ", data);
             sc_printf("DV: %d, ", 1);
             
             sc_printf("NX: %d, ", (data & 0x18)>>3);
             sc_printf("ID: %d, ", ungray(id, 7));
             sc_printf("TS: %X, TS2: %X, ", TS, (TS>>5));
             sc_printf("PileUp: %X, ", (data & 0x00000004)>>2);
             sc_printf("OverF : %X, ", (data & 0x00000002)>>1);
             sc_printf("Parity: %X\r\n", (data & 0x00000001));
             
             sc_printf("ADC: %X,   ", adc);
             sc_printf("LT: %X.%X, Epoch: %X.%X, LTS: %X, ", high, low, high>>12, ((high&0x7FF)<<20) + (low>>12), (low>>3)&0x1FF);
             sc_printf("FT: %X, Delay: %d\r\n", low&0x7, delay);
            
             sc_printf("SEND:    Epoch: %X.%X, TS: %X\r\n\n",  high>>12, ((high&0x7FF)<<20) + (low>>12), TS);
          }
         
         } else if (b_type==2) {
                sc_printf("EPOCHE: %X\r\n\r\n", ((burst1 & 0xFFFFFF)<<8) + ((burst2>>24) & 0xFF));
       } else if (b_type==7) {
           if (((burst1&0x00FF0000)>>16)==3){
              ts = ((data & 0x7F000000)>>17)+((data & 0x007F0000)>>16);
              TS = ungray(ts^(0x3FFF), 14);
              id = (data & 0x00007F00)>>8;
              delay = (((low>>3)&0x1FF) - (TS>>5)) & 0x1FF;
                
              pari=0;
              for (j=0; j<=2; j++)   pari = (pari + ((data>>j) & 1)) & 1;
                 for (j=8; j<=14; j++)   pari = (pari + ((data>>j) & 1)) & 1;
                 for (j=16; j<=21; j++) pari = (pari + (1 ^ ((data>>j) & 1))) & 1;
                 for (j=24; j<=30; j++) pari = (pari + (1 ^ ((data>>j) & 1))) & 1;
              sc_printf("HW Detected Parity Error!\r\n");
              if (pari==0) sc_printf("This is wrong!!!\r\n");
              
               sc_printf("Data: %X   --   ", data);
              sc_printf("DV: %d, ", 1);
      
              sc_printf("NX: %d, ", (data & 0x18)>>3);
              sc_printf("ID: %d, ", ungray(id, 7));
              sc_printf("TS: %X, ", TS);
              sc_printf("PileUp: %X, ", (data & 0x00000004)>>2);
              sc_printf("OverF : %X, ", (data & 0x00000002)>>1);
              sc_printf("Parity: %X\r\n", (data & 0x00000001));
              
              sc_printf("ADC: %X,   ", adc);
              sc_printf("LT: %X.%X, Epoch: %X.%X, LTS: %X, ", high, low, high>>12, ((high&0x7FF)<<20) + (low>>12), (low>>3)&0x1FF);
              sc_printf("Delay: %d\r\n\n", delay);
           } else if (((burst1&0x00FF0000)>>16)==4){
                  sc_printf("Synch Parity Error!\r\n\n");
           } else {
                  sc_printf("Unknown System Message!\r\n\n");
           }
         } else if (b_type==3) {
            ROC = (burst1>>26)&0x7;
            Channel = (burst1>>24)&0x3;
            TS = ((burst1>>10)&0x3fff);
            Epoch = ((burst1&0x3ff)<<14)|(burst2>>18);
            State = (burst2>>16)&0x3;
            sc_printf("Synch -- ROC: %X, Channel: %X, TS: %X, Epoch: %X, State: %X\r\n", ROC, Channel, TS, Epoch, State);
            sc_printf("Synch-TS: %X.%X\r\n\n", high, low);
          } else if (b_type==4) {
            ROC = (burst1>>26)&0x7;
            Channel = (burst1>>19)&0x7F;
            TS = (burst1>>5)&0x3fff;
            lead = (burst1>>4)&0x1;
            sc_printf("AUX -- ROC: %X, Channel: %X, TS: %X, Leading: %X\r\n", ROC, Channel, TS, lead);
            sc_printf("AUX-TS: %X.%X\r\n\n", high, low);
         } else {
         sc_printf("Data: %X   --   ", data);
         sc_printf("NOT VALID.\r\n");
       }    


    if (b2_type==1) {
          ts = ((data2 & 0x7F000000)>>17)+((data2 & 0x007F0000)>>16);
          TS = ungray(ts^(0x3FFF), 14);
          id = (data2 & 0x00007F00)>>8;
          delay = (((low2>>3)&0x1FF) - (TS>>5)) & 0x1FF;
   
          error=0;
          d_epoch=0;
   
          if (((low2>>3)&0x1FF) < (TS>>5)) {
             d_epoch=1;
          }    

          pari=0;
          for (j=0; j<=2; j++)   pari = (pari + ((data2>>j) & 1)) & 1;
          for (j=8; j<=14; j++)   pari = (pari + ((data2>>j) & 1)) & 1;
          for (j=16; j<=21; j++) pari = (pari + (1 ^ ((data2>>j) & 1))) & 1;
          for (j=24; j<=30; j++) pari = (pari + (1 ^ ((data2>>j) & 1))) & 1;
     
          if (pari != 0) {sc_printf ("Parity ERROR!\r\n"); error=1;}
          if (b2_ts != TS) {sc_printf ("TS ERROR!\r\n"); error=1;}
          if (b2_id != ungray(id, 7)) {sc_printf ("ID ERROR!\r\n"); error=1;}
          if (b2_adc != adc2) {sc_printf ("ADC ERROR!\r\n"); error=1;}
          if (b2_pileup != (data2 & 0x4)>>2) {sc_printf ("PileUp ERROR!\r\n"); error=1;}
          if (b2_overf != (data2 & 0x2)>>1) {sc_printf ("OverF ERROR!\r\n"); error=1;}
          if (b2_epoch != d_epoch) {sc_printf("Epoch ERROR!\r\n"); error=1;}
   
          if ((((low2>>3)&0x1FF) < (TS>>5)) || (mode==0) || (error==1)) {
          //if ((error==1)) {
   
             sc_printf("Burst:\r\n");
             sc_printf("TS: %X, ID: %X, ROC-TS: %X, NX: %X, ROC: %X, Type: %X\r\n", b2_ts, b2_id, b2_roc_ts, b2_nx, b2_roc, b2_type);
             sc_printf("ADC: %X, PileUp: %X, OverF: %X, Parity: %X, LastEpoch: %X\r\n", b2_adc, b2_pileup, b2_overf, (data2&1), b2_epoch);
             
             sc_printf("Debug:\r\n");
   
             if (d_epoch==1) {
                sc_printf("!!!");
                if (low2<=0x800) high2--;
                low2-=0x800;
             }    

             sc_printf("Data: %X   --   ", data2);
             sc_printf("DV: %d, ", 1);
      
             sc_printf("NX: %d, ", (data2 & 0x18)>>3);
             sc_printf("ID: %d, ", ungray(id, 7));
             sc_printf("TS: %X, TS2: %X, ", TS, (TS>>5));
             sc_printf("PileUp: %X, ", (data2 & 0x00000004)>>2);
             sc_printf("OverF : %X, ", (data2 & 0x00000002)>>1);
             sc_printf("Parity: %X\r\n", (data2 & 0x00000001));
             
             sc_printf("ADC: %X,   ", adc2);
             sc_printf("LT: %X.%X, Epoch: %X.%X, LTS: %X, ", high2, low2, high2>>12, ((high2&0x7FF)<<20) + (low2>>12), (low2>>3)&0x1FF);
             sc_printf("FT: %X, Delay: %d\r\n\r\n", low2&0x7, delay);
           
             sc_printf("SEND:    Epoch: %X.%X, TS: %X\r\n\n",  high2>>12, ((high2&0x7FF)<<20) + (low2>>12), TS);
          }    

      } else if (b2_type==2)  {
                sc_printf("EPOCHE: %X\r\n\n", ((burst2 & 0xFF)<<24) + (burst3>>8));
     } else if (b2_type==7) {
           if (((burst2&0x000000FF))==3){
              ts = ((data2 & 0x7F000000)>>17)+((data2 & 0x007F0000)>>16);
              TS = ungray(ts^(0x3FFF), 14);
              id = (data2 & 0x00007F00)>>8;
              delay = (((low2>>3)&0x1FF) - (TS>>5)) & 0x1FF;

                 pari=0;
              for (j=0; j<=2; j++)   pari = (pari + ((data2>>j) & 1)) & 1;
                 for (j=8; j<=14; j++)   pari = (pari + ((data2>>j) & 1)) & 1;
                 for (j=16; j<=21; j++) pari = (pari + (1 ^ ((data2>>j) & 1))) & 1;
                 for (j=24; j<=30; j++) pari = (pari + (1 ^ ((data2>>j) & 1))) & 1;
              sc_printf("HW Detected Parity Error!\r\n");
              if (pari==0) sc_printf("This is wrong!!!\r\n");

              sc_printf("Data: %X   --   ", data2);
              sc_printf("DV: %d, ", 1);
      
              sc_printf("NX: %d, ", (data2 & 0x18)>>3);
              sc_printf("ID: %d, ", ungray(id, 7));
              sc_printf("TS: %X, ", TS);
              sc_printf("PileUp: %X, ", (data2 & 0x00000004)>>2);
              sc_printf("OverF : %X, ", (data2 & 0x00000002)>>1);
              sc_printf("Parity: %X\r\n", (data2 & 0x00000001));
              
              sc_printf("ADC: %X,   ", adc2);
              sc_printf("LT: %X.%X, Epoch: %X.%X, LTS: %X, ", high2, low2, high2>>12, ((high2&0x7FF)<<20) + (low2>>12), (low2>>3)&0x1FF);
              sc_printf("Delay: %d\r\n\n", delay);
           } else if (((burst2&0x000000FF))==4){
                  sc_printf("Synch Parity Error!\r\n\n");
           }     
           else sc_printf("Unknown System Message!");
         } else if (b2_type==3) {
            ROC = (burst2>>10)&0x7;
            Channel = (burst2>>8)&0x3;
            TS = ((burst2&0xFF)<<6)|((burst3>>26)&0x3f);
            Epoch = (burst3>>2)&0xffffff;
            State = (burst3&0x3);
            sc_printf("Synch -- ROC: %X, Channel: %X, TS: %X, Epoch: %X, State: %X\r\n", ROC, Channel, TS, Epoch, State);
            sc_printf("Synch-TS: %X.%X\r\n\n", high2, low2);
          } else if (b2_type==4) {
            ROC = (burst2>>10)&0x7;
            Channel = (burst2>>3)&0x7F;
            TS = ((burst2&0x7)<<11)|((burst3>>21)&0x7ff);
            lead = (burst3>>20)&0x1;
            sc_printf("AUX -- ROC: %X, Channel: %X, TS: %X, Leading: %X\r\n", ROC, Channel, TS, lead);
            sc_printf("AUX-TS: %X.%X\r\n\n", high2, low2);
           } else {
         sc_printf("\r\n\r\nData: %X   --   ", data);
         sc_printf("Burst: %X, %X --", (burst2 & 0xFFFF), burst3);
         sc_printf("NOT VALID.\r\n");
       }    
}

void nx_reset()
{
   int i, nxn, mem;
   
   XIo_Out32(NX_INIT, 0);

   XIo_Out32(I2C1_RESET, 0);                   
   XIo_Out32(I2C1_REGRESET, 0);            
   XIo_Out32(I2C2_RESET, 0);                   
   XIo_Out32(I2C2_REGRESET, 0);            
   usleep(200);
   XIo_Out32(I2C1_RESET, 1);                   
   XIo_Out32(I2C1_REGRESET, 1);                
   XIo_Out32(I2C2_RESET, 1);                   
   XIo_Out32(I2C2_REGRESET, 1);                
   usleep(200);

   mem = gC.NX_Number;

   for (nxn=0; nxn<4; nxn++) if (NX_attribute[nxn].active==1) {   
      
      sc_printf("Please wait for Reset of NX %d...", nxn);
   
      set_nx(nxn, 0);
      
      for (i=0; i<=15; i++) {
         NX(i, 0x00);
      }
   
      NX(16,160); NX(17,255); NX(18,25); NX(19,6);
      NX(20,95);  NX(21,87); NX(22,100); NX(23,137); NX(24,255);
      NX(25,69);  NX(26,15); NX(27,54);  NX(28,92);  NX(29,69);
      NX(30,30);  NX(31,31); NX(32, 0);  NX(33, 11);
      
      for (i=0; i<=129; i++){
          if (NX_attribute[nxn].Connector==1) {
            XIo_Out32(I2C1_REGISTER, 42);
            XIo_Out32(I2C1_DATA, 0);
          } else if (NX_attribute[nxn].Connector==2) {
            XIo_Out32(I2C2_REGISTER, 42);
            XIo_Out32(I2C2_DATA, 0);
          }
      }      
      
      for (i=0; i<=45; i++){
           read_I2C(NX_attribute[nxn].Connector, i);
      }      

      sc_printf("OK.\r\n");
   }

   set_nx(mem, 0);   
   
   XIo_Out32(NX_RESET, 0x00);
   usleep(200);
   XIo_Out32(NX_RESET, 0x01);
   usleep(200);
   XIo_Out32(NX_INIT, 1);
   usleep(200);
   XIo_Out32(NX_INIT, 0);

   XIo_Out32(FIFO_RESET, 1);
   XIo_Out32(FIFO_RESET, 0);
   
   XIo_Out32(NX_MISS_RESET, 1);
   XIo_Out32(NX_MISS_RESET, 0);
      
   sc_printf("\r\n");
}

void nx_autodelay()
{
   int dead, deadcount, ok, i;
   Xuint32 low, high, delay, max, min, id, ts, TS, nx, testdelay;
   Xuint32 anz, anz0, anz1;
   
   if (gC.NX_Number==0) XIo_Out32(NX1_DELAY,8);
   if (gC.NX_Number==1) XIo_Out32(NX2_DELAY,8);
   if (gC.NX_Number==2) XIo_Out32(NX3_DELAY,8);
   if (gC.NX_Number==3) XIo_Out32(NX4_DELAY,8);

   testsetup();
   
   for (i=0; i<=15; i++) {
      NX(i, 0xFF);
   }
   
   NX(3, 0x0F);

   XIo_Out32(TP_RESET_DELAY, 20000);
   XIo_Out32(TP_TP_DELAY, 2000 - 1);
   XIo_Out32(TP_COUNT, 5000 * 2);

   max=512; 
   dead=0; deadcount=0; ok=0;
   
   while (ok==0) {
      NX(32, 1);
        
        XIo_Out32(NX_INIT, 1);
         usleep(200);
         XIo_Out32(NX_RESET, 0);
         usleep(200);
         XIo_Out32(FIFO_RESET, 1);
         usleep(200);
         XIo_Out32(FIFO_RESET, 0);
         usleep(200);
         XIo_Out32(NX_RESET, 1);
         usleep(2000);
         XIo_Out32(NX_INIT, 0);

      dead=0;
      while (!XIo_In32(NX_FIFO_FULL)&&(dead<1000000)) dead++;
      NX(32, 0);
      if (dead >= 1000000) {
         sc_printf("!");
         deadcount++;
      }   else {
         ok=1;
      }                   
      if (deadcount>20) {
         sc_printf("\r\nERROR: There is no Testpulse-Data!\r\n");
         return;
      }
   }
   
   for (i=0; i<500; i++) {
       XIo_In32(ADC_DATA);
       nx=XIo_In32(NX_DATA);
       id = (nx & 0x00007F00)>>8;                
        ts = ((nx & 0x7F000000)>>17)+((nx & 0x007F0000)>>16);
       TS = ungray(ts^(0x3FFF), 14);
       low = XIo_In32(LT_LOW);
       high = XIo_In32(LT_HIGH);
       delay = (((low>>3)&0x1FF) - (TS>>5)) & 0x1FF;
       XIo_In32(AUX_DATA_LOW); XIo_In32(AUX_DATA_HIGH); 
   
       if (((nx & 0x80000000)>>31) && (((nx & 0x18)>>3)==gC.NX_Number) && (ungray(id, 7)==31)) {
          if (delay < max) max = delay;
       } else {
          sc_printf("invalid: %d", ungray(id, 7));
       }
   }
      
   max = 512 - max;
   testdelay = max*8 + 16;
   sc_printf("Edge detection... (%d) \r\n", testdelay);

   for (max=0; max < 256; testdelay--) {    
      max=0; min=512;
      anz=0; anz0=0; anz1=0;
      dead=0; deadcount=0; ok=0;
      
      if (gC.NX_Number==0) XIo_Out32(NX1_DELAY,testdelay);
      if (gC.NX_Number==1) XIo_Out32(NX2_DELAY,testdelay);
      if (gC.NX_Number==2) XIo_Out32(NX3_DELAY,testdelay);
      if (gC.NX_Number==3) XIo_Out32(NX4_DELAY,testdelay);
      
      while (ok==0) {
         NX(32, 1);
           XIo_Out32(NX_INIT, 1);
            usleep(200);
            XIo_Out32(NX_RESET, 0);
            usleep(200);
            XIo_Out32(FIFO_RESET, 1);
            usleep(200);
            XIo_Out32(FIFO_RESET, 0);
            usleep(200);
            XIo_Out32(NX_RESET, 1);
            usleep(2000);
            XIo_Out32(NX_INIT, 0);

         dead=0;
         while (!XIo_In32(NX_FIFO_FULL)&&(dead<1000000)) dead++;
         NX(32, 0);
         if (dead >= 1000000) {
            sc_printf("!");
            deadcount++;
         }   else {
            ok=1;
         }                   
         if (deadcount>20) {
            sc_printf("\r\nERROR: There is no Testpulse-Data!\r\n");
            return;
         }
      }
      
      for (i=0; i<500; i++) {
          XIo_In32(ADC_DATA);
          nx=XIo_In32(NX_DATA);
          id = (nx & 0x00007F00)>>8;                
           ts = ((nx & 0x7F000000)>>17)+((nx & 0x007F0000)>>16);
          TS = ungray(ts^(0x3FFF), 14);
          low = XIo_In32(LT_LOW);
          high = XIo_In32(LT_HIGH);
          delay = (((low>>3)&0x1FF) - (TS>>5)) & 0x1FF;
          XIo_In32(AUX_DATA_LOW); XIo_In32(AUX_DATA_HIGH); 
      
          if (((nx & 0x80000000)>>31) && (((nx & 0x18)>>3)==gC.NX_Number) && (ungray(id, 7)==31)) {
                 anz++;
             if (delay > max) max = delay;
             if (delay < min) min = delay;
              if (delay==0) anz0++;
              if (delay==1) anz1++;
          } else {
             sc_printf("invalid: %d", ungray(id, 7));
          }
      }
      sc_printf("testdelay: %d --- meassured min:%d, max: %d --- 0:%d%%, 1:%d%%\r\n", testdelay, min, max, (anz0*100)/anz, (anz1*100)/anz);
   }

    max = testdelay + 2;

   sc_printf ("Setting the measured TS-Delay of %d ...\r\n", max);
   if (gC.NX_Number==0) {XIo_Out32(NX1_DELAY, max); gC.rocNX1Delay = max;}
   if (gC.NX_Number==1) {XIo_Out32(NX2_DELAY, max); gC.rocNX2Delay = max;}
   if (gC.NX_Number==2) {XIo_Out32(NX3_DELAY, max); gC.rocNX3Delay = max;}
   if (gC.NX_Number==3) {XIo_Out32(NX4_DELAY, max); gC.rocNX4Delay = max;}
   sc_printf ("OK.\r\n");
   
   nx_reset();
}


void prefetch(int pref)
{
   if (pref == 1) {
      gC.prefetchActive = 1;
      XIo_Out32(FIFO_RESET, 1);
      XIo_Out32(PREFETCH, 1);
      XIo_Out32(FIFO_RESET, 0);
   } else {
      gC.prefetchActive = 0;
      XIo_Out32(PREFETCH, 0);
      XIo_In32(OCM_REG1);
      XIo_In32(OCM_REG2);
      XIo_In32(OCM_REG3);
      XIo_Out32(FIFO_RESET, 1);
      XIo_Out32(FIFO_RESET, 0);
   }
}

void activate_nx(int nx, int act, int con, int sa)
{
   int a,j; 

    NX_attribute[nx].active=act;
    NX_attribute[nx].Connector=con;
    NX_attribute[nx].I2C_SlaveAddr=sa;

   a=0;
   for (j=0; j<=3; j++) {
      if (NX_attribute[j].active==1) a=a+(1<<j);
   }
   XIo_Out32(ACTIVATE_NX, a);
}

Xuint16 peek(Xuint32 addr, Xuint32 *retVal, Xuint8* rawdata, Xuint32* rawdatasize)
{
   int nx = gC.NX_Number;
   int con = NX_attribute[nx].Connector;

   if ((addr>=ROC_LOW_LEVEL_BEGIN) &&  (addr<ROC_LOW_LEVEL_END) &&
      (gC.activateLowLevel==0)) return 0x4; //Permission denied

   if ((addr>=ROC_NX_REGISTER_BASE) && (addr<=ROC_NX_REGISTER_BASE + 0x2D)) { //NX-Register
      if (con==1) {
         XIo_Out32(I2C1_REGISTER, addr - ROC_NX_REGISTER_BASE);
         *retVal = XIo_In32(I2C1_DATA);
         if (XIo_In32(I2C1_ERROR)==1) return 7;
      } else {
         XIo_Out32(I2C2_REGISTER, addr - ROC_NX_REGISTER_BASE);
         *retVal = XIo_In32(I2C2_DATA);
         if (XIo_In32(I2C2_ERROR)==1) return 7;
      }
      return 0;
   }

   switch (addr){
      
      case ROC_SOFTWARE_VERSION:
         *retVal = SYSCORE_VERSION;
         return 0;
   
      case ROC_HARDWARE_VERSION:
         *retVal = XIo_In32(HW_VERSION);
         return 0;
        
      case ROC_NUMBER: 
         *retVal = gC.rocNumber;
         return 0;
   
      case ROC_BURST: 
         *retVal = gC.prefetchActive;    
         return 0;
   
      case ROC_NX_SELECT: 
         *retVal = gC.NX_Number;    
         return 0;
   
      case ROC_NX_ACTIVE: 
         *retVal = NX_attribute[gC.NX_Number].active;    
         return 0;
   
      case ROC_NX_CONNECTOR: 
         *retVal = NX_attribute[gC.NX_Number].Connector;    
         return 0;
   
      case ROC_NX_SLAVEADDR: 
         *retVal = NX_attribute[gC.NX_Number].I2C_SlaveAddr;    
         return 0;
   
      case ROC_FEB4NX:
         *retVal = gC.rocFEB4nx;
         return 0; 
   
      case ROC_NX_FIFO_EMPTY: 
         *retVal = XIo_In32(NX_FIFO_EMPTY);    
         return 0;
   
      case ROC_NX_FIFO_FULL:
         *retVal = XIo_In32(NX_FIFO_FULL);
         return 0;
        
      case ROC_ACTIVATE_LOW_LEVEL:
         *retVal = gC.activateLowLevel;
         return 0;
   
      case ROC_NX_DATA:
         *retVal =  XIo_In32(NX_DATA);
         return 0;
   
      case ROC_LT_LOW:
         *retVal = XIo_In32(LT_LOW); 
         return 0;
   
      case ROC_LT_HIGH:
         *retVal = XIo_In32(LT_HIGH);
         return 0;
   
      case ROC_NX_MISS:
         *retVal = XIo_In32(NX_MISS); 
         return  0;
   
      case ROC_ADC_DATA:
         *retVal = XIo_In32(ADC_DATA); 
         return 0;
   
      case ROC_ADC_REG:
         XIo_Out32(ADC_READ_EN, 1);
         usleep(2000);     
         *retVal = XIo_In32(ADC_ANSWER); 
         return 0;
   
      case ROC_BURST1:
         *retVal = XIo_In32(BURST1); 
         return 0;
   
      case ROC_BURST2:
         *retVal = XIo_In32(BURST2);
         return 0;
   
      case ROC_BURST3:
         *retVal = XIo_In32(BURST3);
         return 0;
   
      case ROC_DEBUG_NX:
         *retVal = XIo_In32(DEBUG_NX);
         return 0;
   
      case ROC_DEBUG_LTL:
         *retVal = XIo_In32(DEBUG_LTL);
         return 0;
   
      case ROC_DEBUG_LTH:
         *retVal = XIo_In32(DEBUG_LTH);
         return 0;
   
      case ROC_DEBUG_ADC:
         *retVal = XIo_In32(DEBUG_ADC);
         return 0;
   
      case ROC_DEBUG_NX2:
         *retVal = XIo_In32(DEBUG_NX2);
         return 0;
   
      case ROC_DEBUG_LTL2:
         *retVal = XIo_In32(DEBUG_LTL2);
         return 0;
   
      case ROC_DEBUG_LTH2:
         *retVal= XIo_In32(DEBUG_LTH2);
         return 0;
   
      case ROC_DEBUG_ADC2:
         *retVal = XIo_In32(DEBUG_ADC2);
         return 0;
   
      case ROC_I2C_DATA:
         *retVal = (con==1) ? XIo_In32(I2C1_DATA) : XIo_In32(I2C2_DATA);
         return 0;
   
      case ROC_OCM_REG1:
         *retVal= XIo_In32(OCM_REG1);
         return 0;
   
      case ROC_OCM_REG2:
         *retVal=  XIo_In32(OCM_REG2);
         return 0;
   
      case ROC_OCM_REG3:
         *retVal= XIo_In32(OCM_REG3);
         return 0;
   
      case ROC_SR_INIT:
         if (con==1) *retVal= gC.rocSrInit1;
         if (con==2) *retVal= gC.rocSrInit2;
         return 0;
   
      case ROC_BUFG_SELECT:
         if (con==1) *retVal = gC.rocBufgSelect1;
         if (con==2) *retVal = gC.rocBufgSelect2;
         return 0;
   
      case ROC_ADC_LATENCY:
         if (nx==1) *retVal = gC.rocAdcLatency1;
         if (nx==2) *retVal = gC.rocAdcLatency2;
         if (nx==3) *retVal = gC.rocAdcLatency3;
         if (nx==4) *retVal = gC.rocAdcLatency4;
         return 0;
   
      case ROC_DELAY_LTS:
         *retVal = gC.rocLTSDelay; 
         return 0;
   
      case ROC_DELAY_NX0:
         *retVal = gC.rocNX1Delay; 
         return 0;
   
      case ROC_DELAY_NX1:
         *retVal = gC.rocNX2Delay; 
         return 0;
   
      case ROC_DELAY_NX2:
         *retVal = gC.rocNX3Delay; 
         return 0;
   
      case ROC_DELAY_NX3:
         *retVal = gC.rocNX4Delay; 
         return 0;
   
      case ROC_IP_ADDRESS:
         memcpy(retVal, gC.ipAddrSc, 4); 
         swap_byteorder((Xuint8*)retVal,4);
         return 0;
        
      case ROC_MASTER_IP:
         memcpy(retVal, gD.masterIp, 4);
         swap_byteorder((Xuint8*)retVal, 4);
         return 0;
         
      case ROC_MAC_ADDRESS_UPPER:
         *retVal = 0;
         memcpy((Xuint8*)retVal+2, &gC.etherAddrSc[0], 2);
         swap_byteorder((Xuint8*)retVal,4);
         return 0;
         
      case ROC_MAC_ADDRESS_LOWER:
         memcpy(retVal, &gC.etherAddrSc[2], 4);
         swap_byteorder((Xuint8*)retVal,4);
         return 0;
         
      case ROC_NETMASK:
         memcpy(retVal, gC.ipNetmask, 4);
         swap_byteorder((Xuint8*)retVal, 4);
         return 0;
   
      case ROC_MASTER_DATAPORT:
         *retVal = gD.masterDataPort;
         return 0;
         
      case ROC_MASTER_CTRLPORT:
         *retVal = gD.masterControlPort;
         return 0;
   
      case ROC_BUFFER_FLUSH_TIMER:
         *retVal= gC.flushTimeout;
         return 0;
      
      case ROC_TESTPATTERN_GEN:
         *retVal = gC.testpatternGenerator;
         return 0;
      
      case ROC_LOST_ETHER_FRAMES:
         *retVal = TemacLostFrames;
         return 0;
      
      case ROC_UKNOWN_ETHER_FRAMES:
         *retVal = gD.etherFramesOtherIpOrArp;
         return 0;
   
      case ROC_CHECK_BITFILEBUFFER:
         *retVal = calcBitfileBufferChecksum();
         return 0;
         
      case ROC_CHECK_BITFILEFLASH0:
         *retVal = gD.xor0buff;
         return 0;
         
      case ROC_CHECK_BITFILEFLASH1:
         *retVal = gD.xor1buff;
         return 0;
   
      case ROC_CHECK_FILEBUFFER:
         *retVal = calcFileBufferChecksum();
         return 0;
   
      case ROC_TEMAC_REG0:
         return TemacReadCtrlReg(0, retVal);

      case ROC_CTRLPORT:
         *retVal = gC.ctrlPort;
         return 0;
      
      case ROC_DATAPORT:
         *retVal = gC.dataPort;
         return 0;
         
      case ROC_BURST_LOOPCNT:
         *retVal = gC.burstLoopCnt;
         return 0;
         
      case ROC_OCM_LOOPCNT:
         *retVal = gC.ocmLoopCnt;
         return 0;
         
      case ROC_STATBLOCK:
         memcpy(rawdata, &(gD.stat), sizeof(gD.stat));
         *rawdatasize = sizeof(gD.stat);
         return 0;
         
      case ROC_DEBUGMSG:
         *rawdatasize = strlen(gD.debugOutput) + 1;
         memcpy(rawdata, gD.debugOutput, *rawdatasize);
         return 0;
         
      default:
         if ((addr >= ROC_FBUF_START) && (addr + 4 <= ROC_FBUF_START + ROC_FBUF_SIZE)) {
            *retVal = * ((Xuint32*) (global_file_buffer + addr - ROC_FBUF_START));
            return 0;
         } 
   }     
     
   *retVal = 0;  
   return 2;  
}


Xuint16 poke(Xuint32 addr, Xuint32 val, Xuint8* rawdata)
{
   int nx = gC.NX_Number, r;
   int con = NX_attribute[nx].Connector;
   int a,j = 0;

   if ((addr>=ROC_LOW_LEVEL_BEGIN) &&  (addr<ROC_LOW_LEVEL_END) &&
      (gC.activateLowLevel==0)) return 0x4; //Permission denied
  
   if ((addr >= ROC_NX_REGISTER_BASE) && (addr <= ROC_NX_REGISTER_BASE + 0x2D)) {
     //NX-Register
      NX(addr - ROC_NX_REGISTER_BASE, val);
      if (XIo_In32(I2C1_ERROR)==1) return 7;
      if ((addr - ROC_NX_REGISTER_BASE) == 42) return 0;
      if (con==1) {
         XIo_Out32(I2C1_REGISTER, addr - ROC_NX_REGISTER_BASE);
         r = XIo_In32(I2C1_DATA);
         if (XIo_In32(I2C1_ERROR)==1) return 7;
      } else {
         XIo_Out32(I2C2_REGISTER, addr - ROC_NX_REGISTER_BASE);
         r = XIo_In32(I2C2_DATA);
         if (XIo_In32(I2C2_ERROR)==1) return 7;
      }
      return (r==val) ? 0 : 1;
   }
  
   switch (addr){
      case ROC_SYSTEM_RESET: 
         if (val!=1) return 3;
         XIo_Out32(WATCHDOG_OFF, 1);
         return 0; //Success
     
      case ROC_FIFO_RESET:
         if (val>1) return 3;
         XIo_Out32(FIFO_RESET, 1);
         XIo_In32(OCM_REG1);
         XIo_In32(OCM_REG2);
         XIo_In32(OCM_REG3);
         XIo_Out32(FIFO_RESET, 0);
         return 0; //Success
        
      case ROC_NUMBER:
         if (val>7) return 3;
         XIo_Out32(HW_ROC_NUMBER, val);
         gC.rocNumber = val;
         return 0; //Success
   
      case ROC_BURST:
         if ((gD.daqState != 0) || (val > 1)) return 3;
         prefetch(val);
         return 0; //Success
   
      case ROC_TESTPULSE_RESET_DELAY:
         XIo_Out32(TP_RESET_DELAY, val-1);
         return 0; //Success
   
      case ROC_TESTPULSE_LENGTH:
         XIo_Out32(TP_TP_DELAY, val-1);
         return 0; //Success
   
      case ROC_TESTPULSE_NUMBER:
         XIo_Out32(TP_COUNT, val*2);
         return 0; //Success
   
      case ROC_TESTPULSE_START:
         XIo_Out32(TP_START, val);
         return 0; //Success
   
      case ROC_TESTPULSE_RESET:
         XIo_Out32(NX_INIT, 1);
         usleep(200);
         XIo_Out32(NX_RESET, 0);
         usleep(200);
         XIo_Out32(FIFO_RESET, 1);
         usleep(200);
         XIo_Out32(FIFO_RESET, 0);
         usleep(200);
         XIo_Out32(NX_RESET, 1);
         usleep(2000);
         XIo_Out32(NX_INIT, 0);
         return 0; //Success
        
      case ROC_TEMAC_PRINT:
         if (val & 1) TemacPrintPhyRegisters();
         if (val & 2) TemacPrintConfig();
         return 0;
         
      case ROC_TEMAC_REG0:
         return TemacWriteCtrlReg(0, val);
   
      case ROC_OVERWRITE_SD_FILE:
         return overwriteSDFile();
         
      case ROC_NX_SELECT:
         if (val>3) return 3;
         set_nx(val, 1);
         return 0; //Success
   
      case ROC_NX_ACTIVE:
         if (val>1) return 3;
         NX_attribute[gC.NX_Number].active = val;
         a=0;
         for (j=0; j<=3; j++) 
            if (NX_attribute[j].active==1) a=a+(1<<j);
         XIo_Out32(ACTIVATE_NX, a);
         return 0; //Success
   
      case ROC_NX_CONNECTOR:
         if ((val==0) || (val>2)) return 3;
         NX_attribute[gC.NX_Number].Connector = val;
         return 0; //Success
   
      case ROC_NX_SLAVEADDR:
         NX_attribute[gC.NX_Number].I2C_SlaveAddr = val;
         if (NX_attribute[gC.NX_Number].Connector==1) 
            XIo_Out32(I2C1_SLAVEADDR, NX_attribute[gC.NX_Number].I2C_SlaveAddr);
         else 
         if (NX_attribute[gC.NX_Number].Connector==2) 
            XIo_Out32(I2C2_SLAVEADDR, NX_attribute[gC.NX_Number].I2C_SlaveAddr);
         return 0; //Success
   
      case ROC_FEB4NX:
         gC.rocFEB4nx = val;
         XIo_Out32(FEB4nx, val);
         return 0; //Success
   
      case ROC_PARITY_CHECK:
         XIo_Out32(CHECK_PARITY, 1);
         return 0; //Success
      
      case ROC_SYNC_M_SCALEDOWN:
         XIo_Out32(SYNC_M_DELAY, val);
         return 0; //Success
         
      case ROC_SYNC_BAUD_START:
         if (val<2) return 3;
         XIo_Out32(SYNC_BAUD_START, val-2);
         return 0; //Success
   
      case ROC_SYNC_BAUD1:
         if (val<2) return 3;
         XIo_Out32(SYNC_BAUD1, val-1);
         return 0; //Success
   
      case ROC_SYNC_BAUD2:
         if (val<2) return 3;
         XIo_Out32(SYNC_BAUD2, val-1);
         return 0; //Success
   
      case ROC_AUX_ACTIVE:
         XIo_Out32(AUX_ACTIVATE, val);
         return 0; //Success
        
      case ROC_CFG_READ:
         load_config(val > 1 ? (char*) rawdata : defConfigFileName);
         return 0;
   
      case ROC_CFG_WRITE:
         save_config(val > 1 ? (char*) rawdata : defConfigFileName);
         return 0;
            
      case ROC_ACTIVATE_LOW_LEVEL:
         if (val>1) return 3;
         gC.activateLowLevel = val;
         return 0; //Success
   
      case ROC_NX_INIT:
         XIo_Out32(NX_INIT, val);
         return 0; //Success
   
      case ROC_NX_RESET:
         XIo_Out32(NX_RESET, val);
         return 0; //Success
   
      case ROC_NX_MISS_RESET:
         XIo_Out32(NX_MISS_RESET, val);
         return 0; //Success
   
      case ROC_ADC_ADDR:
         XIo_Out32(ADC_ADDR, val);
         return 0; //Success
   
      case ROC_ADC_REG:
         XIo_Out32(ADC_REG, val);
         return 0; //Success
   
      case ROC_I2C_DATA:
         if (con==1) XIo_Out32(I2C1_DATA, val);
         if (con==2) XIo_Out32(I2C2_DATA, val);
         return 0; //Success
   
      case ROC_I2C_RESET:
         if (con==1) XIo_Out32(I2C1_RESET, val);
         if (con==2) XIo_Out32(I2C2_RESET, val);
         return 0; //Success
   
      case ROC_I2C_REGRESET:
         if (con==1) XIo_Out32(I2C1_REGRESET, val);
         if (con==2) XIo_Out32(I2C2_REGRESET, val);
         return 0; //Success
   
      case ROC_I2C_REGISTER:
         if (con==1) XIo_Out32(I2C1_REGISTER, val);
         if (con==2) XIo_Out32(I2C2_REGISTER, val);
         return 0; //Success
   
      case ROC_SHIFT:
         shiftit(val);
         return 0; //Success
   
      case ROC_SR_INIT:
         if (con==1) {
            XIo_Out32(SR_INIT, val);
            gC.rocSrInit1 = val;
         } else 
         if (con==2) {
            XIo_Out32(SR_INIT2, val);
            gC.rocSrInit2 = val;
         }      
         return 0; //Success
   
      case ROC_BUFG_SELECT:
         if (con==1) {
            XIo_Out32(BUFG_SELECT, val);
            gC.rocBufgSelect1 = val;
         } else 
         if (con==2) {
            XIo_Out32(BUFG_SELECT2, val);
            gC.rocBufgSelect2 = val;
         }
         return 0; //Success
   
      case ROC_ADC_LATENCY:
         if (nx==0) {
            XIo_Out32(ADC_LATENCY1, val);
            gC.rocAdcLatency1 = val;
         } else 
         if (nx==1) {
            XIo_Out32(ADC_LATENCY2, val);
            gC.rocAdcLatency2 = val;
         } else 
         if (nx==2) {
            XIo_Out32(ADC_LATENCY3, val);
            gC.rocAdcLatency3 = val;
         } else 
         if (nx==3) {
            XIo_Out32(ADC_LATENCY4, val);
            gC.rocAdcLatency4 = val;
         }   
         return 0; //Success
   
      case ROC_DO_AUTO_LATENCY:
         if (val!=1) return 3;
         nx_autocalibration();
         return 0; //Success
   
      case ROC_DO_AUTO_DELAY:
         if (val!=1) return 3;
         nx_autodelay(); 
         return 0; //Success
   
      case ROC_DO_TESTSETUP:
         if (val!=1) return 3;
         testsetup();
         return 0; //Success
   
      case ROC_DELAY_LTS:
         XIo_Out32(LTS_DELAY, val);
         gC.rocLTSDelay = val;
         return 0; //Success
   
      case ROC_DELAY_NX0:
         XIo_Out32(NX1_DELAY, val);
         gC.rocNX1Delay = val;
         return 0; //Success
   
      case ROC_DELAY_NX1:
         XIo_Out32(NX2_DELAY, val);
         gC.rocNX2Delay = val;
         return 0; //Success
   
      case ROC_DELAY_NX2:
         XIo_Out32(NX3_DELAY, val);
         gC.rocNX3Delay = val;
         return 0; //Success
   
      case ROC_DELAY_NX3:
         XIo_Out32(NX4_DELAY, val);
         gC.rocNX4Delay = val;
         return 0; //Success
   
      case ROC_FLASH_KIBFILE_FROM_DDR:
         if(val >= MAX_NUMBER_OF_KIBFILES) return 3;
            
         // flash bitfile, calc checksum from flashed file
         flash_access_flashBitfile(global_file_buffer, val);
         if(val == 0)
            gD.xor0buff = flash_access_calcKIBXor(0);
         else 
         if (val == 1)
            gD.xor1buff = flash_access_calcKIBXor(1);
         return 5;
         
      case ROC_CLEAR_FILEBUFFER:
         memset(global_file_buffer, 0, ROC_FBUF_SIZE);
         return 0;
      
      case ROC_SWITCHCONSOLE:
         gC.verbose = 1;
         sc_printf("Poke switch console!\r\n");
         if(gC.global_consolemode == 1)//ttymode
            gC.global_consolemode = 0;
         else
            gC.global_consolemode = 1;
         return 0;
      
      case ROC_START_DAQ:
         //if is already in daq mode
         if(gD.daqState != 0) {
            sc_printf("One should stop DAQ before one can start it!!!\r\n");
            return 3;//value error 
         }
   
         sc_printf("Starting DAQ\r\n");
   
         XTime_GetTime(&gD.lastFlushTime);
         gD.daqState = 1;
   
         resetBuffers();
         addSystemMessage(1, false);
         gD.data_taking_on = 1;
         return 0;
         
      case ROC_STOP_DAQ:
         gD.daqState = 0;
         resetBuffers();
         sc_printf("Stop DAQ\r\n");            
         return 0;
   
      case ROC_SUSPEND_DAQ:
         if (gD.daqState != 1) {
            sc_printf("One can suspend DAQ only from runnig mode!!!\r\n");
            return 3;
         }
         
         gD.daqState = 2;
         sc_printf("Suspending DAQ\r\n");
         
         addSystemMessage(2, true);
         gD.data_taking_on = 0;
         return 0;
         
      case ROC_IP_ADDRESS:
         swap_byteorder((Xuint8*)&val, 4);
         memcpy(gC.ipAddrSc, &val, 4);
         save_config(defConfigFileName);
         networkInit();
         return 0;
         
      case ROC_MASTER_LOGIN:
         return 0;
      
      case ROC_MASTER_LOGOUT:
         gD.masterConnected = 0; 
         gD.masterCounter = 0;  
         gD.lastMasterPacketId = 0;
         return 0;
      
      case ROC_MAC_ADDRESS_UPPER:
         swap_byteorder((Xuint8*)&val,4);
         memcpy(&gC.etherAddrSc[0], &val,  2);
         return 0;
         
      case ROC_MAC_ADDRESS_LOWER:
         swap_byteorder((Xuint8*)&val,4);
         memcpy(&gC.etherAddrSc[2], &val, 4);
         return 0;
         
      case ROC_NETMASK:
         swap_byteorder((Xuint8*)&val,4);
         memcpy(gC.ipNetmask, &val, 4);
         return 0;
   
      case ROC_MASTER_DATAPORT:
         gD.masterDataPort = val;
         return 0;
         
      case ROC_MASTER_CTRLPORT:
         gD.masterControlPort = val;
         return 0;
   
      case ROC_BUFFER_FLUSH_TIMER:
         if ((val < 2) || (val > 3600000)) return 3;
         gC.flushTimeout = val;         
         return 0;
      
      case ROC_RESTART_NETWORK:
         TemacInitReg0 = val;
         networkInit();
         return 0;
      
      case ROC_CONSOLE_OUTPUT:
         gC.verbose = val;
         sc_printf("console output is switched to %d\r\n", val);
         return 0;
         
      case ROC_CONSOLE_CMD:
         if (val < 2) return 3;
         return executeConsoleCommand((char*) rawdata);
         
      case ROC_TESTPATTERN_GEN:
         if (val > 1) return 3;
         sc_printf("testpattern gen is switched to %u\r\n", val);
         gC.testpatternGenerator = (val == 1) ? true : false;
         return 0;
         
      case ROC_CTRLPORT:
         gC.ctrlPort = val;
         return 0;
      
      case ROC_DATAPORT:
         gC.dataPort = val;
         return 0;
         
      case ROC_BURST_LOOPCNT:
         gC.burstLoopCnt = val;
         return 0;
         
      case ROC_OCM_LOOPCNT:
         gC.ocmLoopCnt = val;
         return 0;

      default:
         if ((addr >= ROC_FBUF_START) && (addr + val <= ROC_FBUF_START + ROC_FBUF_SIZE)) {
            memcpy(global_file_buffer + addr - ROC_FBUF_START, rawdata, val);
            return 0;
         } 
   }
   return 2;   //Address-Error
}


void load_config(char* filename) 
{
   XCache_DisableICache();
   XCache_DisableDCache();

   sc_printf("Load config file %s ...\r\n", filename);

   global_file_buffer[0] = 0;
   int read_res = load_file_from_sd_card(filename, global_file_buffer);
   
   XCache_EnableICache(CACHEABLE_AREA);
   XCache_EnableDCache(CACHEABLE_AREA);

   if (read_res || (strlen(global_file_buffer) == 0)) {
      sc_printf("\r\n\r\nPlease check your SD-Card - file %s read error...\r\n", filename);
      return;
   }
     
   Xuint8* curr = global_file_buffer;
   Xuint8 lastsymb = 32;
   char buffer[256];
   int len = 0;
   int separ = -1;

   int temp[6];

#define LoadValue(var_name, var)     \
   if (!strcmp(name, var_name))      \
     var = strtol(value, &perr, 0); else 

   while (lastsymb) {
      lastsymb = *curr++;
      if ((lastsymb!=10) && (lastsymb!=0)) {
         if ((lastsymb > 26) && (lastsymb!=32) && (len < sizeof(buffer) - 2)) {
            if ((lastsymb==':') && (separ<0)) separ = len;
            buffer[len++] = lastsymb;
         }
         continue;
      }
      
      // one cannot have arg:value with length smaller than 3 symbols
      if ((len<3) || (separ<0) || (separ==0) || (separ==len-1)) { len = 0; separ = -1; continue; }

      buffer[len] = 0; // put 0 to make valid c-string
      buffer[separ] = 0; // replace ":" by 0 to get name as normal string
      
      char* name = (char*) buffer;
      char* value = (char*) buffer + separ + 1;
      char* perr = 0;
      len = 0; 
      separ = -1;
      
      if (!strcmp(name,"IP")) {
         if (sscanf(value,"%d.%d.%d.%d", temp, temp+1, temp+2, temp+3)==4) {
            gC.ipAddrSc[0] = temp[0];
            gC.ipAddrSc[1] = temp[1];
            gC.ipAddrSc[2] = temp[2];
            gC.ipAddrSc[3] = temp[3];
         } else
            perr = value;
      } else
      
      if (!strcmp(name,"MASK")) {
         if (sscanf(value,"%d.%d.%d.%d", temp, temp+1, temp+2, temp+3)==4) {
            gC.ipNetmask[0] = temp[0];
            gC.ipNetmask[1] = temp[1];
            gC.ipNetmask[2] = temp[2];
            gC.ipNetmask[3] = temp[3];
         } else
            perr = value;
      } else
            
      if (!strcmp(name,"MAC")) {
         if (sscanf(value,"%x:%x:%x:%x:%x:%x", temp, temp+1, temp+2, temp+3, temp+4, temp+5)==6) {
            gC.etherAddrSc[0] = temp[0];
            gC.etherAddrSc[1] = temp[1];
            gC.etherAddrSc[2] = temp[2];
            gC.etherAddrSc[3] = temp[3];
            gC.etherAddrSc[4] = temp[4];
            gC.etherAddrSc[5] = temp[5];
         } else
            perr = value;
      } else

      LoadValue("UDP_CNTL",   gC.ctrlPort)
      LoadValue("UDP_DATA",   gC.dataPort)
      LoadValue("FLUSH_TIME", gC.flushTimeout)
      LoadValue("ROC",        gC.rocNumber)
      LoadValue("DELAY_LTS",  gC.rocLTSDelay)
      LoadValue("DELAY_NX0",  gC.rocNX1Delay)
      LoadValue("DELAY_NX1",  gC.rocNX2Delay)
      LoadValue("DELAY_NX2",  gC.rocNX3Delay)
      LoadValue("DELAY_NX3",  gC.rocNX4Delay)
      LoadValue("SR_INIT",    gC.rocSrInit1)
      LoadValue("BUFG_SELECT",gC.rocBufgSelect1)
      LoadValue("SR_INIT2",   gC.rocSrInit2)
      LoadValue("BUFG_SELECT2",gC.rocBufgSelect2)
      LoadValue("ADC_LATENCY1", gC.rocAdcLatency1)
      LoadValue("ADC_LATENCY2", gC.rocAdcLatency2)
      LoadValue("ADC_LATENCY3", gC.rocAdcLatency3)
      LoadValue("ADC_LATENCY4", gC.rocAdcLatency4)
      LoadValue("NX0_ACTIVE", NX_attribute[0].active)
      LoadValue("NX1_ACTIVE", NX_attribute[1].active)
      LoadValue("NX2_ACTIVE", NX_attribute[2].active)
      LoadValue("NX3_ACTIVE", NX_attribute[3].active)
      LoadValue("NX0_CONNECTOR", NX_attribute[0].Connector)
      LoadValue("NX1_CONNECTOR", NX_attribute[1].Connector)
      LoadValue("NX2_CONNECTOR", NX_attribute[2].Connector)
      LoadValue("NX3_CONNECTOR", NX_attribute[3].Connector)
      LoadValue("NX0_SLAVEADDR", NX_attribute[0].I2C_SlaveAddr)
      LoadValue("NX1_SLAVEADDR", NX_attribute[1].I2C_SlaveAddr)
      LoadValue("NX2_SLAVEADDR", NX_attribute[2].I2C_SlaveAddr)
      LoadValue("NX3_SLAVEADDR", NX_attribute[3].I2C_SlaveAddr)
      LoadValue("NX_NUMBER",     gC.NX_Number)
      LoadValue("UART",          gC.global_consolemode)
      LoadValue("CONSOLE_OUTPUT",gC.verbose)
      LoadValue("TEMAC_LINK_SPEED", TemacLinkSpeed)
      LoadValue("TEMAC_PHYREG0",    TemacInitReg0)
      LoadValue("BURST_LOOPCNT", gC.burstLoopCnt)
      LoadValue("OCM_LOOPCNT", gC.ocmLoopCnt)
      {
         sc_printf("Entry %s : %s is obsolete\r\n", name, value);
         continue;  
      }
      
      if (perr && *perr)
         sc_printf("Error to decode %s : %s\r\n", name, value);
      else
         sc_printf("Read %s : %s\r\n", name, value);
   }
   
#undef LoadValue

   XIo_Out32(HW_ROC_NUMBER, gC.rocNumber);
   XIo_Out32(LTS_DELAY, gC.rocLTSDelay);

   XIo_Out32(NX1_DELAY, gC.rocNX1Delay);
   XIo_Out32(NX2_DELAY, gC.rocNX2Delay);
   XIo_Out32(NX3_DELAY, gC.rocNX3Delay);
   XIo_Out32(NX4_DELAY, gC.rocNX4Delay);
   XIo_Out32(SR_INIT, gC.rocSrInit1);
   XIo_Out32(BUFG_SELECT, gC.rocBufgSelect1);
   XIo_Out32(SR_INIT2, gC.rocSrInit2);
   XIo_Out32(BUFG_SELECT2, gC.rocBufgSelect2);
    
   XIo_Out32(ADC_LATENCY1, gC.rocAdcLatency1);
   XIo_Out32(ADC_LATENCY2, gC.rocAdcLatency2);
   XIo_Out32(ADC_LATENCY3, gC.rocAdcLatency3);
   XIo_Out32(ADC_LATENCY4, gC.rocAdcLatency4);
   
   // ???? why it is inside loop ???
   int a = 0, j = 0;
   for (j=0; j<=3; j++) 
      if (NX_attribute[j].active == 1) a |= (1<<j);
   XIo_Out32(ACTIVATE_NX, a);
   set_nx(gC.NX_Number, 0); 
}

void save_config(char* filename)
{
   char* s = (char*) global_file_buffer + 1024;
    
   s += sprintf(s, "IP: %d.%d.%d.%d\n", gC.ipAddrSc[0], gC.ipAddrSc[1], gC.ipAddrSc[2], gC.ipAddrSc[3]);
   s += sprintf(s, "MASK: %d.%d.%d.%d\n", gC.ipNetmask[0],  gC.ipNetmask[1],  gC.ipNetmask[2],  gC.ipNetmask[3]);
   s += sprintf(s, "MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", gC.etherAddrSc[0],  gC.etherAddrSc[1],  gC.etherAddrSc[2],  gC.etherAddrSc[3],  gC.etherAddrSc[4],  gC.etherAddrSc[5]);
   s += sprintf(s, "UDP_CNTL: %d\n", gC.ctrlPort);
   s += sprintf(s, "UDP_DATA: %d\n", gC.dataPort);
   s += sprintf(s, "FLUSH_TIME: %d\n", gC.flushTimeout);
   s += sprintf(s, "ROC: %d\n", gC.rocNumber);
   s += sprintf(s, "DELAY_LTS: %d\n", gC.rocLTSDelay);
   s += sprintf(s, "DELAY_NX0: %d\n", gC.rocNX1Delay);
   s += sprintf(s, "DELAY_NX1: %d\n", gC.rocNX2Delay);
   s += sprintf(s, "DELAY_NX2: %d\n", gC.rocNX3Delay);
   s += sprintf(s, "DELAY_NX3: %d\n", gC.rocNX4Delay);
   s += sprintf(s, "SR_INIT: %d\n", gC.rocSrInit1);
   s += sprintf(s, "BUFG_SELECT: %d\n", gC.rocBufgSelect1);
   s += sprintf(s, "SR_INIT2: %d\n", gC.rocSrInit2);
   s += sprintf(s, "BUFG_SELECT2: %d\n", gC.rocBufgSelect2);
   s += sprintf(s, "ADC_LATENCY1: %d\n", gC.rocAdcLatency1);
   s += sprintf(s, "ADC_LATENCY2: %d\n", gC.rocAdcLatency2);
   s += sprintf(s, "ADC_LATENCY3: %d\n", gC.rocAdcLatency3);
   s += sprintf(s, "ADC_LATENCY4: %d\n", gC.rocAdcLatency4);
   s += sprintf(s, "NX0_ACTIVE: %d\n", NX_attribute[0].active);
   s += sprintf(s, "NX1_ACTIVE: %d\n", NX_attribute[1].active);
   s += sprintf(s, "NX2_ACTIVE: %d\n", NX_attribute[2].active);
   s += sprintf(s, "NX3_ACTIVE: %d\n", NX_attribute[3].active);
   s += sprintf(s, "NX0_CONNECTOR: %d\n", NX_attribute[0].Connector);
   s += sprintf(s, "NX1_CONNECTOR: %d\n", NX_attribute[1].Connector);
   s += sprintf(s, "NX2_CONNECTOR: %d\n", NX_attribute[2].Connector);
   s += sprintf(s, "NX3_CONNECTOR: %d\n", NX_attribute[3].Connector);
   s += sprintf(s, "NX0_SLAVEADDR: %d\n", NX_attribute[0].I2C_SlaveAddr);
   s += sprintf(s, "NX1_SLAVEADDR: %d\n", NX_attribute[1].I2C_SlaveAddr);
   s += sprintf(s, "NX2_SLAVEADDR: %d\n", NX_attribute[2].I2C_SlaveAddr);
   s += sprintf(s, "NX3_SLAVEADDR: %d\n", NX_attribute[3].I2C_SlaveAddr);
   s += sprintf(s, "NX_NUMBER: %d\n", gC.NX_Number);
   s += sprintf(s, "UART: %d\n", gC.global_consolemode);
   s += sprintf(s, "CONSOLE_OUTPUT: %d\n", gC.verbose);
   s += sprintf(s, "TEMAC_LINK_SPEED: %d\n", TemacLinkSpeed);
   s += sprintf(s, "TEMAC_PHYREG0: 0x%04X\n", TemacInitReg0);
   s += sprintf(s, "BURST_LOOPCNT: %d\n", gC.burstLoopCnt);
   s += sprintf(s, "OCM_LOOPCNT: %d\n", gC.ocmLoopCnt);
   
   *((Xuint32*) global_file_buffer) = s - (char*) global_file_buffer - 1024;
   memcpy(global_file_buffer + 4, filename, strlen(filename) + 1);
   
   overwriteSDFile();
}

int overwriteSDFile()
{
   Xuint32 fileSize = *((Xuint32*) global_file_buffer);

   if (fileSize>sizeof(global_file_buffer)) {
      sc_printf("File size %u too big, error\r\n", fileSize);
      return 2;
   }
   
   char* filename = global_file_buffer + 4;
   
   sc_printf("Try to overwrite file %s size %u\r\n", filename, fileSize);

   XCache_DisableICache();
   XCache_DisableDCache();
   
   Xuint32 sdblocksize = define_sd_file_size(filename);
   
   sc_printf("File %s uses %u bytes on SD card\r\n", filename, sdblocksize);

   int res = 0;
   
   Xuint8* tgt = (Xuint8*) global_file_buffer + 1024;
   
   if (sdblocksize < fileSize) {
      sc_printf("New file size %u bigger than SD has\r\n", fileSize);
      res = 2;
   } else {
      // avoid any kind of trash in the end of block, which may be in valid file size
      memset(tgt + fileSize, 0, sdblocksize - fileSize);
      if (save_file_to_sd_card(filename, tgt)) {
         sc_printf("File %s cannot be stored\r\n", filename);
         res = 2; 
      } else 
         sc_printf("File %s stored on SD card !!!\r\n", filename);
   }

   XCache_EnableICache(CACHEABLE_AREA);
   XCache_EnableDCache(CACHEABLE_AREA);
   
   return res;
}

 /****************************************************************************************************
  * Networking
  ****************************************************************************************************/
 
void networkInit()
{   
   //set the MAC used in TapDev
   cpy_ether_addr(TemacMAC, gC.etherAddrSc);
   
   printf("\r\nInitialize the device driver...\r\n");
   //while(status == XST_FAILURE)
   tapdev_init();
}

int checkCtrlOperationAllowed(struct ether_header *ether_hdr,
                              struct ip_header *ip_hdr,
                              struct udp_header *udp_hdr,
                              struct SysCore_Message_Packet *scp_hdr)
{
   // if all three addresses are equal to master just accept them
   
   // returns 0 if operation not allowed
   //         1 if it is master opearation (reply should be remembered)
   //         2 if it is slave operation (reply can be skipped)     
   
   if (ether_addr_equal(gD.masterEther, ether_hdr->src_addr) &&
       ip_addr_equal(gD.masterIp, ip_hdr->src) &&
       (gD.masterControlPort == udp_hdr->sport)) {
          gD.masterConnected = 1;
          gD.masterCounter = MASTER_DISCONNECT_TIME;
          return 1;
   }
   
   // this is situation, when via target address we remember all other addresses
   if ((scp_hdr->tag == ROC_POKE) && 
       (scp_hdr->address == ROC_MASTER_LOGIN)) {
          
          if (gD.masterConnected) {
             xil_printf("No more master allowed !!!\r\n");
             return 0;
          }
          
          cpy_ether_addr(gD.masterEther, ether_hdr->src_addr);
          cpy_ip_addr(gD.masterIp, ip_hdr->src);
          gD.masterControlPort = udp_hdr->sport;
                  
          gD.masterConnected = 1;
          gD.masterCounter = MASTER_DISCONNECT_TIME;
          gD.lastMasterPacketId = 0;
                  
          xil_printf("Master host: %d.%d.%d.%d  port: %d\n\r", 
             gD.masterIp[0], gD.masterIp[1], gD.masterIp[2], gD.masterIp[3], gD.masterControlPort);
          
          return 1;
       }

   // for moment allow all PEEK opearations for slave
   if (scp_hdr->tag == ROC_PEEK) return 2;
   
   return 0; // reject operation
}

void sendConsoleData(Xuint32 addr, void* rawdata, unsigned payloadSize)
{
   if ((payloadSize==0) || (payloadSize > MAX_UDP_PAYLOAD - sizeof(struct SysCore_Message_Packet))) {
      xil_printf("Error message len %u\r\n", payloadSize);
      return;
   }
   
   Xuint8 send_buf[MAX_ETHER_FRAME_SIZE];
   
   struct ether_header *ether_hdr = (struct ether_header*) send_buf; 
   struct ip_header *ip_hdr = (struct ip_header*) (send_buf + IP_HEADER_OFFSET);
   struct udp_header *udp_hdr = (struct udp_header*) (send_buf + UDP_HEADER_OFFSET);
   struct SysCore_Message_Packet* scp = (struct SysCore_Message_Packet*) (send_buf + UDP_PAYLOAD_OFFSET);

    //ethernet header
   ether_hdr->ether_type = ETHERTYPE_IP;
   
   cpy_ether_addr(ether_hdr->src_addr, gC.etherAddrSc);
   cpy_ether_addr(ether_hdr->dest_addr, gD.masterEther);
   
   //ip header
   ip_hdr->version = IP_VERSION_4;
   ip_hdr->header_length = IP_HEADER_LENGTH;
   ip_hdr->type_of_service = 0;//IP_DSCP_HIGHEST_PRIORITY;
//   ip_hdr->total_length = MAX_ETHER_PAYLOAD;
   ip_hdr->packet_number_id = 0;
   ip_hdr->flags = IP_FLAGS;
   ip_hdr->fragment_offset = 0;
   ip_hdr->time_to_live = IP_TTL;
   ip_hdr->proto = IP_PROTO_UDP;
//   ip_hdr->header_chksum = 0;
   cpy_ip_addr(ip_hdr->src, gC.ipAddrSc);
   cpy_ip_addr(ip_hdr->dest, gD.masterIp);
   
   //udp header
   udp_hdr->sport = gC.ctrlPort;
   udp_hdr->dport = gD.masterControlPort;
//   udp_hdr->len = MAX_IP_PAYLOAD;
   udp_hdr->chksum = 0;
   
   scp->tag = ROC_CONSOLE;
   scp->password = ROC_PASSWORD;
   scp->address = addr;
   scp->value = payloadSize;
   scp->id = 0;
   
   memcpy(send_buf + UDP_PAYLOAD_OFFSET + sizeof(struct SysCore_Message_Packet), rawdata, payloadSize);
   
   payloadSize += sizeof(struct SysCore_Message_Packet); 

   while ((payloadSize + UDP_PAYLOAD_OFFSET) % 4) payloadSize++;

   udp_hdr->len = sizeof(struct udp_header) + payloadSize;

   ip_hdr->total_length = sizeof(struct ip_header) + udp_hdr->len;

   ip_hdr->header_chksum = 0;
   ip_hdr->header_chksum = crc16((Xuint16*) ip_hdr, sizeof(struct ip_header));

   payloadSize += UDP_PAYLOAD_OFFSET;

//   tapdev_write_to_fifo_end_of_packet(send_buf, payloadSize);
//   tapdev_send(payloadSize);         

   // do not send console message if we have not enough space in FIFO 
   if (XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND) < payloadSize) return;
   
   XStatus Status = XTemac_FifoWrite(&TemacInstance, send_buf, payloadSize, XTE_END_OF_PACKET);
   
   if (Status == XST_SUCCESS) 
      Status = XTemac_FifoSend(&TemacInstance, payloadSize);

   // do we need this ??? - anyway, it is fast   
   if (Status == XST_SUCCESS) {
      gCnt.sendRate += payloadSize;
      
      Status = TemacGetTxStatus();
      
      if (Status == XST_NO_DATA) Status = XST_SUCCESS;
   }
   
   if (Status != XST_SUCCESS) {
      sc_printf("Control message send fails\r\n");
      TemacResetDevice();
      tapdev_init();
   }
}

void sendControlMessage(struct ether_header *ether_src,
                        struct ip_header *ip_src,
                        struct udp_header *udp_src,
                        Xuint8* send_buf, Xuint32 rawdatasize)
{
   struct ether_header *ether_hdr = (struct ether_header*) send_buf; 
   struct ip_header *ip_hdr = (struct ip_header*) (send_buf + IP_HEADER_OFFSET);
   struct udp_header *udp_hdr = (struct udp_header*) (send_buf + UDP_HEADER_OFFSET);

    //ethernet header
   ether_hdr->ether_type = ETHERTYPE_IP;
   
   cpy_ether_addr(ether_hdr->dest_addr, ether_src->src_addr);
   cpy_ether_addr(ether_hdr->src_addr, gC.etherAddrSc);
   
   //ip header
   ip_hdr->version = IP_VERSION_4;
   ip_hdr->header_length = IP_HEADER_LENGTH;
   ip_hdr->type_of_service = 0;//IP_DSCP_HIGHEST_PRIORITY;
//   ip_hdr->total_length = MAX_ETHER_PAYLOAD;
   ip_hdr->packet_number_id = 0;
   ip_hdr->flags = IP_FLAGS;
   ip_hdr->fragment_offset = 0;
   ip_hdr->time_to_live = IP_TTL;
   ip_hdr->proto = IP_PROTO_UDP;
//   ip_hdr->header_chksum = 0;
   cpy_ip_addr(ip_hdr->src, gC.ipAddrSc);
   cpy_ip_addr(ip_hdr->dest, ip_src->src);
   
   //udp header
   udp_hdr->sport = gC.ctrlPort;
   udp_hdr->dport = udp_src->sport;
//   udp_hdr->len = MAX_IP_PAYLOAD;
   udp_hdr->chksum = 0;
   
   Xuint16 payloadSize = sizeof(struct SysCore_Message_Packet) + rawdatasize;

   while ((payloadSize < MAX_UDP_PAYLOAD) &&
          (payloadSize + UDP_PAYLOAD_OFFSET) % 4) payloadSize+=1;

   udp_hdr->len = sizeof(struct udp_header) + payloadSize;

   ip_hdr->total_length = sizeof(struct ip_header) + udp_hdr->len;

   ip_hdr->header_chksum = 0;
   ip_hdr->header_chksum = crc16((Xuint16*) ip_hdr, sizeof(struct ip_header));

   payloadSize += UDP_PAYLOAD_OFFSET;

//   tapdev_write_to_fifo_end_of_packet(send_buf, payloadSize);
//   tapdev_send(payloadSize);         


   while (XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND) < payloadSize);
   
   XStatus Status = XTemac_FifoWrite(&TemacInstance, send_buf, payloadSize, XTE_END_OF_PACKET);
   
   if (Status == XST_SUCCESS) 
      Status = XTemac_FifoSend(&TemacInstance, payloadSize);

   // do we need this ??? - anyway, it is fast   
   if (Status == XST_SUCCESS) {
      gCnt.sendRate += payloadSize;
      
      Status = TemacGetTxStatus();
      
      if (Status == XST_NO_DATA) Status = XST_SUCCESS;
   }
   
   if (Status != XST_SUCCESS) {
      sc_printf("Control message send fails\r\n");
      TemacResetDevice();
      tapdev_init();
   }
      

   
}


int ethernetPacketDispatcher()
{    
   Xuint32 RxFrameLength = 0;

   /* Check if frame has arrived */
   switch(TemacGetRxStatus()) {
      case XST_SUCCESS:  /* Got a successfull receive status */
         break;
      case XST_NO_DATA:  /* Timed out */
      case XST_DATA_LOST:
         return 0;
      default:           /* Some other error */
         TemacResetDevice();
         tapdev_init();
         return 0;
   }

   if (XTemac_FifoRecv(&TemacInstance, &RxFrameLength) != XST_SUCCESS) {
      TemacResetDevice();
      tapdev_init();
      return 0;
   }
   
   if (RxFrameLength <= 14) return 0;
   
//   if (RxFrameLength < 14 + sizeof(struct icmp_header))
//      xil_printf("Strange packet size %u\r\n", RxFrameLength);

   // no problem, just take as much data as we can
   if (RxFrameLength > MAX_ETHER_FRAME_SIZE)
      RxFrameLength = MAX_ETHER_FRAME_SIZE;

   if (XTemac_FifoRead(&TemacInstance, 
          gD.recv_buf, RxFrameLength, XTE_END_OF_PACKET) != XST_SUCCESS) {
      TemacResetDevice();
      tapdev_init();
      return 0;
   }
   
   gCnt.recvRate += RxFrameLength;

   /******************************************
    * ethernet header filtering
    * delete all:
    * - not arp or ip packets
    *******************************************/

   struct ether_header *ether_hdr = (struct ether_header *) gD.recv_buf;
   
   if (ether_hdr->ether_type == ETHERTYPE_IP)
   {
      struct ip_header *ip_hdr = (struct ip_header *) (gD.recv_buf + IP_HEADER_OFFSET);
      struct udp_header *udp_hdr = (struct udp_header*) (gD.recv_buf + UDP_HEADER_OFFSET);
      
      //ip filter
      
//      if (ip_hdr->proto == IP_PROTO_UDP)
//         xil_printf("Packet sport %d dport %d\r\n", udp_hdr->sport, udp_hdr->dport);

      // this is UDP packet comming over data port
      if (ip_hdr->proto          == IP_PROTO_UDP      &&
         ip_addr_equal(ip_hdr->dest, gC.ipAddrSc)     &&
         udp_hdr->dport          == gC.dataPort       && 
         ( ((struct SysCore_Data_Request*) (gD.recv_buf + UDP_PAYLOAD_OFFSET))->password == ROC_PASSWORD) )
      {
        // for safety reasons, check that message comes from correct source 
        if (ether_addr_equal(gD.masterEther, ether_hdr->src_addr) &&
            ip_addr_equal(gD.masterIp, ip_hdr->src) &&
            (gD.masterDataPort == udp_hdr->sport)) {
                gD.masterConnected = 1;
                gD.masterCounter = MASTER_DISCONNECT_TIME;
                processDataRequest((struct SysCore_Data_Request*) (gD.recv_buf + UDP_PAYLOAD_OFFSET));
            }
         return 1;
      } else
      
      // this is UDP packet, comming to control port
      if (ip_hdr->proto          == IP_PROTO_UDP      &&
         ip_addr_equal(ip_hdr->dest, gC.ipAddrSc)     &&
         udp_hdr->dport          == gC.ctrlPort      && 
         ( ((struct SysCore_Message_Packet*) (gD.recv_buf + SCP_HEADER_OFFSET))->password == ROC_PASSWORD) )
      {
         struct SysCore_Message_Packet *scp_hdr = (struct SysCore_Message_Packet*) (gD.recv_buf + SCP_HEADER_OFFSET);
         struct SysCore_Message_Packet *scp_tgt = (struct SysCore_Message_Packet*) (gD.send_buf + SCP_HEADER_OFFSET);
         
         memcpy(scp_tgt, scp_hdr, sizeof(struct SysCore_Message_Packet));
         
         int check = checkCtrlOperationAllowed(ether_hdr, ip_hdr, udp_hdr, scp_hdr);

         Xuint32 rawdatasize = 0;
         
         if (check==0) {
            // if operation not allowed, just reply with same packet, but
            // set error return code 7
            scp_tgt->value = 7; // forbidden
         } else
         if ((check==1) && (scp_hdr->id == gD.lastMasterPacketId)) {
            // just send again previous reply
            memcpy(gD.send_buf, gD.retry_buf, MAX_ETHER_FRAME_SIZE);
            rawdatasize = gD.retry_buf_rawsize;
         } else 
         switch (scp_hdr->tag) {
            case ROC_PEEK:
               scp_tgt->address = peek(scp_hdr->address, &(scp_tgt->value),
                                        gD.send_buf + SCP_HEADER_OFFSET + sizeof(struct SysCore_Message_Packet), &rawdatasize);
               break;
               
            case ROC_POKE:
               scp_tgt->value = poke(scp_hdr->address, scp_hdr->value, 
                            gD.recv_buf + SCP_HEADER_OFFSET + sizeof(struct SysCore_Message_Packet));
               break;
         }

         sendControlMessage(ether_hdr, ip_hdr, udp_hdr, gD.send_buf, rawdatasize);
         
         if ((check==1) && (scp_hdr->id != gD.lastMasterPacketId)) {
            gD.lastMasterPacketId = scp_hdr->id;
            memcpy(gD.retry_buf, gD.send_buf, MAX_ETHER_FRAME_SIZE);
            gD.retry_buf_rawsize = rawdatasize;
         }
         
         return 1;
      }
      else if(ip_hdr->proto == IP_PROTO_ICMP)
      {
         struct ip_header *ip_hdr = (void*)ether_hdr + IP_HEADER_OFFSET;
         struct icmp_header *icmp_hdr = (void*)ether_hdr + ICMP_HEADER_OFFSET;
      
         //icmp filter
         //only ping packets
         if (icmp_hdr->type  == ICMP_TYPE_PING_REQUEST &&         
            ip_addr_equal(ip_hdr->dest, gC.ipAddrSc))
         {
            struct ether_header *ether_buff =   (void*) gD.send_buf + ETHER_HEADER_OFFSET;      
            struct ip_header *ip_buff =       (void*) gD.send_buf + IP_HEADER_OFFSET;      
            struct icmp_header *icmp_buff =    (void*) gD.send_buf + ICMP_HEADER_OFFSET;      
            //send pong
            memset(gD.send_buf, 0, MAX_ETHER_FRAME_SIZE);               
            memcpy(gD.send_buf, gD.recv_buf, ip_hdr->total_length + sizeof(struct ether_header));

            cpy_ether_addr(ether_buff->dest_addr, ether_hdr->src_addr);
            cpy_ether_addr(ether_buff->src_addr,  ether_hdr->dest_addr);
            cpy_ip_addr(ip_buff->dest, ip_hdr->src);
            cpy_ip_addr(ip_buff->src, ip_hdr->dest);
            icmp_buff->type = ICMP_TYPE_PING_REPLY;
            ip_buff->time_to_live *= 2;

            icmp_buff->chksum = 0;
            icmp_buff->chksum = crc16((void*)icmp_buff, ip_hdr->total_length - sizeof(struct ip_header));
            ip_buff->header_chksum = 0;
            ip_buff->header_chksum = crc16((void*) ip_buff, sizeof(struct ip_header));
            
            int packetSize = ip_hdr->total_length + sizeof(struct ether_header);

            tapdev_write_to_fifo_end_of_packet(gD.send_buf, packetSize);
            tapdev_send(packetSize);    
            
            gCnt.sendRate += packetSize;
            
            return 1;
         }
      }
   } else 
   if(ether_hdr->ether_type == ETHERTYPE_ARP) 
   {
      struct arp_header* arp_hdr =  (void*)ether_hdr + ARP_HEADER_OFFSET;
      if (arp_hdr->oper == ARP_REQUEST)
      {
         //send arp reply
         
         memset(gD.send_buf, 0, MAX_ETHER_FRAME_SIZE);
         struct ether_header *ether_tgt = (void*)gD.send_buf;
         struct arp_header *arp_tgt = (void*)(gD.send_buf + ARP_HEADER_OFFSET);

         ether_tgt->ether_type = ETHERTYPE_ARP;
         cpy_ether_addr(ether_tgt->src_addr, gC.etherAddrSc);

         arp_tgt->htype = ARP_HTYPE_ETHER;
         arp_tgt->ptype = ARP_PTYPE_IP;
         arp_tgt->hlen = ETHER_ADDR_LEN;
         arp_tgt->plen = IP_ADDR_LEN;
         cpy_ether_addr(arp_tgt->sha, gC.etherAddrSc);
         cpy_ip_addr(arp_tgt->spa, gC.ipAddrSc);

         cpy_ether_addr(ether_tgt->dest_addr, arp_hdr->sha);
         arp_tgt->oper = ARP_REPLY;
         cpy_ether_addr(arp_tgt->tha, arp_hdr->sha);
         cpy_ip_addr(arp_tgt->tpa, arp_hdr->spa);
         //sc_printf("Send ARP reply to %d.%d.%d.%d\r\n", arp_hdr->spa[0], arp_hdr->spa[1], arp_hdr->spa[2], arp_hdr->spa[3]);
         tapdev_write_to_fifo_end_of_packet(gD.send_buf, ARP_PACKET_SIZE);
         tapdev_send(ARP_PACKET_SIZE);
         gCnt.sendRate += ARP_PACKET_SIZE;
      }
      return 1;
   } else {
      gD.etherFramesOtherIpOrArp++;
      gCnt.frameRate++;
   }
   
   return 1; // indicate that we receieve something
}

Xuint8 calcBitfileBufferChecksum()
{
   struct KibFileHeader* pHeader = (void*)global_file_buffer;
   Xuint32 checklen = pHeader->headerSize + pHeader->binfileSize;
   Xuint8 res = 0;
   Xuint32 i;

   sc_printf("Checking Bitfile input buffer len = %d  \r\n", (int)checklen);
   
   if (checklen > sizeof(global_file_buffer)) {
      sc_printf("Error with length %x\r\n", checklen);
      checklen = sizeof(global_file_buffer);
   }

   for(i = 0; i < checklen; i++)
      res ^= global_file_buffer[i];

   sc_printf("pHeader->binfileSize: %d\r\n", (int)pHeader->binfileSize);
   sc_printf("pHeader->headerSize: %d\r\n", (int)pHeader->headerSize);
   sc_printf("XOR checksum is %d\r\n", (int)res);
   
   return res;
}

Xuint8 calcFileBufferChecksum()
{
   Xuint32 bufSize = *((Xuint32*) global_file_buffer) + 1024;

   if (bufSize>sizeof(global_file_buffer)) {
      sc_printf("Buffer size %u too big, error\r\n", bufSize);
      return 0;
   }
   
   Xuint32 i;
   Xuint8 res = 0;
   for(i = 0; i < bufSize; i++)
      res ^= global_file_buffer[i];

   char* filename = global_file_buffer + 4;
   
   sc_printf("File %s XOR sum  = %u\r\n", filename, res);

   return res;
}

  
 /*************************************************************************************************
 * Helper Functions
 ****************************************************************************************************/
void timer_set(XTime *timer, XTime timeoutMS)
{
   XTime_GetTime(timer);
   *timer += (timeoutMS * (XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / 1000));
}

Xuint8 timer_expired(XTime *timer)
{
   XTime time_now;
   XTime_GetTime(&time_now);
   return (time_now > *timer) ? true : false;
}

void swap_byteorder(Xuint8* target, Xuint16 bytes)
{
   Xuint16 i;
   Xuint8 a[bytes];
   memcpy(a, target, bytes);
   for(i = 0; i < bytes; i++)
      target[i] = a[bytes-1-i];
}
   

/* _____________________________________________________________________ */

#define setCurrentPacket() \
   gD.curr_pkt = (struct SysCore_Data_Packet*) (gD.buf[gD.head] + UDP_PAYLOAD_OFFSET); \
   gD.curr_pkt->pktid = gD.headid; \
   gD.curr_pkt->nummsg = 0; \
   gD.curr_target = ((Xuint8*) gD.curr_pkt) + sizeof(struct SysCore_Data_Packet); 

#define shiftToNextBuffer()                                \
   gD.numbuf++;                                            \
   gD.headid++;                                            \
   gD.head++;                                              \
   if (gD.head == gD.numalloc) gD.head = 0;                \
   if (gD.data_taking_on && (gD.numbuf > gD.high_water))   \
     gD.data_taking_on = 0;                                \
   setCurrentPacket()

#define MAX_MSG_PACKET ((MAX_UDP_PAYLOAD - sizeof(struct SysCore_Data_Packet)) / 6)

int allocateBuffers(Xuint32 maxnumalloc, int checkmem)
{
   Xuint32 n = 0, k = 0;

   Xuint32 full_udp_packet = UDP_PAYLOAD_OFFSET + MAX_UDP_PAYLOAD;

   // calculate how many full UDP packets we can allocate, 
   // taking into accoumnt array of pointers + probable small overhead
   maxnumalloc = maxnumalloc / ( full_udp_packet + 4 + 1);
   
   gD.buf = 0;      /* main buffer with all packages inside */
   gD.numalloc = 0; /* number of allocated items in buffer */
   
   gD.buf = (Xuint8**) malloc(maxnumalloc * sizeof(Xuint8*));
   if (gD.buf == NULL) {
      xil_printf("Not able to initialise BIG buffer\r\n");
      return -1;  
   }
   
   for (n=0;n<maxnumalloc;n++) {
      gD.buf[n] = (Xuint8*) malloc(full_udp_packet);
      if (gD.buf[n]==NULL) break;
      
      if (checkmem)
         for (k=0;k<full_udp_packet;k++) gD.buf[n][k] = (k % 256);
      else
         memset(gD.buf[n], 0, full_udp_packet);   
      
      gD.numalloc++;
   }
   
   if (gD.numalloc < 100) {
      xil_printf("Too few buffers are created %d\r\n", gD.numalloc);
      return -1;  
   }
   
   int totalmb = gD.numalloc * full_udp_packet / 1024 / 1024;
   
   xil_printf("Totally %d buffers %d MBytes allocated\r\n", gD.numalloc, totalmb);

   if (checkmem) {
      
      Xuint32 numerr = 0;
      for (n=0;n<gD.numalloc;n++) {
         for (k=0;k<full_udp_packet;k++) 
            if (gD.buf[n][k] != (k % 256)) numerr++;
         memset(gD.buf[n], 0, full_udp_packet);
      }
            
      xil_printf("Info: memory errors detected 0x%x\r\n", numerr);
   }
   
   return 0;
}


void resetBuffers()
{
   gD.head = 0;    /* buffer which is now is filled */
   gD.numbuf = 0;  /* number of buffers*/
   gD.headid = 0;  /* id of the buffer in the head */

   gD.high_water = gD.numalloc - 5; /* Maximum number of buffers to be used in normal situation */
   gD.low_water = gD.numalloc / 10; 
   gD.data_taking_on = 0;     /* indicate if we can take new buffer */

   gD.send_head = 0;   /* buffer which must be send next */
   gD.send_limit = 0;    /* limit for sending - can be in front of existing header */
   
   gD.resend_size = 0; 
   
   gD.last_req_pkt_id = 0;
   
   setCurrentPacket()

//   memset(gD.udpheader, 0, sizeof(gD.udpheader));      
//
//   struct ether_header *ether_hdr = (struct ether_header*) gD.udpheader; 
//   struct ip_header *ip_hdr = (struct ip_header*) (gD.udpheader + IP_HEADER_OFFSET);
//   struct udp_header *udp_hdr = (struct udp_header*) (gD.udpheader + UDP_HEADER_OFFSET);
//        
//    //ethernet header
//   ether_hdr->ether_type = ETHERTYPE_IP;
//   cpy_ether_addr(ether_hdr->dest_addr, gD.masterEther);
//   cpy_ether_addr(ether_hdr->src_addr, gC.etherAddrSc);
//
//   //ip header
//   ip_hdr->version = IP_VERSION_4;
//   ip_hdr->header_length = IP_HEADER_LENGTH;
//   ip_hdr->type_of_service = 0;//IP_DSCP_HIGHEST_PRIORITY;
//   ip_hdr->total_length = MAX_ETHER_PAYLOAD;
//   ip_hdr->packet_number_id = 0;
//   ip_hdr->flags = IP_FLAGS;
//   ip_hdr->fragment_offset = 0;
//   ip_hdr->time_to_live = IP_TTL;
//   ip_hdr->proto = IP_PROTO_UDP;
//   ip_hdr->header_chksum = 0;
//   cpy_ip_addr(ip_hdr->src, gC.ipAddrSc);
//   cpy_ip_addr(ip_hdr->dest, gD.masterIp);
//   
//   //udp header
//   udp_hdr->sport = gC.dataPort;
//   udp_hdr->dport = gD.masterDataPort;
//   udp_hdr->len = MAX_IP_PAYLOAD;
//   udp_hdr->chksum = 0;
}

void processDataRequest(struct SysCore_Data_Request* req)
{
   if (gD.daqState==0) {
      sc_printf("Request when daq is stopped !!!!\r\n");
      return;  
   }
   
   // here overflow of packet id is possible
   Xuint32 diff = req->frontpktid - gD.headid;
   Xuint32 send_limit = gD.head + diff;
   
   if (diff < 1000) {
      // this is normal situation - front send packet slightly ahead of first buffer
      while (send_limit >= gD.numalloc) send_limit -= gD.numalloc;
   } else 
   if (diff > 0x80000000) {
      // this is most probably negative value, send_limit behind current packet
      while (send_limit >= gD.numalloc) send_limit += gD.numalloc;
   } else {
      // this is error and cannot be, ignore packet completely  
      xil_printf("Error: req->frontpktid:0x%x - gD.headid:0x%x = 0x%x\r\n", req->frontpktid, gD.headid, diff);
      return;
   }
   
   gD.send_limit = send_limit;
   
//   xil_printf("Set send limit to %d\r\n",send_limit);
   
   // now calculate new buffers size
   diff = gD.headid - req->tailpktid;
   if (diff >= gD.numalloc) {
      xil_printf("Error: gD.headid - req->tailpktid = 0x%x\r\n", diff);
      return;
   }
   gD.numbuf = diff;
   
   // enable data taking again if one have already enough buffer space to fill
   if (!gD.data_taking_on)
      if ((gD.numbuf <= gD.low_water) && (gD.daqState == 1))
         gD.data_taking_on = 1;
         
   gD.last_req_pkt_id = req->reqpktid;
   
   Xuint32 cnt = req->numresend;
   
   if (cnt==0) return;

   Xuint32* resend_ids = (Xuint32*) (((Xuint8*) req) + sizeof(struct SysCore_Data_Request));
   
   Xuint32 n, found, oldsize = gD.resend_size;
   
//   xil_printf("Current head %d headid %d check %d \r\n", 
//      gD.head, gD.headid, ((struct SysCore_Data_Packet*) gD.buf[gD.head])->pktid);
   
   while (cnt--) {
      diff = gD.headid - *resend_ids;

//      xil_printf("Resubmit paket %d diff %d\r\n", *resend_ids, diff);
      
      if (diff > gD.numbuf) {
         xil_printf("Error: wrong packet id to resend 0x%x\r\n", *resend_ids);
         return;
      }
      
      Xuint32 pkt_indx = gD.head;
      while (pkt_indx<diff) pkt_indx += gD.numalloc;
      pkt_indx-=diff;

      resend_ids++;
      
      found = 0;
      // check if we had already request for that packet
      for (n=0;n<oldsize;n++)
         if (gD.resend_indx[n] == pkt_indx) {
            found = 1;
            break;   
         }
      if (found) continue;

      if (gD.resend_size >= RESEND_MAX_SIZE) {
         xil_printf("Error: too many packets to resend\r\n");
         return;
      }

//      xil_printf("Paket index %d check %d \r\n", 
//          pkt_indx, ((struct SysCore_Data_Packet*) gD.buf[pkt_indx])->pktid);

      gD.resend_indx[gD.resend_size++] = pkt_indx;
   }
}


int sendDataPacket(Xuint8* buf, Xuint32 payloadSize)
{
   struct ether_header *ether_hdr = (struct ether_header*) buf; 
   struct ip_header *ip_hdr = (struct ip_header*) (buf + IP_HEADER_OFFSET);
   struct udp_header *udp_hdr = (struct udp_header*) (buf + UDP_HEADER_OFFSET);

    //ethernet header
   ether_hdr->ether_type = ETHERTYPE_IP;
   
   cpy_ether_addr(ether_hdr->dest_addr, gD.masterEther);
   cpy_ether_addr(ether_hdr->src_addr, gC.etherAddrSc);

   //ip header
   ip_hdr->version = IP_VERSION_4;
   ip_hdr->header_length = IP_HEADER_LENGTH;
   ip_hdr->type_of_service = 0;//IP_DSCP_HIGHEST_PRIORITY;
//   ip_hdr->total_length = MAX_ETHER_PAYLOAD;
   ip_hdr->packet_number_id = 0;
   ip_hdr->flags = IP_FLAGS;
   ip_hdr->fragment_offset = 0;
   ip_hdr->time_to_live = IP_TTL;
   ip_hdr->proto = IP_PROTO_UDP;
//   ip_hdr->header_chksum = 0;
   cpy_ip_addr(ip_hdr->src, gC.ipAddrSc);
   cpy_ip_addr(ip_hdr->dest, gD.masterIp);
   
   //udp header
   udp_hdr->sport = gC.dataPort;
   udp_hdr->dport = gD.masterDataPort;
//   udp_hdr->len = MAX_IP_PAYLOAD;
   udp_hdr->chksum = 0;

//   struct ip_header *ip_hdr = (struct ip_header*) (gD.udpheader + IP_HEADER_OFFSET);
//   struct udp_header *udp_hdr = (struct udp_header*) (gD.udpheader + UDP_HEADER_OFFSET);

   while ((payloadSize + UDP_PAYLOAD_OFFSET) % 4) payloadSize+=2;

   udp_hdr->len = sizeof(struct udp_header) + payloadSize;

   ip_hdr->total_length = sizeof(struct ip_header) + udp_hdr->len;

   ip_hdr->header_chksum = 0;
   ip_hdr->header_chksum = crc16((Xuint16*) ip_hdr, sizeof(struct ip_header));

//   memcpy(buf, gD.udpheader, UDP_PAYLOAD_OFFSET);
   
   payloadSize += UDP_PAYLOAD_OFFSET;
   
   XStatus Status = XTemac_FifoWrite(&TemacInstance, buf, payloadSize, XTE_END_OF_PACKET);
   
   if (Status == XST_SUCCESS) 
      Status = XTemac_FifoSend(&TemacInstance, payloadSize);

   // do we need this ??? - anyway, it is fast   
   if (Status == XST_SUCCESS) {
      gCnt.sendRate += payloadSize;
      
      Status = TemacGetTxStatus();
      
      if (Status == XST_NO_DATA) Status = XST_SUCCESS;
   }
   
   if (Status != XST_SUCCESS) {
      xil_printf("Packet send fails\r\n");
      TemacResetDevice();
      tapdev_init();
      return 0;
   }

   return 1;
}

void addSystemMessage(Xuint8 id, int flush)
{
   gD.curr_target[0] = 0xE0 | (gC.rocNumber << 2); 
   gD.curr_target[1] = id;
   gD.curr_target[2] = 0;
   gD.curr_target[3] = 0;
   gD.curr_target[4] = 0;
   gD.curr_target[5] = 0;
   
   gD.curr_target+=6; 
     
   gD.curr_pkt->nummsg++;
   
   if ((gD.curr_pkt->nummsg >= MAX_MSG_PACKET) || flush) {
      shiftToNextBuffer()
   }
}

void doPacketFlush()
{
   // if we can send something, but didnot do this 
   // (while it was too few data or we in suspend mode)
   if (gD.send_head != gD.send_limit) {
      // than produce (flush) buffer only if few new messages 
      // are there or there is nothing at all 
      if ((gD.curr_pkt->nummsg>0) || (gD.numbuf==0)) {
         shiftToNextBuffer();
         return;
      }
   }
   
   // check if we can resend last normally send buffer
 
   // hust fo safety - should not happen while resending will not allow flushing
   if (gD.resend_size >= RESEND_MAX_SIZE) return;
   
   // if no buffers - no resend possibilities
   if (gD.numbuf == 0) return;


   Xuint32 prev_send = gD.send_head;
   if (prev_send==0) prev_send = gD.numalloc - 1; else prev_send--;
   
   // check if buffer which we want to resnd is inside buffers limits
   int inside = 0;
   
   if (gD.head >= gD.numbuf)
      inside = (prev_send < gD.head) && (prev_send >= gD.head - gD.numbuf);
   else
      inside = (prev_send < gD.head) || (prev_send >= gD.head + gD.numalloc - gD.numbuf);

   if (inside) {
      gD.resend_indx[gD.resend_size++] = prev_send;
      xil_printf("flush packet 0x%x resend\r\n", 
        ((struct SysCore_Data_Packet*) (gD.buf[prev_send] + UDP_PAYLOAD_OFFSET))->pktid);
   }
}

void mainLoop()
{
   unsigned char* target = 0;
   unsigned char nummsg = 0;

   Xuint16 oper_in_loop = 0;
   unsigned char zeros_in_loop = 0;
   
   Xuint32 data_reg1, data_reg2, data_reg3;
   
   struct SysCore_Data_Packet* sendpkt = 0;
   Xuint32 sendpayload = 0;
   Xuint32 sendid = 0;
   
   XTime curr_time, last_tmout, curr_time2, curr_time3;

   XTime_GetTime(&last_tmout);
   curr_time = last_tmout;
   curr_time2 = last_tmout;
   curr_time3 = last_tmout;

   memset(&gCnt, 0, sizeof(gCnt));
   gD.lastStatTime = last_tmout;
   
   unsigned char did_send_packet = 0;
   
   while (1) {
      
      // ____________________________ DATA TAKING ______________________
      
      zeros_in_loop = 0;
      if (gC.prefetchActive==0) {
         data_reg1 = BURST1;
         data_reg2 = BURST2;
         data_reg3 = BURST3;
         oper_in_loop = gC.burstLoopCnt;
      } else {
         data_reg1 = OCM_REG1;
         data_reg2 = OCM_REG2;
         data_reg3 = OCM_REG3;
         oper_in_loop = gC.ocmLoopCnt;
      }
      
      // first of all, take data from FPGA (if there are free buffers)
      while (gD.data_taking_on && oper_in_loop) {
         nummsg = gD.curr_pkt->nummsg;
         target = gD.curr_target;
         while (nummsg < (MAX_MSG_PACKET - 2) && --oper_in_loop) {

            *(Xuint32*)target       = XIo_In32(data_reg1);
            *(Xuint32*)(target + 4) = XIo_In32(data_reg2);
            *(Xuint32*)(target + 8) = XIo_In32(data_reg3);
            
            if (*target & 0xE0) {
               target += 6;
               nummsg++;
               zeros_in_loop = 0;
               
               if (*target & 0xE0) {
                  target += 6;
                  nummsg++;
               }
            } else 
            if (*(target + 6) & 0xE0) {
               *(Xuint32*)(target) = *(Xuint32*)(target + 6); 
               *(Xuint16*)(target + 4) = *(Xuint16*)(target + 10); 
               target += 6;
               nummsg++;
               zeros_in_loop = 0;
            } else {
               gCnt.nopRate++;
               if (zeros_in_loop++ > 3) oper_in_loop = 1; // stop loop on next iteration
            }
         }
         
         gD.curr_pkt->nummsg = nummsg;
         
         if (nummsg >= (MAX_MSG_PACKET - 2)) { 
            gCnt.dataRate += nummsg;
            shiftToNextBuffer() 
         } else {
            gD.curr_target = target;
         }
      }

      // ____________________________ DISPATCHING ______________________

      XTime_GetTime(&curr_time);
      gCnt.takePerf += (curr_time - curr_time3);
      

      // second check, if one get any packet from udp
      ethernetPacketDispatcher();

      // ____________________________ 10 ms timeout ______________________

      // Make regular actions every 10 ms
      if (curr_time - last_tmout > XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / 100) {
         // exit from mainLoop if console mode was changed
         if (gC.global_consolemode != 0) return;

         // if master for long time didnot send any commands, data requests and so on
         // just mark it as unconnected
         // if master send any new packet, it will be accepted as master again
         // any other node should do poke(ROC_MASTER_LOGIN)

         if (gD.masterConnected) {
            if (gD.masterCounter-- <=0) {
               gD.masterConnected = 0;
               gD.masterCounter = 0;
            }
         }
         
         // check if flash time expired
         if (gD.daqState && (curr_time - gD.lastFlushTime > XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / 1000 * gC.flushTimeout)) {
           
            // if we didnot send any packet during last flash timeout,
            // send either new portion of data or, if output blocked,
            // last packet again
            if (!did_send_packet) doPacketFlush();
            
            did_send_packet = 0;
            gD.lastFlushTime = curr_time;
         }
         
         if (curr_time - gD.lastStatTime > 2 * XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ) {
            
            double k = XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / (curr_time - gD.lastStatTime + 0. );
            double kperf = 100000. / (curr_time - gD.lastStatTime + 0. );
            
            gD.stat.dataRate = k* 6.* gCnt.dataRate; 
            gD.stat.sendRate = k * gCnt.sendRate;   
            gD.stat.recvRate = k * gCnt.recvRate;   
            gD.stat.nopRate = k * gCnt.nopRate;    
            gD.stat.frameRate = k * gCnt.frameRate;  
            gD.stat.takePerf = kperf * gCnt.takePerf;  
            gD.stat.dispPerf = kperf * gCnt.dispPerf;  
            gD.stat.sendPerf = kperf * gCnt.sendPerf;  
            
            if ((gC.verbose & 2) && gD.masterConnected)
               sendConsoleData(ROC_STATBLOCK, &(gD.stat), sizeof(gD.stat));
            
            memset(&gCnt, 0, sizeof(gCnt));
            gD.lastStatTime = curr_time;
         }
         
         last_tmout = curr_time;
      }

      // ____________________________ DATA SENDING ______________________

      XTime_GetTime(&curr_time2);
      gCnt.dispPerf += (curr_time2 - curr_time);

      // in the end check if one can send data packet(s)
      if ((gD.resend_size>0) ||
          ((gD.send_head != gD.head) && (gD.send_head != gD.send_limit))) {

         // define which buffer will be sended
         if (gD.resend_size > 0) 
            sendid = gD.resend_indx[gD.resend_size-1];
         else 
            sendid = gD.send_head;

         // check if we have enough place in fifo
         sendpkt = (struct SysCore_Data_Packet*) (gD.buf[sendid] + UDP_PAYLOAD_OFFSET);
         
         sendpayload = sizeof(struct SysCore_Data_Packet) + sendpkt->nummsg*6;
         
         // if we have enough fifo
         if (XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND) >= 
             (UDP_PAYLOAD_OFFSET + sendpayload + 2)) {
         
            // copy value of last request
            sendpkt->lastreqid = gD.last_req_pkt_id;
         
            if (sendDataPacket(gD.buf[sendid], sendpayload)) {
               did_send_packet = 1;
         
               // move counters forward
               if (gD.resend_size > 0) 
                  gD.resend_size--;
               else {
                  if (++gD.send_head == gD.numalloc) gD.send_head = 0;
               }
            }
         }
      }
      
      XTime_GetTime(&curr_time3);
      gCnt.sendPerf += (curr_time3 - curr_time2);
   }
}

/***************************************************************************************************
 * sc_printf implementation
 ***************************************************************************************************/  

void sc_printf(const char *fmt, ...) 
{ 
   if (gC.verbose == 0) return;

   if (!fmt || (*fmt==0)) return;
   
   va_list args; 
   va_start(args, fmt); 
   
   int result = vsnprintf(gD.debugOutput, sizeof(gD.debugOutput), fmt, args);
      
   va_end(args);
   
   if ((result<0) || (result>=sizeof(gD.debugOutput)))
     strcpy(gD.debugOutput, "Conversion error");
     
   // output on the terminal  
   if (gC.verbose & 1) 
      xil_printf(gD.debugOutput);
      
   if (gC.verbose & 2) {
     // send packet to network  
     
     if (gD.masterConnected)
        sendConsoleData(ROC_DEBUGMSG, gD.debugOutput, result+1);
   }
   
   // output to Uart ??
   if (gC.verbose & 4) {
      int i=0;  
      for (;i<result;i++) 
         XUartLite_SendByte(STDOUT_BASEADDRESS, gD.debugOutput[i]);
   }
} 


void doSeveralTests()
{
  XTime tm = 0; 
  Xuint32 xnn = 0;
  Xuint32 FifoFreeBytes = 0;
  
  xnn = 0;
  timer_set(&tm, 100);
  while (timer_expired(&tm) == false) {
     xnn++;
     FifoFreeBytes = XTemac_FifoGetFreeBytes(&TemacInstance, XTE_SEND);
  }

  xil_printf("Info: FIFO free bytes = 0x%x xnn = 0x%x\r\n", FifoFreeBytes, xnn);

  xnn = 0;
  FifoFreeBytes = 0;
  timer_set(&tm, 100);
  while (timer_expired(&tm) == false) {
     xnn++;
     XTemac_FifoRecv(&TemacInstance, &FifoFreeBytes);
  }

  xil_printf("Info: Recv FIFO size = 0x%x xnn = 0x%x\r\n", FifoFreeBytes, xnn);

  
  xnn = 0;
  timer_set(&tm, 100);
  while (timer_expired(&tm) == false) {
     xnn++;    
     XTemac_FifoRecv(&TemacInstance, &FifoFreeBytes);
  }
  xil_printf("Info: Already recv bytes = 0x%x xnn = 0x%x\r\n", FifoFreeBytes, xnn);

  xnn = 0;
  timer_set(&tm, 100);
  while (timer_expired(&tm) == false) {
     xnn++;    
     TemacGetRxStatus();
  }
  xil_printf("Info: TemacGetRxStatus in 100 ms xnn = 0x%x\r\n", xnn);

  xnn = 0;
  timer_set(&tm, 100);
  while (timer_expired(&tm) == false) {
     xnn++;    
     TemacGetTxStatus();
  }
  xil_printf("Info: TemacGetTxStatus in 100 ms xnn = 0x%x\r\n", xnn);

  xnn = 0;
  timer_set(&tm, 100);
  while (timer_expired(&tm) == false) xnn++;
  xil_printf("Info: in 100 ms get 0x%x gettime operations\r\n", xnn);
  
  xnn = tm;
  FifoFreeBytes = XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ;
  xil_printf("Info: Time 0x%x  per_sec 0x%x  sizeof %x \r\n", xnn, FifoFreeBytes, sizeof(Xuint32[40000][350]));


  XTime tm2 = 0; 
  Xuint32 burst1, burst2, burst3;
  xnn = 500000;

  XIo_Out32(PREFETCH, 0);
  XIo_In32(OCM_REG1);
  XIo_In32(OCM_REG2);
  XIo_In32(OCM_REG3);
  XIo_Out32(FIFO_RESET, 1);
  XIo_Out32(FIFO_RESET, 0);
  
  XTime_GetTime(&tm);
  while (xnn--) {  
     burst1 = XIo_In32(BURST1);
     burst2 = XIo_In32(BURST2);
     burst3 = XIo_In32(BURST3);
  }
  XTime_GetTime(&tm2);
  int rrr = 100.* (tm2-tm) / (XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / 1000);
  
  xil_printf("Info: with burst 1000000 messages can be read in %d.%02d ms\r\n", rrr / 100, rrr % 100);

  XIo_Out32(FIFO_RESET, 1);
  XIo_Out32(PREFETCH, 1);
  XIo_Out32(FIFO_RESET, 0);


   unsigned reg1 = OCM_REG1, reg2 = OCM_REG2, reg3 = OCM_REG3;

   xnn = 500000;
   XTime_GetTime(&tm);
   while (xnn--) {  
     burst1 = XIo_In32(reg1);
     burst2 = XIo_In32(reg2);
     burst3 = XIo_In32(reg3);
   }
   XTime_GetTime(&tm2);
   rrr = 100. * (tm2-tm) / (XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / 1000);
  
   xil_printf("Info: with ocm reg 1000000 messages can be read in %d.%02d ms\r\n", rrr / 100, rrr % 100);


   xnn = 500000;
   XTime_GetTime(&tm);
   while (xnn--) {  
     burst1 = XIo_In32(OCM_REG1);
     burst2 = XIo_In32(OCM_REG2);
     burst3 = XIo_In32(OCM_REG3);
   }
   XTime_GetTime(&tm2);
   rrr = 100.* (tm2-tm) / (XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ / 1000);
  
   xil_printf("Info: with ocm reg 1000000 messages can be read in %d.%02d ms\r\n", rrr / 100, rrr % 100);

   
   int kkk = MAX_UDP_PAYLOAD;
   
   xil_printf("Max udp load = %d 0x%x\r\n", kkk, MAX_UDP_PAYLOAD);
   
   
   Xuint32 v;
   Xuint32* ptr = &v;
   Xuint8* ptr8 = (Xuint8*) ptr8;
   char vchar = 0;
   
   int cnt1 = 0, cnt2 = 0, cnt3 = 0;
   
   v = 0;
   
   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 100000;
      StartProfile(msr1)
      while (cnt2--) {
         (*ptr)++;
         if (*ptr > 243) *ptr = 0;
          
      }
      StopProfile(msr1, 100, "100000 Inc via pointer") 
   }
   
   v = 0;

   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 100000;
      StartProfile(msr2)
      while (cnt2--) {
         v++;
         if (v>243) v = 0;
      }
      StopProfile(msr2, 100, "100000 direct incs") 
   }

   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 100000;
      StartProfile(msr2)
      while (cnt2--) {
         vchar++;
         if (vchar>243) vchar = 0;
      }
      StopProfile(msr2, 100, "100000 char incs") 
   }

   *ptr8 = 0xFF;
   cnt3 = 0;

   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 10000;
      StartProfile(msr3)
      while (cnt2--) if (*ptr8 >> 5) cnt3++;
      StopProfile(msr3, 100, "10000 tests via shift (8bit)") 
   }

   *ptr8 = 0xFF;
   cnt3 = 0;

   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 10000;
      StartProfile(msr4)
      while (cnt2--) if (*ptr8 & 0xE0) cnt3++;
      StopProfile(msr4, 100, "10000 tests via 'and' (8bit)") 
   }
   
   *ptr = 0xFFFFFFFF;
   cnt3 = 0;

   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 10000;
      StartProfile(msr5)
      while (cnt2--) if (*ptr & 0xE0000000) cnt3++;
      StopProfile(msr5, 100, "10000 tests via 'and' (32bit)") 
   }

   *ptr = 0xFFFFFFFF;
   cnt3 = 0;

   for (cnt1 = 0; cnt1<=100; cnt1++) {
      cnt2 = 10000;
      StartProfile(msr6)
      while (cnt2--) if (*ptr >> 29) cnt3++;
      StopProfile(msr6, 100, "10000 tests via shift (32bit)") 
   }
   
   unsigned bbb = 0x123456;
   cnt3 = 654;
   
   double fff = 3.4562;
   
   Xuint8 oldv = gC.verbose;
   gC.verbose = 1;
   
   sc_printf("My test string %d %x %s\r\n", cnt3, bbb, "ok");
   
   sc_printf("My test double %6.4f sizeof(unsigned) = %d\r\n", fff, sizeof(unsigned));

   gC.verbose = oldv;
   
   char ipaddr[] = "10.11.123.5";
   int buf[4];
   int res = sscanf(ipaddr, "%d.%d.%d.%d", buf, buf+1, buf+2, buf+3);
   xil_printf("sscanf res = %d %d.%d.%d.%d\r\n", res, buf[0], buf[1], buf[2], buf[3]);  
   res = sscanf(ipaddr, "%x.%x.%x.%x", buf, buf+1, buf+2, buf+3);
   xil_printf("hex sscanf res = %d %d.%d.%d.%d\r\n", res, buf[0], buf[1], buf[2], buf[3]);  
}

/*---------------------------------------------------*/
 
int main (void) {

   Xuint32 hwv = 0;
   Xuint32 swv = 0;

   XCache_EnableICache(CACHEABLE_AREA);
   XCache_EnableDCache(CACHEABLE_AREA);

   //for console fallback
   XUartLite uart;
   XUartLite_Initialize(&uart, XPAR_RS232_DEVICE_ID);
   XUartLite_ResetFifos(&uart);

   ///////INIT:
   hwv = XIo_In32(HW_VERSION);
   swv = SYSCORE_VERSION;
   xil_printf("\r\n\r\n");
   xil_printf("Welcome to SysCore!\r\n");
   xil_printf("Software version is %x.%x.%x.%x\r\n", ((Xuint8*)&swv)[0],((Xuint8*)&swv)[1],((Xuint8*)&swv)[2],((Xuint8*)&swv)[3]);
   xil_printf("Hardware version is %x.%x.%x.%x\r\n", ((Xuint8*)&hwv)[0],((Xuint8*)&hwv)[1],((Xuint8*)&hwv)[2],((Xuint8*)&hwv)[3]);
   
   if(((Xuint8*)&swv)[0] == ((Xuint8*)&hwv)[0] && ((Xuint8*)&swv)[1] == ((Xuint8*)&hwv)[1])
   {
      xil_printf("Hardware and Software major numbers are identical.\r\n");
      xil_printf("System is well configured!\r\n\r\n");
   }
   else
   {
      xil_printf("\r\n   ------   WARNING!!!   ------   \r\n\r\n");
      xil_printf("Hardware and Software major numbers are NOT identical.\r\n");
      xil_printf("The System is configured in an inconsistant way!!!\r\n\r\n");
   }
   
   xil_printf("Wait for init... \r\n");
   
   XIo_Out32(TP_COUNT, 2);
   
   activate_nx(0, 0, 1, 8);
   activate_nx(1, 0, 1, 9);
   activate_nx(2, 0, 2, 8);
   activate_nx(3, 0, 2, 9);
   gC.NX_Number = 0;
   
   XIo_Out32(LTS_DELAY, gC.rocLTSDelay);
   XIo_Out32(NX1_DELAY, gC.rocNX1Delay);
   XIo_Out32(NX2_DELAY, gC.rocNX2Delay);
   XIo_Out32(NX3_DELAY, gC.rocNX3Delay);
   XIo_Out32(NX4_DELAY, gC.rocNX4Delay);

   XIo_Out32(SR_INIT, gC.rocSrInit1);
   usleep(20000);
   XIo_Out32(BUFG_SELECT, gC.rocBufgSelect1);
   usleep(20000);

   XIo_Out32(SR_INIT2, gC.rocSrInit2);
   usleep(20000);
   XIo_Out32(BUFG_SELECT2, gC.rocBufgSelect1);
   usleep(20000);

   XIo_Out32(ADC_LATENCY1, gC.rocAdcLatency1);
   usleep(20000);
   XIo_Out32(ADC_LATENCY2, gC.rocAdcLatency2);
   usleep(20000);
   XIo_Out32(ADC_LATENCY3, gC.rocAdcLatency3);
   usleep(20000);
   XIo_Out32(ADC_LATENCY4, gC.rocAdcLatency4);
   usleep(20000);

   xil_printf("\r\nLoading configuration from SD card...\r\n");
   
   load_config(defConfigFileName);

   xil_printf ("\r\nStarting Reset... \r\n");
   nx_reset();

    //Set the ADC to 12 Bit output
   XIo_Out32(ADC_ADDR, 33); XIo_Out32(ADC_REG, 3);
   usleep(20000);
   XIo_Out32(ADC_ADDR, 255); XIo_Out32(ADC_REG, 1);

   XIo_Out32(ADC_ADDR2, 33); XIo_Out32(ADC_REG2, 3);
   usleep(20000);
   XIo_Out32(ADC_ADDR2, 255); XIo_Out32(ADC_REG2, 1);


   xil_printf ("\r\nEverything is set up and ready to run. \r\n");
   xil_printf ("OK.\r\n\r\n");

   //network init
   xil_printf("\r\nInit networkadapter...\r\n");
   XTime_SetTime(0);
   networkInit();   
   
   prefetch(0); // set default to slow mode
   
   if (gC.verbose==0) xil_printf("Info: Console messages are deactivated.\r\n");
   if (gC.verbose & 1) xil_printf("Info: Console messages are activated.\r\n");
   if (gC.verbose & 2) xil_printf("Info: Network messages are activated.\r\n");
   if (gC.verbose & 4) xil_printf("Info: UART messages are activated.\r\n");

//   doSeveralTests();

   allocateBuffers(0x7000000, true);
  
   resetBuffers();
   
   gD.masterConnected = 0;
   gD.masterCounter = 0; 
   gD.masterDataPort = 0;
   gD.masterControlPort = 0;
   gD.lastMasterPacketId = 0;
   
   gD.daqState = 0;
   gD.lastFlushTime = 0;

   memset(&gCnt, 0, sizeof(gCnt));
   memset(&(gD.stat), 0, sizeof(gD.stat));
   gD.lastStatTime = 0;
   
   strcpy(gD.debugOutput, "none");
   
   gD.etherFramesOtherIpOrArp = 0;

//   printTemacConfig();

   while (1) {
     if (gC.global_consolemode==1) {
        xil_printf("Info: Enter console loop...\r\n");
        consoleMainLoop();
     } else {
        xil_printf("Info: Enter main loop...\r\n");
        gC.global_consolemode = 0;
        mainLoop();
     }
     xil_printf("Info: Exit from main loop !!! (%d)\r\n", gC.global_consolemode);
   }
   
   return 0;
}

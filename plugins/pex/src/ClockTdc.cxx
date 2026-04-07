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
#include "pex/ClockTdc.h"
#include "pex/Device.h"
#include "pexor/PexorTwo.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include <unistd.h>
#include <string>
#include <vector>

namespace pex
{

const char *xmlClockTdcReadScaler = "CtdcEnableScaler";
const char *xmlClockTdcScalerChannels = "CtdcScalerChannels";

const char *xmlClockTdcClockSource = "CtdcClockSource";
const char *xmlClockTdcClockFreq = "CtdcClockFrequency";
const char *xmlClockTdcDataFormat = "CtdcDataFormat";
const char *xmlClockTdcTriggerWindowLen = "CtdcTriggerWindowLength";
const char *xmlClockTdcPreTriggerLen = "CtdcPreTriggerLength";

const char *xmlClockTdcChanMaskDisabled = "CtdcChannelMaskDisabled_";
const char *xmlClockTdcChanMaskTrigger  = "CtdcChannelMaskTrigger_";

const char *xmlClockTdcPadiThresholds = "CtdcPadiThresholds_";

const char *xmlClockTdcTestPulse="CtdcEnableTestPulse";

}

pex::ClockTdc::ClockTdc (const std::string &name, dabc::Command cmd) :
    pex::FrontendBoard::FrontendBoard (name, FEB_CTDC, cmd)
{
  DOUT2("Created new pex::ClockTdc...\n");

  fReadScalerData = Cfg (pex::xmlClockTdcReadScaler, cmd).AsBool(false);
  fCtdTestPulseOn = Cfg (pex::xmlClockTdcTestPulse, cmd).AsBool(false);

  fNumScalerChannels =Cfg (pex::xmlClockTdcScalerChannels, cmd).AsInt(131);
  fCtdcFrequency=Cfg (pex::xmlClockTdcClockFreq, cmd).AsDouble(CTDC_FREQ);
  if(fCtdcFrequency<=0) fCtdcFrequency=CTDC_FREQ;
  fCtdcClockSource=Cfg (pex::xmlClockTdcClockSource, cmd).AsInt(CTDC_CLOCK_SOURCE);
  fCtdcDataFormat=Cfg (pex::xmlClockTdcDataFormat, cmd).AsInt(CTDC_DATA_FORMAT);
  fCtdcTrigWinLen=Cfg (pex::xmlClockTdcTriggerWindowLen, cmd).AsInt(CTDC_TRIG_WIN_LEN);
  fCtdcPreTrigLen=Cfg (pex::xmlClockTdcPreTriggerLen, cmd).AsInt(CTDC_PRE_TRIG_LEN);


  // more configuration bits from arrays:
  std::vector<long int> arr;
  for (int sfp = 0; sfp < PEX_NUMSFP; sfp++)
  {

    for (int slave = 0; slave < PEX_MAXSLAVES; slave++)
        {
          arr.clear();
          arr = Cfg(dabc::format("%s%d_%X", pex::xmlClockTdcChanMaskDisabled, sfp, slave), cmd).AsIntVect();
          for (size_t i = 0; i < arr.size(); ++i) // array contains 5 blocks for channel regions
            {
              fChan_Enabled[sfp][slave].push_back(arr[i]);
            }
          arr.clear();
          arr = Cfg(dabc::format("%s%d_%X", pex::xmlClockTdcChanMaskTrigger, sfp, slave), cmd).AsIntVect();
          for (size_t i = 0; i < arr.size(); ++i) // array contains 5 blocks for channel regions
            {
              fChan_Trigger_Enabled[sfp][slave].push_back(arr[i]);
            }
          arr.clear();
          arr = Cfg(dabc::format("%s%d_%X", pex::xmlClockTdcPadiThresholds, sfp, slave), cmd).AsIntVect();
          for (size_t i = 0; i < arr.size(); ++i) // array contains one entry for each channel - 128
          {
            fThreshold[sfp][slave].push_back(arr[i]);
          }
        }    // slave
    } // sfp


    /** init trigger register addresses for the 5 channel blocks*/
    fCtdcChanEnaRegister[0] = REG_CTDC_DIS_0;
    fCtdcChanEnaRegister[1] = REG_CTDC_DIS_1;
    fCtdcChanEnaRegister[2] = REG_CTDC_DIS_2;
    fCtdcChanEnaRegister[3] = REG_CTDC_DIS_3;
    fCtdcChanEnaRegister[4] = REG_CTDC_DIS_4;
    fCtdcChanTrigEnaRegister[0] = REG_CTDC_TRIG_DIS_0;
    fCtdcChanTrigEnaRegister[1] = REG_CTDC_TRIG_DIS_1;
    fCtdcChanTrigEnaRegister[2] = REG_CTDC_TRIG_DIS_2;
    fCtdcChanTrigEnaRegister[3] = REG_CTDC_TRIG_DIS_3;
    fCtdcChanTrigEnaRegister[4] = REG_CTDC_TRIG_DIS_4;

}

pex::ClockTdc::~ClockTdc ()
{

}

int pex::ClockTdc::Disable (int , int )
{
  // not used, just for interface compat
  return 0;
}

int pex::ClockTdc::Enable (int , int )
{
  // not used, just for interface compat
  return 0;
}

int pex::ClockTdc::Configure (int sfp, int sl)
{
  int rev = 0;
  ///int value = 0;

  // get submemory addresses to check setup:
  unsigned long base_dbuf0 = 0, base_dbuf1 = 0;
  unsigned long num_submem = 0, submem_offset = 0;
  unsigned long ctdc_ctrl  = 0;
  unsigned long ctdc_del = 0, ctdc_gate = 0;
  //  unsigned long qfw_ctrl = 0;

  // needed for check of meta data, read it in any case
  //    printm ("SFP: %d, FEBEX3: %d \n", sfp, dev);
  //    // get address offset of febex buffer 0,1 for each febex/exploder
  rev = fKinpex->ReadBus (REG_BUF0, base_dbuf0, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (double buffer 0 address)\n", rev, sfp, sl, REG_BUF0);
    return -1;
  }
  DOUT1("CTDC at sfp %d slave %d: Base address for Double Buffer 0  0x%lx  \n", sfp, sl, base_dbuf0);
  rev = fKinpex->ReadBus (REG_BUF1, base_dbuf1, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (double buffer 0 address)\n", rev, sfp, sl, REG_BUF1);
    return -1;
  }
  DOUT1("CTDC at sfp %d slave %d: Base address for Double Buffer 1  0x%lx  \n", sfp, sl, base_dbuf1);

  //
  //    // get nr. of channels per tamex
  rev = fKinpex->ReadBus (REG_SUBMEM_NUM, num_submem, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x ((num submem)\n", rev, sfp, sl, REG_SUBMEM_NUM);
    return -1;
  }
  DOUT1("CTDC at sfp %d slave %d: Number of TDCs: 0x%lx  \n", sfp, sl, num_submem);

  //    // get buffer per channel offset
  rev = fKinpex->ReadBus (REG_SUBMEM_OFF, submem_offset, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (submem_offset)\n", rev, sfp, sl, REG_SUBMEM_OFF);
    return -1;
  }
  DOUT1("CTDC at sfp %d slave %d:  Offset of TDCs to the Base address: 0x%lx  \n", sfp, sl, submem_offset);



  // set clock source
  if (fCtdcClockSource == 0xC)
       {
         if (sl == 0)
         {
           rev = fKinpex->WriteBus (REG_CTDC_TIME_RST, fCtdcClockSource, sfp, sl);
           if (rev)
            {
              EOUT("\n\nError %d - CTDC setting clock source failed for slave (%d,%d) \n ", rev, sfp, sl);
              return rev;
            }
         }
         else
         {
           rev = fKinpex->WriteBus (REG_CTDC_TIME_RST, 0x0, sfp, sl);
           if (rev)
           {
             EOUT("\n\nError %d - CTDC setting clock source failed for slave (%d,%d) \n ", rev, sfp, sl);
             return rev;
           }
         }
       }
       else
       {
           rev = fKinpex->WriteBus (REG_CTDC_TIME_RST, fCtdcClockSource, sfp, sl);
           if (rev)
           {
             EOUT("\n\nError %d - CTDC setting clock source failed for slave (%d,%d) \n ", rev, sfp, sl);
             return rev;
           }
       }
       usleep(100);


       // disable test data length
       rev = fKinpex->WriteBus (REG_DATA_LEN, 0x10000000, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d - CTDC disabling test data length failed for slave (%d,%d) \n ", rev, sfp, sl);
         return rev;
       }

       // disable trigger acceptance in CTDC
       rev = fKinpex->WriteBus (REG_CTDC_CTRL, 0, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d - CTDC write REG_CTDC_CTRL failed for slave (%d,%d) \n ", rev, sfp, sl);
         return rev;
       }
       rev = fKinpex->ReadBus (REG_CTDC_CTRL, ctdc_ctrl, sfp, sl);
        if ( rev || (ctdc_ctrl & 0x1) != 0)
        {

          EOUT("\n\ndisabling trigger acceptance in CTDC failed for sfp:%d slave %x \n", sfp, sl);
          return -1;
        }

        // enable trigger acceptance in CTDC
        rev = fKinpex->WriteBus (REG_CTDC_CTRL, fCtdcDataFormat, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d - CTDC write REG_CTDC_CTRL failed for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }
        rev = fKinpex->ReadBus (REG_CTDC_CTRL, ctdc_ctrl, sfp, sl);
        if ( rev || (ctdc_ctrl & 0x7) != (unsigned long) fCtdcDataFormat)
        {
          EOUT("\n\nenabling data format in CTDC failed for sfp:%d slave %x \n", sfp, sl);
          return -1;
        }



        // set trigger delay
        rev = fKinpex->WriteBus (REG_CTDC_DEL, fCtdcTrigWinLen - fCtdcPreTrigLen, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d - CTDC write REG_CTDC_DEL failed for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }


        // set trigger window length
        rev = fKinpex->WriteBus (REG_CTDC_GATE, fCtdcTrigWinLen, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d - CTDC write REG_CTDC_GATE failed for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }

        rev = fKinpex->ReadBus (REG_CTDC_DEL, ctdc_del, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (REG_CTDC_DEL address)\n", rev, sfp, sl, REG_CTDC_DEL);
          return rev;
        }

        rev = fKinpex->ReadBus (REG_CTDC_GATE, ctdc_gate, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (REG_CTDC_GATE address)\n", rev, sfp, sl, REG_CTDC_GATE);
          return rev;
        }

        DOUT0("trigger window: before trigger: %5.0f ns \n", (float)(ctdc_gate - ctdc_del) * 1000./fCtdcFrequency);
        DOUT0("                after  trigger: %5.0f ns \n", (float)(ctdc_del)               * 1000./fCtdcFrequency);
        DOUT0("                total:        : %5.0f ns \n", (float)(ctdc_gate)              * 1000./fCtdcFrequency);
        DOUT0("\n");

        // enable tdc channels
        for (int k=0; k<5; k++)
            {
              //printm ("ena reg: 0x%x, ena val: 0x%x, ", l_ctdc_chan_ena_reg[l_k], l_ctdc_chan_ena[sfp][dev][l_k]);
              int enab = 0x0;
              size_t kanal = k;
               if (fChan_Enabled[sfp][sl].size() > kanal)    // check if we got a configuration from file
               {
                          enab = fChan_Enabled[sfp][sl].at(kanal);
               }
               rev = fKinpex->WriteBus (fCtdcChanEnaRegister[k], enab, sfp, sl);
               if (rev)
                {
                 EOUT("\n\nError %d - CTDC write channel enable failed for slave (%d,%d), channel block %d \n ", rev, sfp, sl, k);
                 return rev;
                }

               enab = 0x0;
               if (fChan_Trigger_Enabled[sfp][sl].size() > kanal)    // check if we got a configuration from file
               {
                 enab = fChan_Trigger_Enabled[sfp][sl].at(kanal);
               }
               rev = fKinpex->WriteBus (fCtdcChanTrigEnaRegister [k], enab, sfp, sl);
               if (rev)
               {
                 EOUT("\n\nError %d - CTDC write trigger enable failed for slave (%d,%d), channel block %d \n ", rev, sfp, sl, k);
                 return rev;
               }

            } // for (int k=0; k<5; k++)


            // write SFP id for channel header
          rev = fKinpex->WriteBus (REG_HEADER, sfp, sfp, sl);
            if (rev)
            {
              EOUT("\n\nError %d - CTDC slave write REG_HEADER failed for slave (%d,%d)\n ", rev, sfp, sl);
              return rev;
            }



            // reset gosip token waiting bits // shizu
            rev = fKinpex->WriteBus (REG_RST, 1, sfp, sl);
            if (rev)
            {
              EOUT("\n\nError %d - CTDC slave write REG_RST failed for slave (%d,%d)\n ", rev, sfp, sl);
              return rev;
            }



            // set padi thresholds


            for (int l_k=0; l_k<CTDC_N_PADI_FE; l_k++)
            {
              for (int l_l=0; l_l<CTDC_N_PADI_CHA; l_l++)
              {
                // must set threshholds always for channel n of both padi chips per frontend
                int l_dat1 = 0x80008000;                // write enable bit for both padi on frontends
                int l_dat2 = (l_l << 10) | (l_l << 26); // set address for padi channels n on both padis of a frontend
                int l_dat3=CTDC_THRESH_DEFAULT;
                size_t chmax=(l_k*16)+l_l+8;
                if (fThreshold[sfp][sl].size() > chmax)    // check if we got a configuration from file
                  {

                    l_dat3 =   fThreshold[sfp][sl].at(l_k*16+l_l)             // set threshold for cha n
                       + (fThreshold[sfp][sl].at((l_k*16+ l_l + 8)) << 16);  // and cha n+8
                  }
                int l_dat  = l_dat1 | l_dat2 | l_dat3;
                //printm ("SFP %d CTDC id %2d PFE %2d PCHA %d: data: 0x%x\n", sfp, dev, l_k, l_l, l_dat);
                rev = fKinpex->WriteBus (REG_CTDC_PADI_SPI_DATA, l_dat, sfp, sl);
                if (rev)
                {
                  EOUT("\n\nError %d - SFP %d CTDC id %d PADI FE %d PADI_CH %d writing to REG_CTDC_PADI_SPI_DATA failed\n ", rev, sfp, sl, l_k, l_l);
                  return rev;
                }
                l_dat = 0x1 << l_k;
                //printm ("SFP %d CTDC id %2d PFE %2d PCHA %d: ctrl: 0x%x\n", sfp, dev, l_k, l_l, l_dat);
                rev = fKinpex->WriteBus (REG_CTDC_PADI_SPI_CTRL, l_dat, sfp, sl);
                  if (rev)
                  {
                    EOUT("\n\nError %d - SFP %d CTDC id %d PADI FE %d PADI_CH %d writing to REG_CTDC_PADI_SPI_CTRL failed\n ", rev, sfp, sl, l_k, l_l);
                    return rev;
                  }
              }
            }

        // Calibration pulse on/off
        //REG_CTDC_CAL_PULSE
        if(fCtdTestPulseOn)
          {

            DOUT1("TEST MODE with TEST PULSE ON> Internal signals by the oscillator on board are connected to TDC input!!\n");
            rev = fKinpex->WriteBus (REG_CTDC_CAL_PULSE, 1, sfp, sl);
            if (rev)
            {
              EOUT("\n\nError %d - Set TDC test pulse failed for slave (%d,%d)\n ", rev, sfp, sl);
              return rev;
            }
          }
        else
          {
            DOUT1("TEST PULSE OFF>> TDC inputs are connected to CTDC!!\n");
            rev = fKinpex->WriteBus (REG_CTDC_CAL_PULSE, 0, sfp, sl);
            if (rev)
            {
              EOUT("\n\nError %d - Set TDC test pulse failed for slave (%d,%d)\n ", rev, sfp, sl);
              return rev;
            }
          }

  return rev;
}


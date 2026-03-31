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
#include "pex/Tamex.h"
#include "pex/Device.h"
#include "pexor/PexorTwo.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include <unistd.h>
#include <string>
#include <vector>

namespace pex
{
const char *xmlTamexType = "TamexType";

const char *xmlTamexInitModulesFlag = "TamexDoInitModules";

const char *xmlTamexSetPadiThres = "TamexSetPadiThresh";
const char *xmlTamexPadiThres = "TamexPadiThreshDefault";

const char *xmlTamexSetPQDCThres = "TamexSetPQDCThresh";
const char *xmlTamexPQDCThres = "TamexPQDCThreshDefault";

const char *xmlTamexPreTrigTime = "TamexPreTriggerTime";
const char *xmlTamexPostTrigTime = "TamexPostTriggerTime";

const char *xmlTamexChmaskEnab = "TamexChmaskEnabled";
const char *xmlTamexChmaskTrig = "TamexChmaskTrigger";
const char *xmlTamexChmaskPol = "TamexChmaskPolarity";
const char *xmlTamexDataRedEnabl = "TamexDataReductionEnabled";

const char *xmlTamexClkTamex2 = "TamexClockTam2";
const char *xmlTamexClkTamex3 = "TamexClockTam3";
const char *xmlTamexClkTamex4 = "TamexClockTam4Padi";

const char *xmlTamexTestpulseOn = "TamexTestPulseOn";
const char *xmlTamexTestpulseDelay = "TamexTestPulseDelay";
const char *xmlTamexTestpulseFreq = "TamexTestPulseFrequency";
const char *xmlTamexTestpulseChmask = "TamexChmaskTestPulse";

// these were defines before JAM26:
const char *xmlTamexEnableTriggerWindow = "TamexEnableTriggerWindow";
const char *xmlTamexLongTriggerWindow = "TamexUseLongTriggerWindow";
const char *xmlTamexEnableRefChannel = "TamexEnableReferenceChannel";
const char *xmlTamexCombineTrigger = "TamexCombinePadiTriggerOr";
const char *xmlTamexEnableOr = "TamexEnableOrTamex2";

}

pex::Tamex::Tamex (const std::string &name, dabc::Command cmd) :
    pex::FrontendBoard::FrontendBoard (name, FEB_TAMEX, cmd)
{
  DOUT2("Created new pex::Tamex...\n");

  fPostTrigTime = Cfg (pex::xmlTamexPostTrigTime, cmd).AsInt (POST_TRIG_TIME);
  fPreTrigTime = Cfg (pex::xmlTamexPreTrigTime, cmd).AsInt (PRE_TRIG_TIME);
  fDataReduction = Cfg (xmlTamexDataRedEnabl, cmd).AsInt (DATA_REDUCTION);
  fTdcChPolarity = Cfg (pex::xmlTamexChmaskPol, cmd).AsInt (TDC_CH_POL);
  fEnabledTdcCh = Cfg (pex::xmlTamexChmaskEnab, cmd).AsInt (EN_TDC_CH);
  fEnabledTrigCh = Cfg (pex::xmlTamexChmaskTrig, cmd).AsInt (EN_TRIG_CH);
  fClkSrcTdcTam2 = Cfg (pex::xmlTamexClkTamex2, cmd).AsInt (CLK_SRC_TDC_TAM2);
  fClkSrcTdcTam3 = Cfg (pex::xmlTamexClkTamex3, cmd).AsInt (CLK_SRC_TDC_TAM3);
  fClkSrcTdcTam4Padi = Cfg (pex::xmlTamexClkTamex4, cmd).AsInt (CLK_SRC_TDC_TAM4_PADI);
  fTestPulseOn = Cfg (pex::xmlTamexTestpulseOn, cmd).AsInt (TEST_PULSE_ON);
  fTestPulseCh = Cfg (pex::xmlTamexTestpulseChmask, cmd).AsInt (TEST_PULSE_CH);
  fTestPulseDelay = Cfg (pex::xmlTamexTestpulseDelay, cmd).AsInt (TEST_PULSE_DELAY);
  fTestPulseFreq = Cfg (pex::xmlTamexTestpulseFreq, cmd).AsInt (TEST_PULSE_FREQ);

  // more configuration bits from arrays:
  std::vector<long> arr;
  arr.clear ();
  arr = Cfg (pex::xmlTamexType, cmd).AsIntVect ();
  for (size_t i = 0; i < arr.size (); ++i)
  {
    if (i >= PEX_NUMSFP)
      break;
    fTamexType[i] = (tamex_type_t) arr[i];
  }

  fSetPadiThresholdsAtInit = Cfg (xmlTamexSetPadiThres, cmd).AsBool (SET_PADI_TH_AT_INIT);

  /* default Padi threshold*/
  fPadiThreshold = Cfg (xmlTamexSetPadiThres, cmd).AsInt (PADI_DEF_TH);

  fSetPqdcThresholdsAtInit = Cfg (xmlTamexSetPQDCThres, cmd).AsBool (SET_PQDC_TH_AT_INIT);
  fPqdcThreshold = Cfg (pex::xmlTamexPQDCThres, cmd).AsDouble (PQDC_DEF_TH_MV);
  fInitializeTamexModules = Cfg (xmlTamexInitModulesFlag, cmd).AsBool (true);

#ifdef  EN_REF_CH
  bool useref = true;
#else
  bool useref=false;
#endif
  fEnableReferenceChannel = Cfg (pex::xmlTamexEnableRefChannel, cmd).AsBool (useref);

  fEnableTriggerWindow = Cfg (pex::xmlTamexEnableTriggerWindow, cmd).AsBool (true);
  fLongTriggerWindow = Cfg (pex::xmlTamexLongTriggerWindow, cmd).AsBool (LONG_TRIGGER_WINDOW);
  fCombinePadiTrigger = Cfg (pex::xmlTamexCombineTrigger, cmd).AsBool (COMBINE_TRIG);
  fEnableOrTamex2 = Cfg (pex::xmlTamexEnableOr, cmd).AsBool (ENABLE_OR_TAM2);

  if (fLongTriggerWindow)
  {
    fTriggerWindow = (fPostTrigTime << 16) + fPreTrigTime;
  }
  else
  {
    fTriggerWindow = (fPostTrigTime << 16) + fPreTrigTime;
    if (fEnableTriggerWindow)
      fTriggerWindow |= (1 << 31);
  }
}

pex::Tamex::~Tamex ()
{

}

int pex::Tamex::Disable (int , int )
{
  // not used, just for interface compat
  return 0;
}

int pex::Tamex::Enable (int , int )
{
  // not used, just for interface compat
  return 0;
}

int pex::Tamex::Configure (int sfp, int sl)
{
  int rev = 0;
  ///int value = 0;

  // get submemory addresses to check setup:
  unsigned long base_dbuf0 = 0, base_dbuf1 = 0;
  unsigned long num_submem = 0, submem_offset = 0;
  unsigned long tam_rst_stat = 0;
//    unsigned long tam_ctrl = 0, feb_time = 0;
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
  DOUT1("TAMEX at sfp %d slave %d: Base address for Double Buffer 0  0x%lx  \n", sfp, sl, base_dbuf0);
  rev = fKinpex->ReadBus (REG_BUF1, base_dbuf1, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (double buffer 0 address)\n", rev, sfp, sl, REG_BUF1);
    return -1;
  }
  DOUT1("TAMEX at sfp %d slave %d: Base address for Double Buffer 1  0x%lx  \n", sfp, sl, base_dbuf1);

  //
  //    // get nr. of channels per tamex
  rev = fKinpex->ReadBus (REG_SUBMEM_NUM, num_submem, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x ((num submem)\n", rev, sfp, sl, REG_SUBMEM_NUM);
    return -1;
  }
  DOUT1("TAMEX at sfp %d slave %d: Number of TDCs: 0x%lx  \n", sfp, sl, num_submem);

  //    // get buffer per channel offset
  rev = fKinpex->ReadBus (REG_SUBMEM_OFF, submem_offset, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (submem_offset)\n", rev, sfp, sl, REG_SUBMEM_OFF);
    return -1;
  }
  DOUT1("TAMEX at sfp %d slave %d:  Offset of TDCs to the Base address: 0x%lx  \n", sfp, sl, submem_offset);

//    // reset gosip token waiting bits // shizu
  rev = fKinpex->WriteBus (REG_RST, 0x1, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d - reset gosip token waiting bits failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  rev = fKinpex->ReadBus (REG_RST, tam_rst_stat, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (gosip reset)\n", rev, sfp, sl, REG_RST);
    return -1;
  }
  DOUT1("TAMEX at sfp %d slave %d:  Status DRDY: %ld  \n", sfp, sl, tam_rst_stat);

  //
//    // disable test data length
  ////    // disable test data length
  rev = fKinpex->WriteBus (REG_DATA_LEN, 0x10000000, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX disabling test data length failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // disable/enable data reduction // shizu
  rev = fKinpex->WriteBus (REG_DATA_REDUCTION, fDataReduction, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX setting data reduction failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // disable buffers to record // shizu
  rev = fKinpex->WriteBus (REG_MEM_DISABLE, DISABLE_CHANNEL, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX disabling buffers to record failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  //
//    // write SFP id for TDC header
  rev = fKinpex->WriteBus (REG_HEADER, sfp, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX  write REG_HEADER  failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  //
//    // Set PADI default thresholds

  if (fSetPadiThresholdsAtInit)
  {
    if ((fTamexType[sfp] == TAMEX2_PADI) || (fTamexType[sfp] == TAMEX4_PADI))    // TAMEX2+PADI & TAMEX-PADI1
    {

      rev = fKinpex->WriteBus (REG_TAM_SPI_DAT, fPadiThreshold, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PADI failed 1 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0x1, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PADI failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0x0, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PADI failed 3 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

    }
  }

  //    // Set PQDC default thresholds
  if (fSetPqdcThresholdsAtInit)
  {
    if (fTamexType[sfp] == TAMEX4_PQDC1)    // TAMEX4 PQDC1
    {
      int l_pqdc_th = (int) (((1100.0 - fPqdcThreshold) / 3300.0) * 65535.0);
      // Channel 1 / 4 on all 4 FPGAS
      // printm (RON"DEBUG>>"RES" SPI DATA: 0x%x \n", (0x800000 | l_pqdc_th));

      rev = fKinpex->WriteBus (REG_TAM_SPI_DAT, 0x800000 | l_pqdc_th, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed  for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf1, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf0, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 3 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      usleep (100000);
      // Channel 2 / 4 on all 4 FPGAS

      rev = fKinpex->WriteBus (REG_TAM_SPI_DAT, 0x810000 | l_pqdc_th, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed  for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf1, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf0, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 3 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

      usleep (100000);

      // Channel 3 / 4 on all 4 FPGAS
      rev = fKinpex->WriteBus (REG_TAM_SPI_DAT, 0x820000 | l_pqdc_th, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 1  for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf1, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf0, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 3 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      usleep (100000);

      // Channel 4 / 4 on all 4 FPGAS

      rev = fKinpex->WriteBus (REG_TAM_SPI_DAT, 0x830000 | l_pqdc_th, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 1  for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf1, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      rev = fKinpex->WriteBus (REG_TAM_SPI_CTL, 0xf0, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting PQDC thresholds failed 3 for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
    }
  }    // if(fSetPqdcThresholdsAtInit)

  // Set TAMEX registers
  if (fInitializeTamexModules)
  {
    if ((fTamexType[sfp] == TAMEX2_PASSIVE) || (fTamexType[sfp] == TAMEX2_PADI))    // Set TAMEX2 clock source
    {
      if (fClkSrcTdcTam2 == 0x22)    // special case clock distribution via TRBus
      {
        rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, 0x21, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }
      }
      else
      {
        rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, fClkSrcTdcTam2, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }
      }

      if ((fClkSrcTdcTam2 == 0x22) && (sl == 0))    // If clock from TRBus used
      {
        rev = fKinpex->WriteBus (REG_TAM_BUS_EN, 0x80, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }

      }

    }
    else if ((fTamexType[sfp] == TAMEX4_PADI) || (fTamexType[sfp] == TAMEX4_PQDC1))    // Set TAMEX-PADI1 / TAMEX4 clock source
    {
      if (fClkSrcTdcTam4Padi == 0x4)    // First module clock master for crate with local oscillator
      {
        if (sfp == 0)
        {

          rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, 0x1, sfp, sl);
          if (rev)
          {
            EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
            return rev;
          }
        }
        else
        {
          rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, 0x0, sfp, sl);
          if (rev)
          {
            EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
            return rev;
          }
        }
      }
      else if (fClkSrcTdcTam4Padi == 0x8)    // First module clock master for crate with clock from front panel
      {
        if (sfp == 0)
        {
          rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, 0x2, sfp, sl);
          if (rev)
          {
            EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
            return rev;
          }
        }
        else
        {
          rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, 0x0, sfp, sl);
          if (rev)
          {
            EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
            return rev;
          }
        }
      }
      else
      {
        rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, fClkSrcTdcTam4Padi, sfp, sl);    // Set same clock source on all modules
        if (rev)
        {
          EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }
      }
    }
    else if (fTamexType[sfp] == TAMEX3)    // Set TAMEX3 clock source
    {
      rev = fKinpex->WriteBus (REG_TAM_CLK_SEL, fClkSrcTdcTam3, sfp, sl);    // Set same clock source on all modules
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting clock source failed  for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
      if ((fClkSrcTdcTam3 == 0x22) && (sfp == 0))    // If clock from TRBus used
      {
        rev = fKinpex->WriteBus (REG_TAM_BUS_EN, 0x80, sfp, sl);    // Set same clock source on all modules
        if (rev)
        {
          EOUT("\n\nError %d TAMEX  Enabling TRBus CLK on slave 0 failed  for slave (%d,%d) \n ", rev, sfp, sl);
          return rev;
        }

      }

    }
    // JAM2026- in dabc we do not re-initialize automatically after error yet
#ifdef DONT_RE_INITIALIZE_MODULES_AFTER_ERROR  // Henning
    fInitializeTamexModules = false;
#endif
  }

  DOUT0("Set TAMEX trigger window: 0x%x   for slave (%d,%d) \n ", rev, sfp, sl);
  rev = fKinpex->WriteBus (REG_TAM_TRG_WIN, fTriggerWindow, sfp, sl);    // Set trigger window
  if (rev)
  {
    EOUT("\n\nError %d TAMEX  Setting TDC trigger window failed  for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  int l_tam_fifo_almost_full_sh = FIFO_ALMOSTFULL_TH << 16;    /// shizu 07.02.2021
  int l_enable_or = 0, l_combine_trig = 0;
  if (fEnableOrTamex2)
    l_enable_or = 1 << 29;
  if (fCombinePadiTrigger)
    l_combine_trig = 1 << 28;

  if (fEnableReferenceChannel)
  {

    rev = fKinpex->WriteBus (REG_TAM_CTRL, 0x20d0 | l_tam_fifo_almost_full_sh, sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d TAMEX  TDC reset failed  for slave (%d,%d) \n ", rev, sfp, sl);
      return rev;
    }

    if ((fTamexType[sfp] == TAMEX2_PASSIVE) || (fTamexType[sfp] == TAMEX2_PADI) || (fTamexType[sfp] == TAMEX4_PADI)
        || (fTamexType[sfp] == TAMEX4_PQDC1))    // TAMEX2 & TAMEX-PADI1 & TAMEX4
    {
      // clear reset & set CNTRL_REG (CH0 enabled)

      rev = fKinpex->WriteBus (REG_TAM_CTRL, 0x20c0 | l_enable_or | l_combine_trig | l_tam_fifo_almost_full_sh, sfp,
          sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting TDC control register failed for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
    }
    else if (fTamexType[sfp] == TAMEX3)                             // TAMEX3
    {
      // clear reset & set CNTRL_REG (CH0 enabled)

      rev = fKinpex->WriteBus (REG_TAM_CTRL, 0x20c0 | l_tam_fifo_almost_full_sh, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting TDC control register failed for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
    }
  }
  else    //if(fEnableReferenceChannel)

  {

    rev = fKinpex->WriteBus (REG_TAM_CTRL, 0x2050 | l_tam_fifo_almost_full_sh, sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d TAMEX  TDC reset failed for slave (%d,%d) \n ", rev, sfp, sl);
      return rev;
    }

    if ((fTamexType[sfp] == TAMEX2_PASSIVE) || (fTamexType[sfp] == TAMEX2_PADI) || (fTamexType[sfp] == TAMEX4_PADI)
        || (fTamexType[sfp] == TAMEX4_PQDC1))    // TAMEX2 & TAMEX-PADI1 & TAMEX4
    {
      // clear reset & set CNTRL_REG
      rev = fKinpex->WriteBus (REG_TAM_CTRL, 0x2040 | l_enable_or | l_combine_trig | l_tam_fifo_almost_full_sh, sfp,
          sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting TDC control register failed for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }
    }
    else if (fTamexType[sfp] == TAMEX3)                             // TAMEX3
    {
      // clear reset & set CNTRL_REG
      //          l_stat = f_pex_slave_wr (sfp, dev, REG_TAM_CTRL, 0x7c2040);

      rev = fKinpex->WriteBus (REG_TAM_CTRL, 0x2040 | l_tam_fifo_almost_full_sh, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d TAMEX  Setting TDC control register failed for slave (%d,%d) \n ", rev, sfp, sl);
        return rev;
      }

    }
  }    ////if(fEnableReferenceChannel)
       //
  rev = fKinpex->WriteBus (REG_TAM_EN_CH, fEnabledTdcCh, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX  Enabling TDC channels failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  rev = fKinpex->WriteBus (REG_TAM_EN_TR, fEnabledTrigCh, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX   Writing TRIG enable register failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  rev = fKinpex->WriteBus (REG_TAM_POL_TDC, fTdcChPolarity, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX Writing TDC CH polarity register failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  rev = fKinpex->WriteBus (REG_TAM_POL_TRG, fTdcChPolarity, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d TAMEX Writing Self-Trigger CH polarity register failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  // shizu
  rev = fKinpex->ReadBus (REG_RST, tam_rst_stat, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (gosip reset)\n", rev, sfp, sl, REG_RST);
    return -1;
  }
  DOUT1("TAMEX at sfp %d slave %d:  Status DRDY: %ld  \n", sfp, sl, tam_rst_stat);

  if (fTestPulseOn == 1)
  {
    DOUT1(
        "TEST MODE with TEST PULSE ON>> Internal signals by the oscillator on board are connected to TDC input!! %x\n",
        ((fTestPulseOn << 4) | fTestPulseDelay));

    rev = fKinpex->WriteBus (REG_TAM_MISC1, fTestPulseCh, sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d TAMEX Set TDC test pulse failed 1 for slave (%d,%d) \n ", rev, sfp, sl);
      return rev;
    }

    rev = fKinpex->WriteBus (REG_TAM_MISC2, ((fTestPulseFreq << 5) | (fTestPulseOn << 4) | fTestPulseDelay), sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d TAMEX Set TDC test pulse failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
      return rev;
    }

  }
  else
  {
    rev = fKinpex->WriteBus (REG_TAM_MISC1, 0, sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d TAMEX Disable TDC test pulse failed 1 for slave (%d,%d) \n ", rev, sfp, sl);
      return rev;
    }
    rev = fKinpex->WriteBus (REG_TAM_MISC2, 0, sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d TAMEX Diable TDC test pulse failed 2 for slave (%d,%d) \n ", rev, sfp, sl);
      return rev;
    }
  }
  return rev;
}


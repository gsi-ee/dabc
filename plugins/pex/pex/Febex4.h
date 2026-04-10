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
#ifndef PEX_Febex4
#define PEX_Febex4

#include "pex/Febex3.h"
//#include "pex/Device.h"

#include <vector>
#include <string>

//////////////
//// definitions taken from murx framework so far:

//#define REG_FEB_CTRL       0x200000
//#define REG_FEB_TRIG_DELAY 0x200004
//#define REG_FEB_TRACE_LEN  0x200008
//#define REG_FEB_SELF_TRIG  0x20000C
//#define REG_FEB_STEP_SIZE  0x200010
//#define REG_FEB_SPI        0x200014
//#define REG_FEB_TIME       0x200018
//#define REG_FEB_XXX        0x20001C
//
//#define ADC_FIX_SET        0xd41
//#define ADC_FIX_VAL        0x800

//#define ENERGY_SUM_A_REG  0x208090
//#define ENERGY_GAP_REG    0x2080A0
//#define ENERGY_SUM_B_REG  0x2080B0

//#define DATA_FILT_CONTROL_REG 0x2080C0
//#define DATA_FILT_CONTROL_DAT 0x80         // (0x80 E,t summary always +  data trace                 always
//// (0x82 E,t summery always + (data trace + filter trace) always
//// (0x84 E,t summery always +  data trace                 if > 1 hit
//// (0x86 E,t summery always + (data trace + filter trace) if > 1 hit
//// Trigger/Hit finder filter
//
//#define TRIG_SUM_A_REG    0x2080D0
//#define TRIG_GAP_REG      0x2080E0
//#define TRIG_SUM_B_REG    0x2080F0



//#define REG_FEB4_CTRL          0x200000
//#define REG_FEB4_TRIG_DELAY    0x200004
//#define REG_FEB4_TRACE_LEN     0x200008
//#define REG_FEB4_SELF_TRIG     0x20000C
//#define REG_FEB4_STEP_SIZE     0x200010
//#define REG_FEB4_SPI           0x200014
//#define REG_FEB4_TIME          0x200018
//#define REG_FEB4_XXX           0x20001C


#define REG_TIME_RST          0x200020



#define FEB4_GOSIP_DATA_WRITE      0x208010
#define FEB4_GOSIP_DATA_READ       0x208020

//#define FEB4_TRIG_SUM_A_REG        0x2080D0
//#define FEB4_TRIG_GAP_REG          0x2080E0
//#define FEB4_TRIG_SUM_B_REG        0x2080F0
//
//#define FEB4_ENERGY_SUM_A_REG      0x208090
//#define FEB4_ENERGY_GAP_REG        0x2080A0
//#define FEB4_ENERGY_SUM_B_REG      0x2080B0
//
//#define FEB4_DATA_FILT_CONTROL_REG 0x2080C0

#define FEB4_IVANS_NEWCODE
#ifdef FEB4_IVANS_NEWCODE
//--------------------------------------------------------------------------------------------------------
// I. Rusanov : 13.02.2025
//--------------------------------------------------------------------------------------------------------
#define FEB4_TH_REG_FOR_ETF_TRIGGER 0x208074    // New address to w/r threshold by using
                        // of ETF filter for self trriggering.
//
#endif

#define FEB4_CODING  0x9801400  // switch to offset binary output mode (0x980: header of read register; 0x14: address, 0x00: data)

#define FEB4_ADC0_CLK_PHASE     0x23001607
#define FEB4_ADC1_CLK_PHASE     0x23001607


#define FEB4_DATA_FILT_CONTROL_RST     0x000000
#define FEB4_ADC_SAMPLES_SKIPPING     0x10     // one 8-bit word from 0x01 to 0xFF
#define FEB4_REG_ADC_SAMPLES_SKIPPING  0x208074

#define FEB4_ADC_FIX_CODE             0x23000d40
// threshold, to be set for each channel of all slaves at 4 spfs
// max value 0x1FF 255 (adc counts)

#define FEBEX_ETFTHRESH_DEFAULT 0x001fff



//////////////////////////////////////////////////////////////////////////

namespace pex
{

 extern const char *xmlAdcSamplesSkipping;
 extern const char *xmlEnableEtfTrigger;
 extern const char *xmlChannelEtfThreshold;


/**
 * JAM 10.04.2026 -
 *
 * Frontend board implementation for FEBEX4 - prototype
 * */

class Febex4: public pex::Febex3
{
  //friend class Device;

public:
  Febex4(const std::string &name, dabc::Command cmd);
  virtual ~Febex4();

protected:

  /** number of samples to skip, one 8-bit word from 0x01 to 0xFF*/
  int fSkippedSamples;

#ifdef FEB4_IVANS_NEWCODE
  /** if true, use "Ivans new code" for etf trigger */
  bool fUseEtfTrigger;

  /** "Ivans new code:" etf_thresh - 12.03.2024 */
  std::vector<int> fEtfThreshold[PEX_NUMSFP][PEX_MAXSLAVES];
#endif

  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure(int sfp, int slave) override;

};

}

#endif

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
#ifndef PEX_Febex3
#define PEX_Febex3

#include "pex/FrontendBoard.h"
//#include "pex/Device.h"

#include <vector>
#include <string>

//////////////
//// definitions taken from murx framework so far:

#define REG_FEB_CTRL       0x200000
#define REG_FEB_TRIG_DELAY 0x200004
#define REG_FEB_TRACE_LEN  0x200008
#define REG_FEB_SELF_TRIG  0x20000C
#define REG_FEB_STEP_SIZE  0x200010
#define REG_FEB_SPI        0x200014
#define REG_FEB_TIME       0x200018
#define REG_FEB_XXX        0x20001C

#define ADC_FIX_SET        0xd41
#define ADC_FIX_VAL        0x800

#define FEBEX_CH     16

#define FEB_TRACE_LEN   200  // in nr of samples
#define FEB_TRIG_DELAY   20  // in nr.of samples
//#define FEB_TRACE_LEN  200  // in nr of samples
//#define FEB_TRIG_DELAY 100  // in nr.of samples

#define CLK_SOURCE_ID     {0xff,0}  // sfp_port, module_id of the module to distribute clock
//#define CLK_SOURCE_ID     {0x0,0}  // sfp_port, module_id of the module to distribute clock

//--------------------------------------------------------------------------------------------------------

#define DATA_FILT_CONTROL_REG 0x2080C0
#define DATA_FILT_CONTROL_DAT 0x80         // (0x80 E,t summary always +  data trace                 always
// (0x82 E,t summery always + (data trace + filter trace) always
// (0x84 E,t summery always +  data trace                 if > 1 hit
// (0x86 E,t summery always + (data trace + filter trace) if > 1 hit
// Trigger/Hit finder filter

#define TRIG_SUM_A_REG    0x2080D0
#define TRIG_GAP_REG      0x2080E0
#define TRIG_SUM_B_REG    0x2080F0

#define TRIG_SUM_A     8  // for 12 bit: 8, 4 ,9 (8+1); for 14 bit: 14, 4, 15 (14 + 1).
#define TRIG_GAP       4
#define TRIG_SUM_B     9 // 8 + 1: one has to be added.

// Energy Filters and Modes
#define ENABLE_ENERGY_FILTER 1
#define TRAPEZ               1  // if TRAPEZ is off, MWD will be activated
#ifdef ENABLE_ENERGY_FILTER
#ifdef TRAPEZ
#define ENERGY_SUM_A_REG  0x208090
#define ENERGY_GAP_REG    0x2080A0
#define ENERGY_SUM_B_REG  0x2080B0

#define ENERGY_SUM_A   6
#define ENERGY_GAP     3
#define ENERGY_SUM_B   7  // 64 + 1: one has to be added.
#endif
#endif

//////////////////////////////
// JAM  2026 default values for config arrays that were not set in xml file:

// febex has 14bit ADC
#define FEBEX_14BIT_DEFAULT     1
// negative polarity for sfp
#define FEBEX_POL_NEG_DEFAULT   0

// trigger mode   0  3step
//                1  2-window 60  MHz
//                2  2-window 30  MHz
//                4  2-window 15  MHz
//                8  2-window 7.5 MHz
#define FEBEX_TRIG_MOD_DEFAULT  1

// enable even odd or mode
#define FEBEX_EOR_ENAB_DEFAULT 0

//disable channels mask  bit 0 special channel, bit 1-16  adc channels
//                       0x00000  all enabled
#define FEBEX_CHMASK_DISAB_DEFAULT 0x00000

//data sparsifying mask  bit 0  special channel, bit 1-16  adc channels
//                                0x00000  sparsifying disabled for all channles
//                                0x1fffe  sparsifying for all adc channels enabled,
//                                         sparsifying for special channel  disabled
#define FEBEX_CHMASK_SPARS_DEFAULT 0x00000

//internal trigger enable/disable for adc channels 0-15
//                0x0000  trigger disabled for all adc channels
//                0xffff  trigger enabled  for all adc channels
#define FEBEX_CHMASK_TRIG_DEFAULT 0xffff

//  FEBEX3/4  threshold, to be set for each channel of all slaves at 4 spfs
// max value 0x1FF 255 (adc counts)
#define FEBEX_THRESH_DEFAULT 0x03f
//////////////////////////////////////////////////////////////////////////

namespace pex
{

extern const char *xmlTracelen;
extern const char *xmlTrigDelay;
extern const char *xmlDataFilterControl;
extern const char *xmlTrigSumA;
extern const char *xmlTrigGap;
extern const char *xmlTrigSumB;
extern const char *xmlEnergySumA;
extern const char *xmlEnergyGap;
extern const char *xmlEnergySumB;
extern const char *xmlClockSourceSFP;
extern const char *xmlClockSourceSlave;

extern const char *xmlFebex14bit;
extern const char *xmlNegativePolarity;
extern const char *xmlTriggerMode;
extern const char *xmlEvenOddOr;
extern const char *xmlDisableMask;
extern const char *xmlSparsifyingMask;
extern const char *xmlTriggerMask;
extern const char *xmlChannelThreshold;

/**
 * JAM 19.02.2026 -
 *
 * Frontend board implementation for FEBEX3 - prototype
 * */

class Febex3: public pex::FrontendBoard
{
  //friend class Device;

public:
  Febex3(const std::string &name, dabc::Command cmd);
  virtual ~Febex3();

protected:

  /** length of ADC trace, number of samples */
  int fTracelen;

  /** trigger delay  of ADC trace,  number of samples */
  int fTrigDelay;

  /** data filter control register:
   *  (0x80 E,t summary always +  data trace                 always
   *  (0x82 E,t summery always + (data trace + filter trace) always
   *  (0x84 E,t summery always +  data trace                 if > 1 hit
   *  (0x86 E,t summery always + (data trace + filter trace) if > 1 hit
   *  */
  int fDataFilterControl;

  /** Trigger/Hit finder filter: trigger sum A
   * for 12 bit: 8, 4 ,9 (8+1); for 14 bit: 14, 4, 15 (14 + 1).*/
  int fTrigSumA;

  /** Trigger/Hit finder filter: trigger gap */
  int fTrigGap;

  /** Trigger/Hit finder filter: trigger sum B
   * NOTE: one has to be added (e.g. value 9 for 8)*/
  int fTrigSumB;

  /** Energy Filters and Modes: energy sum A */
  int fEnergySumA;

  /** Energy Filters and Modes: energy gap */
  int fEnergyGap;

  /** Energy Filters and Modes: energy sum B
   * NOTE: one has to be added (e.g. value 65 for 64) */
  int fEnergySumB;

  /* sfp_port of the module to distribute clock, if any */
  int fClockSourceSFP;

  /* module_id of the module to distribute clock, if any*/
  int fClockSourceSlave;

  std::vector<int> f12_14[PEX_NUMSFP];
  std::vector<int> fPolarity[PEX_NUMSFP];
  std::vector<int> fTriggerMod[PEX_NUMSFP];
  std::vector<int> fEvenOddOr[PEX_NUMSFP];
  std::vector<int> fDisableChannel[PEX_NUMSFP];
  std::vector<int> fDataReduction[PEX_NUMSFP];
  std::vector<int> fEnableTrigger[PEX_NUMSFP];
  std::vector<int> fThreshold[PEX_NUMSFP][PEX_MAXSLAVES];    // [FEBEX_CH];

  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure(int sfp, int slave) override;

};

}

#endif

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
#ifndef PEX_Poland
#define PEX_Poland

#include "pex/FrontendBoard.h"
//#include "dabc/commands.h"


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

//////////////////////////////////////////////////////////////////////////

namespace pex
{



/**
 * JAM 16.02.2006 -
 *
 * Frontend board implementation for Poland - prototype
 * */

class Poland: public pex::FrontendBoard
{
  //friend class Device;

public:
  Poland(const std::string &name, dabc::Command cmd);
  virtual ~Poland ();


protected:

  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable (int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable (int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure (int sfp, int slave) override;



};


}

#endif

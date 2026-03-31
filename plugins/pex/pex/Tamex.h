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
#ifndef PEX_Tamex
#define PEX_Tamex

#include "pex/FrontendBoard.h"
//#include "pex/Device.h"

#include <vector>
#include <string>

//////////////
//// definitions taken from murx framework so far:


#define REG_TAM_CTRL      0x200000
#define REG_TAM_TRG_WIN   0x200004
#define REG_TAM_EN_CH     0x200008
#define REG_TAM_POL_TDC   0x200010
#define REG_TAM_EN_TR     0x33001C         // 0x20000c
#define REG_TAM_POL_TRG   0x330018         // 0x200010
#define REG_TAM_MISC1     0x330010     // 0x200014
#define REG_TAM_MISC2     0x330014     // 0x200018
#define REG_TAM_MISC3     0x20001c
#define REG_TAM_CLK_SEL   0x311000
#define REG_TAM_BUS_EN    0x311008
#define REG_TAM_SPI_DAT   0x311018
#define REG_TAM_SPI_CTL   0x311014







//////////////////////////////
// JAM  2026 default values for config arrays that were not set in xml file:


// tamex readout
// ZERO suppression mode (#define DATA_REDUCTION  0x1 & #define ZERO_LENGTH_DATA 1 )  doesn't works with the parallel mode by the mbspex library (USE_MBSPEX_LIB & not SEQUENTIAL_TOKE_SEND)

#define LONG_TRIGGER_WINDOW  1


//#define PATTERN_UNIT 1



// TAMEX module type on SFP    0  1  2  3
//                             |  |  |  |
#define TAMEX_TM             { 3, 3,10, 10}  //  1: TAMEX2 with passive signal input interface
                                            //  2: TAMEX2 with PADI    signal input interface
                                            //  3: TAMEX-PADI1, TAMEX4
                                            // 10: TAMEX3
                                            // 41: TAMEX4 with PQDC1 Frontend


// #define DONT_RE_INITIALIZE_MODULES_AFTER_ERROR 1 // Henning

#define DATA_REDUCTION  0x1 // switch on meta data suppression for all readout tamex boards in case of no
//#define DATA_REDUCTION  0x0 // switch on meta data suppression for all readout tamex boards in case of no
                            // hit found in tamex:
                            // 0x1: meta data reduction enabled
                            // 0x0:                     disabled

#define DISABLE_CHANNEL    0x0  // expert setting, leave as it is
#define FIFO_ALMOSTFULL_TH 0x14 // expert setting, leave as it is
//#define FIFO_ALMOSTFULL_TH 0x30 // expert setting, leave as it is

//----------------------------------------------------------------------------
// Test pulse to TDC On =1 /Off = 0
// Plese keep it off during expeirments.
// The width of the first test pulse is 16 nsec and the 2nd is 8 nsec. (However if the firmware utilizes an internal stretcher of 10 nsec, the width is also stretched by 10 nsec for the positive polarity. If the negative polarity is used, the TE-LE shrinks by 10 nsec.)
// If the frequency is set to 120 kHz, it will not generate the delayed 2nd pulse.
//----------------------------------------------------------------------------
#define TEST_PULSE_ON 0x0  //  The TDC module is connected to TDC inputs.
//#define TEST_PULSE_ON 0x1  //  The TDC moudle is connected to test pulse produced in FPGA.
//
#define TEST_PULSE_CH 0x1ffffffff  // TDC channels to have test pulse.
//#define TEST_PULSE_CH 0xffff  // TDC channels to have test pulse.
//#define TEST_PULSE_CH 0x0  // TDC channels to have test pulse.
//#define TEST_PULSE_DELAY 0xf  // The test pulse produced in FPGA has 2nd pulse with the delay of 8nsec*this value.
                 // Maximum delay can be 120 nsec with the value=0xf.
#define TEST_PULSE_DELAY 0x0  // The 2nd pulse of the test pulse is disabled.
#define TEST_PULSE_FREQ  0x0  //  0:30kHz 1:120kHz

//----------------------------------------------------------------------------
// Clock source specification for TDC clock
//----------------------------------------------------------------------------


// TAMEX-PADI1 / TAMEX4
#define CLK_SRC_TDC_TAM4_PADI 0x1  // TAMEX-PADI1: 0x0 -> CLK from previous module via backplane
                                   //                     (feed in 200 MHz on crate interface)
                                   //              0x1 -> CLK from on-board oscillator
                                   //              0x2 -> CLK from module front panel (feed in 200 MHz)
                                   //              0x4 -> CLK from previous module via backplane
                                   //                     (first module CLK-master with local CLK)
                                   //              0x8 -> CLK from previous module via backplane
                                   //                     (first module CLK-master with external CLK from front)

// TAMEX2:
#define CLK_SRC_TDC_TAM2      0x24 // TAMEX2: 0x20 -> External CLK via 2 pin lemo (200 MHz)
                                   //         0x21 -> CLK from TRBus (25 MHz) via on-board PLL            ! to be tested
                                   //         0x22 -> CLK from TRBus + Module 0 feeds 25 MHz CLK to TRBus ! to be tested
                                   //         0x24 -> On-board oscillator (200 MHz)

// TAMEX3:
#define CLK_SRC_TDC_TAM3      0x2a // TAMEX3: 0x26 -> External CLK from backplane (200 MHz)
//#define CLK_SRC_TDC_TAM3      0x20 // TAMEX3: 0x26 -> External CLK from backplane (200 MHz)
//#define CLK_SRC_TDC_TAM3      0x26 // TAMEX3: 0x26 -> External CLK from backplane (200 MHz)
                                   //         0x2a -> On-board oscillator (200 MHz)
                                   //         0x22 -> CLK from TRBus (25 MHz) via on-board PLL
                                   //                 (Module 0 feeds 25 MHz CLK to TRBus)
                                   //         0x20 -> External CLK from backplane (25 MHz)

//----------------------------------------------------------------------------
// PQDC/PADIWA Thresholds
//----------------------------------------------------------------------------

#define SET_PQDC_TH_AT_INIT 1

#define PQDC_DEF_TH_MV -800  // PQDC fast branch threshold in mV relative to baseline (signal AC coupled and amplified 20-30 times)
                            // neg signal >> negative value
                            // pos signal >> positive value

//----------------------------------------------------------------------------
// PADI discriminator settings (TAMEX-PADI1 & TAMEX2 with PADI add-on)
//----------------------------------------------------------------------------

#define SET_PADI_TH_AT_INIT 0 // Henning

#define PADI_DEF_TH 0xa000a000 // PADI thresholds set at startup 2x16 bits for PADI1/2

#define COMBINE_TRIG 1         // Must be 1 or 0:
                               // If 1, OR signals from both PADIs are combined to one OR_out signal

#define ENABLE_OR_TAM2 0       // Must be 1 or 0:
                               // If 1, NIM trigger signals are generated from PADI OR on TAMEX2

#define EN_TRIG_ASYNC 0 // Henning

//----------------------------------------------------------------------------
// TDC & Trigger settings
//----------------------------------------------------------------------------

#define EN_TDC_CH  0xFFFFFFFF // channel enable 32-1 (0xffffffff -> all channels enabled)

#define EN_TRIG_CH 0xFFFFFFFF // Trigger enable 32-1 (only on TAMEX-PADI1 so far)
                              // 0xffffffff -> all channels enabled for trigger output

#define TDC_CH_POL 0x00000000 // Input signal polarity (0: for negative, 1: for positive)

#define EN_REF_CH             // Measurement of AccTrig on CH0 - comment line to disable

//----------------------------------------------------------------------------
// Trigger Window
//----------------------------------------------------------------------------

#ifndef LONG_TRIGGER_WINDOW
 #define TRIG_WIN_EN 1           // 0 trigger window control is off,
#endif                           // everything will be written out
                                 // with LONG_TRIGGER_WINDOW enabled trigger window control will be
                                 // handled implicitly:
                                 // OFF when PRE_TRIG_TIME and POST_TRIG_TIME are set to 0
                                 // ON  when PRE_TRIG_TIME and POST_TRIG_TIME are set to not 0

#define PRE_TRIG_TIME     200    // in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns
//#define POST_TRIG_TIME  60000    // in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns
//#define POST_TRIG_TIME   1400    // in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns
#define POST_TRIG_TIME   200    // in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns









namespace pex
{

extern const char *xmlTamexInitModulesFlag;

extern const char *xmlTamexSetPadiThres;
extern const char *xmlTamexPadiThres;
extern const char *xmlTamexSetPQDCThres;
extern const char *xmlTamexPQDCThres;

extern const char *xmlTamexType;

extern const char *xmlTamexPreTrigTime;
extern const char *xmlTamexPostTrigTime;


extern const char *xmlTamexChmaskEnab;
extern const char *xmlTamexChmaskTrig;
extern const char *xmlTamexChmaskPol;
extern const char *xmlTamexDataRedEnabl;

extern const char *xmlTamexClkTamex2;
extern const char *xmlTamexClkTamex3;
extern const char *xmlTamexClkTamex4;

extern const char *xmlTamexTestpulseOn;
extern const char *xmlTamexTestpulseDelay;
extern const char *xmlTamexTestpulseFreq;
extern const char *xmlTamexTestpulseChmask;

extern const char *xmlTamexEnableTriggerWindow;
extern const char *xmlTamexLongTriggerWindow;
extern const char *xmlTamexEnableRefChannel;
extern const char *xmlTamexCombineTrigger;
extern const char *xmlTamexEnableOr;


typedef enum tamex_type
{
  TAMEX2_PASSIVE=1,         //  1: TAMEX2 with passive signal input interface
  TAMEX2_PADI=2,            //  2: TAMEX2 with PADI    signal input interface
  TAMEX4_PADI=3,            //  3: TAMEX-PADI1, TAMEX4
  TAMEX3=10,                // 10: TAMEX3
  TAMEX4_PQDC1 = 41         // 41: TAMEX4 with PQDC1 Frontend
} tamex_type_t;








/**
 * JAM 27.03.2026 -
 *
 * Frontend board implementation for FEBEX3 - prototype
 * */

class Tamex: public pex::FrontendBoard
{
  //friend class Device;

public:
  Tamex(const std::string &name, dabc::Command cmd);
  virtual ~Tamex();

protected:

  /* type of tamex modules for each sfp chain*/
  tamex_type_t fTamexType[PEX_NUMSFP];




/* trigger window times
   in nr of time slices a 5.00 ns: max 0x7ff := 2047 * 5.00 ns
trigger window is OFF when PRE_TRIG_TIME and POST_TRIG_TIME both set to 0 */
  int fPreTrigTime;
  int fPostTrigTime;

  /** Effective trigger window evaluated from  fPreTrigTime and fPostTrigTime*/
  long fTriggerWindow;

/* data reduction
  switch on meta data suppression for all readout tamex boards
  in case of no hit found in tamex
  0x1 meta data reduction enabled  0x0 disabled*/
  int fDataReduction;

/* channel polarity mask (still common for all slaves)
  Input signal polarity (0 for negative, 1 for positive)*/
  int fTdcChPolarity;

/* enable channel mask (still common for all slaves)
   trigger enable 32-1 (only on TAMEX-PADI1 so far)
   0xffffffff -> all channels enabled for trigger output*/
  int fEnabledTdcCh;

 /*trigger channel mask (still common for all slaves)*/
  int fEnabledTrigCh;

/* clock source modes
   TAMEX2  0x20 -> External CLK via 2 pin lemo (200 MHz)
           0x21 -> CLK from TRBus (25 MHz) via on-board PLL            ! to be tested
           0x22 -> CLK from TRBus + Module 0 feeds 25 MHz CLK to TRBus ! to be tested
           0x24 -> On-board oscillator (200 MHz)*/
  int fClkSrcTdcTam2;


/* clock source modes
   TAMEX3  0x26 -> External CLK from backplane (200 MHz)
           0x2a -> On-board oscillator (200 MHz)
           0x22 -> CLK from TRBus (25 MHz) via on-board PLL
                   (Module 0 feeds 25 MHz CLK to TRBus)
           0x20 -> External CLK from backplane (25 MHz)*/
  int fClkSrcTdcTam3; //


/*  clock source modes
    TAMEX-PADI1  0x0 -> CLK from previous module via backplane
                     (feed in 200 MHz on crate interface)
               0x1 -> CLK from on-board oscillator
               0x2 -> CLK from module front panel (feed in 200 MHz)
               0x4 -> CLK from previous module via backplane
                      (first module CLK-master with local CLK)
               0x8 -> CLK from previous module via backplane
                      (first module CLK-master with external CLK from front)*/
  int fClkSrcTdcTam4Padi;

 /* enable internal test pulse (1 on, 0 off)*/
  int fTestPulseOn;

 /*testpulse channel mask
  each bit 1 enables testpulse of such channel*/
  long fTestPulseCh;

 /* delay
  The test pulse produced in FPGA has 2nd pulse with the delay of 8nsec*this valu
  Maximum delay can be 120 nsec with the value=0xf. value 0 disables 2nd pulse */
  int fTestPulseDelay;

/* frequency
   0 - >30kHz 1-> 120kHz */
  int fTestPulseFreq;

  /* set default Padi threshold at init*/
    bool fSetPadiThresholdsAtInit;

   /* default Padi threshold*/
   int fPadiThreshold;


  /* set default PQDC threshold at init*/
   bool fSetPqdcThresholdsAtInit;

  /* default PQDC threshold*/
  float fPqdcThreshold;

  /* if true do re-initialiaztion at startup*/
  bool fInitializeTamexModules;

  /* Enable measurement of acquisition trigger on CH0 */
   bool fEnableReferenceChannel;

   /* enable triggerwindow mode - only hits inside window are transferred, if false everything will be written out */
    bool fEnableTriggerWindow;

   /* long triggerwindow mode  */
   bool fLongTriggerWindow;

   /* if true,  OR signals from both PADIs are combined to one OR_out signal*/
   bool fCombinePadiTrigger;

   /* If true, NIM trigger signals are generated from PADI OR on TAMEX2 */
   bool fEnableOrTamex2;



  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure(int sfp, int slave) override;

};

}

#endif

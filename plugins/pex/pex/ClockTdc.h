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
#ifndef PEX_ClockTdc
#define PEX_ClockTdc

#include "pex/FrontendBoard.h"
//#include "pex/Device.h"

#include <vector>
#include <string>


#define CTDC_CHA_TH      128 // for PADI setting
#define CTDC_CHA         129 // for analysis
#define CTDC_N_PADI_FE     8 // nr. of padi frontend boards per clock tdc, each frontend 2 padi chips a` 8 channels
#define CTDC_N_PADI_CHA    8 // nr. of padi channels per padi chip


// important/letal: set TRIG_CVT in setup.usf to CTDC_TRIG_WIN_LEN - irq latency (5 us) in real time

#define CTDC_FREQ           150. // phase shift clock tdc: 250 mhz (set 250.), 4.00 ns
                                 // serdes clock tdc:      150 mhz (set 150.), 6.67 ns

//#define CTDC_TRIG_WIN_LEN   250  // maximum trigger window length 4095 slices of 4.0 or 6.67 ns (see TDC_FREQ)
//#define CTDC_PRE_TRIG_LEN   100  //     "
//#define CTDC_TRIG_WIN_LEN   4095  // maximum trigger window length 4095 slices of 4.0 or 6.67 ns (see TDC_FREQ)
//#define CTDC_PRE_TRIG_LEN   4095  //     "
#define CTDC_TRIG_WIN_LEN   240  // 2 micro sec
#define CTDC_PRE_TRIG_LEN   120  //     "

#define CTDC_CLOCK_SOURCE   0x0  // 0x0: local/internal clock oscillator (200 mhz)
                                 // 0x1: external (200 mhz)

#define CTDC_DATA_FORMAT      1  // 1: fast data taking, most reduced, data from inside trigger window
//#define CTDC_DATA_FORMAT      7  // 1: fast data taking, most reduced, data from inside trigger window
                                 // 3: with channel headers and footers, data from inside trigger windoe
                                 // 7: with channel headers and footers, all data



//#define ZERO_LENGTH_DATA 1        // Some firmware can have absolute zero length data and can skip dma data transfer time to time.
                                  // We need extra reset for the events without dma transfer.
//----------------------------------------------------------------------------
// Plese keep it off during expeirments.
// Test pulse to TDC On =1 /Off = 0
// frequency: ~ 244 kHz (65/0x100*1000)
// width: ~ 15.4 nsec. (1000/65)
//#define TEST_PULSE_ON 0  //  The TDC module is connected to TDC inputs.
//#define TEST_PULSE_ON 1  //  The TDC moudle is connected to test pulse produced in FPGA.
#define CTDC_TEST_PULSE_ON -1  // Keep the setting as it is.
// ---------------------------------------------------------------------------
// Readout of the scaler data enable/disable.
// Reference counter for the scaler has ~ 2 kHz (65/0x8000*1000)
//#define SCALER_DATA 0x1  // Readout of the scaler data by trigger 3.
#ifdef SCALER_DATA
#define NUM_SCALER_CH      131 // 128+1 TDC inputs, triger 1, 2kHz
#endif //
//----------------------------------------------------------------------------


#define CTDC_THRESH_DEFAULT 0x1ff
//*********************************************************************************************************



// ctdc specific registers
#define REG_CTDC_CTRL       0x200000
#define REG_CTDC_DEL        0x200004
#define REG_CTDC_GATE       0x200008

#define REG_CTDC_DIS_0      0x200010
#define REG_CTDC_DIS_1      0x200014
#define REG_CTDC_DIS_2      0x200018
#define REG_CTDC_DIS_3      0x20001c
#define REG_CTDC_DIS_4      0x2000a0

#define REG_CTDC_TRIG_DIS_0 0x200030
#define REG_CTDC_TRIG_DIS_1 0x200034
#define REG_CTDC_TRIG_DIS_2 0x200038
#define REG_CTDC_TRIG_DIS_3 0x20003c
#define REG_CTDC_TRIG_DIS_4 0x2000a4

#define REG_CTDC_TIME_RST   0x200020

#define REG_CTDC_PADI_SPI_CTRL  0x200040
#define REG_CTDC_PADI_SPI_DATA  0x200044

#define REG_CTDC_POL_TDC    0x200100
#define REG_CTDC_CAL_PULSE  0x200104

#define REG_SCALER  0x201000
#define REG_CH_129  0x201000





namespace pex
{
 extern const char *xmlClockTdcReadScaler;
 extern const char *xmlClockTdcScalerChannels;
 extern const char *xmlClockTdcClockSource;
 extern const char *xmlClockTdcClockFreq;
 extern const char *xmlClockTdcDataFormat;
 extern const char *xmlClockTdcTriggerWindowLen;
 extern const char *xmlClockTdcPreTriggerLen;
 extern const char *xmlClockTdcChanMaskDisabled;
 extern const char *xmlClockTdcChanMaskTrigger;
 extern const char *xmlClockTdcPadiThresholds;
 extern const char *xmlClockTdcTestPulse;

/**
 * JAM 31.03.2026 -
 *
 * Frontend board implementation for CLOCKTDC - prototype
 * */

class ClockTdc: public pex::FrontendBoard
{

public:
  ClockTdc(const std::string &name, dabc::Command cmd);
  virtual ~ClockTdc();

protected:



    /* Readout of the scaler data by trigger 3.
      Reference counter for the scaler has ~ 2 kHz (65/0x8000*1000)*/
    bool fReadScalerData;

    /* Number of scaler channels if fReadScalerData is enabled
     *  e.g 131 // 128+1 TDC inputs, triger 1, 2kHz */
    int fNumScalerChannels;

    /* phase shift clock tdc: 250 mhz (set 250.), 4.00 ns
                                      serdes clock tdc:      150 mhz (set 150.), 6.67 ns */
    float fCtdcFrequency;

    /** 0x0: local/internal clock oscillator (200 mhz)
        0x1: external (200 mhz) */
    int fCtdcClockSource;


    int fCtdcDataFormat;

    /** maximum trigger window length 4095 slices of 4.0 or 6.67 ns (see TDC_FREQ) */
    int fCtdcTrigWinLen;

    /** pre trigger window length 4095 slices of 4.0 or 6.67 ns (see TDC_FREQ) */
    int fCtdcPreTrigLen;

    /** enable internal test pulse */
    bool fCtdTestPulseOn;

    /** channel enabling mask in 5 channel blocks (vector elements) for each sfp and all 16 slaves
      channel         0-31        32-63       64-95       96-128   128 */
    std::vector<int> fChan_Enabled[PEX_NUMSFP][PEX_MAXSLAVES];

    /** trigger enabling mask in 5 channel blocks (vector elements) for each sfp and allslaves
          channel         0-31        32-63       64-95       96-128   128 */
    std::vector<int> fChan_Trigger_Enabled[PEX_NUMSFP][PEX_MAXSLAVES];

    /** PADI thresholds for each channel (128 vector elements) for each sfp and all slaves */
    std::vector<int> fThreshold[PEX_NUMSFP][PEX_MAXSLAVES];


    /** keep channel enable register addresses for the 5 channel blocks*/
    int fCtdcChanEnaRegister[5];

    /** keep trigger enable register addresses for the 5 channel blocks*/
    int fCtdcChanTrigEnaRegister[5];

  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable(int sfp, int slave) override;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure(int sfp, int slave) override;

};

}

#endif

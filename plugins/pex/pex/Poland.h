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
//// definitions taken from PolandSetup.h of poland gosipgui

// qfw specific registers:
//#define REG_CTRL            0x200000
//#define REG_QFW_OFFSET_BASE 0x200100

#define POLAND_REG_TRIGCOUNT 0x0

#define POLAND_REG_RESET        0x200000
#define POLAND_REG_STEPS_BASE   0x200014
#define POLAND_REG_STEPS_TS1    0x200014
#define POLAND_REG_STEPS_TS2    0x200018
#define POLAND_REG_STEPS_TS3    0x20001C


#define POLAND_REG_QFW_RESET 0x200010

#define POLAND_REG_TIME_BASE    0x200020
#define POLAND_REG_TIME_TS1     0x200020
#define POLAND_REG_TIME_TS2     0x200024
#define POLAND_REG_TIME_TS3     0x200028
#define POLAND_TS_NUM           3

#define POLAND_REG_QFW_MODE         0x200004
#define POLAND_REG_QFW_PRG          0x200008

#define POLAND_REG_DAC_MODE         0x20002c
#define POLAND_REG_DAC_PROGRAM      0x200030
#define POLAND_REG_DAC_BASE_WRITE         0x200050
#define POLAND_REG_DAC_BASE_READ         0x200180

#define POLAND_REG_DAC_ALLVAL       0x2000d4
#define POLAND_REG_DAC_CAL_STARTVAL  0x2000d0
#define POLAND_REG_DAC_CAL_OFFSET    0x200034
#define POLAND_REG_DAC_CAL_DELTA     0x20000c
#define POLAND_REG_DAC_CAL_TIME      0x200038


#define POLAND_REG_INTERNAL_TRIGGER     0x200040
#define POLAND_REG_DO_OFFSET            0x200044
#define POLAND_REG_OFFSET_BASE          0x200100
#define POLAND_REG_MASTERMODE           0x200048
#define POLAND_REG_TRIG_ON              0x20004C


#define POLAND_REG_FAN_BASE             0x200208
#define POLAND_REG_TEMP_BASE             0x200210

#define POLAND_REG_FAN_SET             0x2000dc

#define POLAND_REG_CSA_CTRL            0x2000e0


#define POLAND_REG_ID_BASE 0x200220

//#define POLAND_REG_ID_MSB 0x200228
//#define POLAND_REG_ID_LSB 0x20022c

#define POLAND_REG_FIRMWARE_VERSION 0x200280

#define POLAND_FAN_NUM              4
#define POLAND_TEMP_NUM             7

// conversion factor temperature sensors to degrees centigrade:
#define POLAND_TEMP_TO_C 0.0625

// conversion factor fan speed to rpm:
#define POLAND_FAN_TO_RPM 30.0



#define POLAND_ERRCOUNT_NUM             8
#define POLAND_DAC_NUM                  32

/** microsecond per time register unit*/
#define POLAND_TIME_UNIT                0.02

#define POLAND_QFWLOOPS 3




//////////////////////////////////////////////////////////////////////////

namespace pex
{

 extern const char *xmlQfwMode;
 extern const char *xmlDACMode;
 extern const char *xmlDACAllValue;
 extern const char *xmlPolandTriggerMasterSFP;
 extern const char *xmlPolandTriggerMasterSlave;
 extern const char *xmlQFWSteps;
 extern const char *xmlQFWTimes;


//         <string>(-) [ 2.5pF &amp; 0.25pC]</string> ->0
//         <string>(-) [25.0pF &amp; 2.50pC]</string> ->1
//         <string>(+) [ 2.5pF &amp; 0.25pC]</string> ->2
//         <string>(+) [25.0pF &amp; 2.50pC]</string> ->3
//         <string>1000uA (-) [ 2.5pF &amp; 0.25pC]</string> ->16
//         <string>1000uA (-) [25.0pF &amp; 2.50pC]</string> ->17
//         <string>1000uA (+) [ 2.5pF &amp; 0.25pC]</string> ->18
//         <string>1000uA (+) [25.0pF &amp; 2.50pC]</string> ->19

typedef enum qfwmode
    {
      QFWMODE_MINUS_2_5pF_0_25pC,
      QFWMODE_MINUS_25_0pF_2_50pC,
      QFWMODE_PLUS_2_5pF_0_25pC,
      QFWMODE_PLUS_25_0pF_2_50pC,
      QFWMODE_4_DUMMY,
      QFWMODE_5_DUMMY,
      QFWMODE_6_DUMMY,
      QFWMODE_7_DUMMY,
      QFWMODE_8_DUMMY,
      QFWMODE_9_DUMMY,
      QFWMODE_10_DUMMY,
      QFWMODE_11_DUMMY,
      FWMODE_12_DUMMY,
      QFWMODE_13_DUMMY,
      QFWMODE_14_DUMMY,
      QFWMODE_15_DUMMY,
      QFWMODE_MINUS_1000muA_2_5pF_0_25pC,
      QFWMODE_MINUS_1000muA_25_0pF_2_50pC,
      QFWMODE_PLUS_1000muA_2_5pF_0_25pC,
      QFWMODE_PLUS_1000muA_25_0pF_2_50pC
    } poland_qfwmode_t;

    typedef enum dacmode
        {
        DACMODE_NONE,
        DACMODE_MANUAL,
        DACMODE_TESTSTRUCTURE,
        DACMODE_CALIBRATION,
        DACMODE_CONSTANT
        } poland_dacmode_t;

//     case 1:
////             // manual settings:
////             for (int d = 0; d < POLAND_DAC_NUM; ++d)
////             {
////               WriteGosip (fSFP, fSlave, POLAND_REG_DAC_BASE_WRITE + 4 * d, theSetup->fDACValue[d]);
////             }
////             break;
////           case 2:
////             // test structure:
////             // no more actions needed
////             break;
////           case 3:
////             // issue calibration:
////             WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_STARTVAL , theSetup->fDACStartValue);
////             WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_OFFSET ,  theSetup->fDACOffset);
////             WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_DELTA ,  theSetup->fDACDelta);
////             WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_TIME ,  theSetup->fDACCalibTime);
////
////             break;
////           case 4:
////             // all same constant value mode:
//


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


protected:

  /** number of steps in each QFW loop */
  int fQfwSteps[POLAND_QFWLOOPS];

  /** time in microseconds for each step int the loop*/
  double fQfwTimes[POLAND_QFWLOOPS];

  /** QFW sampling mode */
  poland_qfwmode_t fQfwMode;

  /** DAC setup mode */
  poland_dacmode_t fDACMode;


  /** SFP with poland trigger master*/
  int fTriggerMasterSFP;

  /** Slave in chain with poland trigger master device*/
   int fTriggerMasterSlave;

   /** common value for all DACs in specific mode*/
   int fDACallvalue;




};


}

#endif

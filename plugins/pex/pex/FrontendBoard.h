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
#ifndef PEX_FrontendBoard
#define PEX_FrontendBoard

#include "dabc/ModuleAsync.h"
#include "dabc/Command.h"

#include <string>

/** number of connected sfps*/
#define PEX_NUMSFP 4

/** maximum number of connected slaves per sfp*/
#define PEX_MAXSLAVES 16

// JAM global register definitions moved here from pexor_gosip.h
#define REG_BUF0_DATA_LEN     0xFFFD00  // buffer 0 submemory data length
#define REG_BUF1_DATA_LEN     0xFFFE00  // buffer 1 submemory data length
#define REG_DATA_REDUCTION  0xFFFFB0  // Nth bit = 1 enable data reduction of  Nth channel from block transfer readout. (bit0:time, bit1-8:adc)
#define REG_MEM_DISABLE     0xFFFFB4  // Nth bit =1  disable Nth channel from block transfer readout.(bit0:time, bit1-8:adc)
#define REG_MEM_FLAG_0      0xFFFFB8  // read only:
#define REG_MEM_FLAG_1      0xFFFFBC  // read only:
#define REG_DATA_REDUCTION_32  0xFFFC00  // Nth bit = 1 enable data reduction of  Nth channel from block transfer readout. (bit0:time, bit1-8:adc)
#define REG_MEM_DISABLE_32     0xFFFC04  // Nth bit =1  disable Nth channel from block transfer readout.(bit0:time, bit1-8:adc)
#define REG_MEM_FLAG_0_32      0xFFFC08  // read only:
#define REG_MEM_FLAG_1_32      0xFFFC0C  // read only:
#define REG_BUF0     0xFFFFD0 // base address for buffer 0 : 0x0000
#define REG_BUF1     0xFFFFD4  // base address for buffer 1 : 0x20000
#define REG_SUBMEM_NUM   0xFFFFD8 //num of channels 8
#define REG_SUBMEM_OFF   0xFFFFDC // offset of channels 0x4000
#define REG_MODID     0xFFFFE0
#define REG_HEADER    0xFFFFE4
#define REG_FOOTER    0xFFFFE8
#define REG_DATA_LEN  0xFFFFEC
#define REG_RST 0xFFFFF4
#define REG_LED 0xFFFFF8
#define REG_VERSION 0xFFFFFC
////////// end global regs

namespace pexor
{
class PexorTwo;
}
namespace pex
{

class Device;

#define MAX_DEVICE_KINDS 6

typedef enum feb_kind
{
  FEB_NONE, FEB_FEBEX3, FEB_FEBEX4, FEB_TAMEX, FEB_CTDC, FEB_FOOT, FEB_POLAND
} feb_kind_t;

/**
 * JAM 16.02.2006 -
 *
 * This is the base class for all front-end boards ("gosip slaves") specific actions
 * For each type of gosip febs, the pex::Device has one implementation of this class
 * Although a dabc worker, this should be a passive component
 * the actions are done in the device thread
 * */

class FrontendBoard: public dabc::ModuleAsync
{
  friend class pex::Device;

public:
  FrontendBoard(const std::string &name, pex::feb_kind_t kind, dabc::Command cmd);
  virtual ~FrontendBoard();

  void SetDevice(pex::Device *dev);

  bool IsType(pex::feb_kind_t kind)
  {
    return (kind == fType);
  }

  //////////////////////////
  static std::string BoardName(pex::feb_kind_t kind)
  {
    std::string name;
    switch (kind)
    {

      case FEB_FEBEX3:
        name = "Febex3";
        break;

      case FEB_FEBEX4:
        name = "Febex4";
        break;
      case FEB_TAMEX:
        name = "Tamex";
        break;
      case FEB_CTDC:
        name = "ClockTDC";
        break;
      case FEB_FOOT:
        name = "Foot";
        break;
      case FEB_POLAND:
        name = "Poland";
        break;
      case FEB_NONE:
      default:
        name = "NONE";

    }
    return name;
  }
///////////////////////////

protected:

  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable(int sfp, int slave)=0;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable(int sfp, int slave)=0;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure(int sfp, int slave)=0;

  feb_kind_t fType;    //<-shortcut to find out my type

  Device *fPexorDevice;    //< reference to device object that does all actions
  pexor::PexorTwo *fKinpex;    // <- shortcut to board object in library

};

}

#endif

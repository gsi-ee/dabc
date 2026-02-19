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


namespace pexor{
class PexorTwo;
}
namespace pex
{

class Device;

#define MAX_DEVICE_KINDS 6

typedef enum feb_kind{
  FEB_NONE,
  FEB_FEBEX3,
  FEB_FEBEX4,
  FEB_TAMEX,
  FEB_CTDC,
  FEB_FOOT,
  FEB_POLAND
} feb_kind_t;

/**
 * JAM 16.02.2006 -
 *
 * This is the base class for all front-end boards ("gosip slaves") specific actions
 * For each type of gosip febs, the pex::Device has one implementation of this class
 * Although a dabc worker, this should be a passive component
 * the actions are done in the device thread
 * */

class FrontendBoard:  public dabc::ModuleAsync
{
  friend class pex::Device;

public:
  FrontendBoard(const std::string &name, pex::feb_kind_t kind, dabc::Command cmd);
  virtual ~FrontendBoard ();

  void SetDevice(pex::Device* dev);

  bool IsType(pex::feb_kind_t kind)
  {
    return (kind==fType);
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
  virtual int Disable (int sfp, int slave)=0;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable (int sfp, int slave)=0;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure (int sfp, int slave)=0;

  feb_kind_t fType; //<-shortcut to find out my type

  Device* fPexorDevice;    //< reference to device object that does all actions
  pexor::PexorTwo* fBoard;  // <- shortcut to board object in library

};


}

#endif

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

#include "dabc/Worker.h"

#include "dabc/Command.h"



namespace pexor{
class PexorTwo;
}
namespace pex
{

class Device;


/**
 * JAM 16.02.2006 -
 *
 * This is the base class for all front-end boards ("gosip slaves") specific actions
 * For each type of gosip febs, the pex::Device has one implementation of this class
 * Although a dabc worker, this should be a passive component
 * the actions are done in the device thread
 * */

class FrontendBoard: public dabc::Worker
{
  //friend class Device;

public:
  FrontendBoard(const std::string &name, dabc::Command cmd);
  virtual ~FrontendBoard ();

  void SetDevice(pex::Device* dev);

protected:

  /** Disable feb at position sfp, slave, before configuration*/
  virtual int Disable (int sfp, int slave)=0;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Enable (int sfp, int slave)=0;

  /** Enable feb at position sfp, slave after configuration*/
  virtual int Configure (int sfp, int slave)=0;


  Device* fPexorDevice;    //< reference to device object that does all actions
  pexor::PexorTwo* fBoard;  // <- shortcut to board object in library

};


}

#endif

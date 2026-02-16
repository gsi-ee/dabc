
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
#ifndef PEX_Factory
#define PEX_Factory
#include "dabc/Factory.h"
#include "../pex/Commands.h"

namespace pex
{

class Device;


class Factory: public dabc::Factory
{
public:

  Factory (const std::string name) :
      dabc::Factory (name), fDevice(nullptr)
  {
  }
  virtual void  Initialize() override;
  virtual dabc::Module* CreateModule (const std::string& classname, const std::string& modulename, dabc::Command cmd) override;

  virtual dabc::Device* CreateDevice (const std::string& classname, const std::string& devname, dabc::Command com) override;

  virtual dabc::Reference CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd) override;

protected:

  pex::Device* fDevice; //< remember device handle for frontends

};

}    // namespace

#endif


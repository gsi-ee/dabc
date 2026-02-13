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
#include "pex/Factory.h"
#include "pex/ReadoutModule.h"
#include "pex/Device.h"
#include "pex/Player.h"

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"

dabc::FactoryPlugin pexfactory (new pex::Factory ("pex"));


dabc::Module* pex::Factory::CreateModule (const std::string& classname, const std::string& modulename,
    dabc::Command cmd)
{
  DOUT0 ("pex::Factory::CreateModule called for class:%s, module:%s", classname.c_str (), modulename.c_str ());

//  if (strcmp (classname.c_str (), "pex::ReadoutModule") == 0)
    if (classname=="pex::ReadoutModule")
  {
    dabc::Module* mod = new pex::ReadoutModule (modulename, cmd);
    unsigned int boardnum = 0;    //cmd->GetInt(ABB_PAR_BOARDNUM, 0);
    DOUT1 ("pex::Factory::CreateModule - Created PEXOR Readout module %s for /dev/pexor-%d",
        modulename.c_str (), boardnum);
    return mod;
  }

  if (classname == "pex::Player")
        return new pex::Player(modulename, cmd);


  return dabc::Factory::CreateModule (classname, modulename, cmd);
}

dabc::Device* pex::Factory::CreateDevice (const std::string& classname, const std::string& devname,
    dabc::Command cmd)
{
  DOUT0 ("pex::Factory::CreateDevice called for class:%s, device:%s", classname.c_str (), devname.c_str ());

  if (strcmp (classname.c_str (), "pex::GenericDevice") != 0)
    return nullptr;

  dabc::Device* dev = new pex::GenericDevice (devname, cmd);

  return dev;
}



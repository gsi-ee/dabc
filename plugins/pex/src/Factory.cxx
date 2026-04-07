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

#include "pex/FrontendBoard.h"



#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

dabc::FactoryPlugin pexfactory (new pex::Factory ("pex"));


dabc::Module* pex::Factory::CreateModule (const std::string& classname, const std::string& modulename,
    dabc::Command cmd)
{
  DOUT0 ("pex::Factory::CreateModule called for class:%s, module:%s", classname.c_str (), modulename.c_str ());

//  if (strcmp (classname.c_str (), "pex::ReadoutModule") == 0)
    if (classname=="pex::ReadoutModule")
  {
    dabc::Module* mod = new pex::ReadoutModule (modulename, cmd);
    unsigned int boardnum = 0;
    DOUT1 ("pex::Factory::CreateModule - Created PEXOR Readout module %s for /dev/pexor-%d",
        modulename.c_str (), boardnum);
    return mod;
  }

  if (classname == "pex::Player")
        return new pex::Player(modulename, cmd);

  ///////// JAM2026: here handle the frontend boards

   if (classname == "pex::Febex3")
   {
       pex::FrontendBoard* theboard=fDevice->CreateFrontendBoard(FEB_FEBEX3, modulename, cmd);
       DOUT1 ("pex::Factory::CreateModule - Created Frontend Board %s for type %d",
               modulename.c_str (), FEB_FEBEX3);
       return theboard;
   }

   if (classname == "pex::Poland")
     {
         pex::FrontendBoard* theboard=fDevice->CreateFrontendBoard(FEB_POLAND, modulename, cmd);
         DOUT1 ("pex::Factory::CreateModule - Created Frontend Board %s for type %d",
                 modulename.c_str (), FEB_POLAND);
         return theboard;
     }

   if (classname == "pex::Tamex")
       {
           pex::FrontendBoard* theboard=fDevice->CreateFrontendBoard(FEB_TAMEX, modulename, cmd);
           DOUT1 ("pex::Factory::CreateModule - Created Frontend Board %s for type %d",
                   modulename.c_str (), FEB_TAMEX);
           return theboard;
       }

   if (classname == "pex::ClockTdc")
       {
             pex::FrontendBoard* theboard=fDevice->CreateFrontendBoard(FEB_CTDC, modulename, cmd);
             DOUT1 ("pex::Factory::CreateModule - Created Frontend Board %s for type %d",
                     modulename.c_str (), FEB_CTDC);
             return theboard;
       }


  return dabc::Factory::CreateModule (classname, modulename, cmd);
}

dabc::Device* pex::Factory::CreateDevice (const std::string& classname, const std::string& devname,
    dabc::Command cmd)
{
  DOUT0 ("pex::Factory::CreateDevice called for class:%s, device:%s", classname.c_str (), devname.c_str ());



if (classname == "pex::GenericDevice")
      {
            fDevice = new pex::GenericDevice (devname, cmd);
            return fDevice;
      }
return nullptr;
}



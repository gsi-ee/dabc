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
#include "abb/Factory.h"
#include "abb/ReadoutModule.h"
#include "abb/WriterModule.h"
#include "abb/Device.h"

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"


abb::Factory abbfactory("abb");


dabc::Module* abb::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{
    DOUT1(("abb::Factory::CreateModule called for class:%s, module:%s", classname, modulename));

    if ((classname==0) || (cmd==0)) return 0;
    if (strcmp(classname,"abb::ReadoutModule")==0)
       {
           dabc::Module* mod= new abb::ReadoutModule(modulename, cmd);
           unsigned int boardnum=cmd->GetInt(ABB_PAR_BOARDNUM, 0);
           DOUT1(("abb::Factory::CreateModule - Created ABBReadout module %s for /dev/fpga%d", modulename, boardnum));
           return mod;
         }
    else if (strcmp(classname,"abb::WriterModule")==0)
       {
           dabc::Module* mod = new abb::WriterModule(modulename, cmd);
           unsigned int boardnum=cmd->GetInt(ABB_PAR_BOARDNUM, 0);
           DOUT1(("abb::Factory::CreateModule - Created ABBWriter module %s for /dev/fpga%d", modulename, boardnum));
           return mod;
         }
    return 0;
}


dabc::Device* abb::Factory::CreateDevice(const char* classname, const char* devname, dabc::Command* cmd)
{
   if (strcmp(classname,"abb::Device")!=0) return 0;
   // advanced device parameters contained in factory command object:
   //unsigned int boardnum= cmd->GetInt(ABB_PAR_BOARDNUM, 0);

//   bool dmamode = true; // put this to parameter later? now always use DMA

//   unsigned int bar=cmd->GetInt(ABB_PAR_BAR, 1);
//   unsigned int addr=cmd->GetInt(ABB_PAR_ADDRESS, (0x8000 >> 2));
//   unsigned int size=cmd->GetInt(ABB_PAR_LENGTH, 8192);
//   dev->SetReadBuffer(bar, addr, size);
//   dev->SetWriteBuffer(bar, addr, size); // for testing, use same values as for reading; later different parameters in setup!

   return new abb::Device(dabc::mgr()->GetDevicesFolder(true), devname, true, cmd);
}









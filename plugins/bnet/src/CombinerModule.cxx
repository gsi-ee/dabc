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
#include "bnet/CombinerModule.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/common.h"

bnet::CombinerModule::CombinerModule(const char* name, dabc::Command cmd) :
   dabc::ModuleSync(name, cmd)
{
   fNumReadouts = Cfg(xmlNumReadouts,cmd).AsInt(1);
   fModus = Cfg(xmlCombinerModus,cmd).AsInt(0);

   fInpPool = CreatePoolHandle(Cfg(CfgReadoutPool, cmd).AsStr(ReadoutPoolName));

   fOutPool = CreatePoolHandle(bnet::TransportPoolName);

   fOutPort = CreateOutput("Output", fOutPool, SenderInQueueSize);

   for (int n=0;n<NumReadouts();n++)
      CreateInput(FORMAT(("Input%d", n)), fInpPool, ReadoutQueueSize);
}

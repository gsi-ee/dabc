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
#include "bnet/LocalDFCModule.h"

#include "dabc/Port.h"
#include "dabc/Buffer.h"

#include "bnet/common.h"

bnet::LocalDFCModule::LocalDFCModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0)
{
   fPool = CreatePoolHandle(bnet::ControlPoolName);
}


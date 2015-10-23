// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/BinaryFile.h"

#include "stream/RecalibrateModule.h"

#include "hadaq/Iterator.h"
#include "mbs/Iterator.h"
#include "hadaq/TdcProcessor.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Iterator.h"
#include "dabc/Buffer.h"
#include "dabc/Publisher.h"

#include <stdlib.h>

#include "base/Buffer.h"
#include "base/StreamProc.h"

// ==================================================================================

stream::RecalibrateModule::RecalibrateModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
}

stream::RecalibrateModule::~RecalibrateModule()
{
}

void stream::RecalibrateModule::OnThreadAssigned()
{
}

bool stream::RecalibrateModule::retransmit()
{
   return false;
}

int stream::RecalibrateModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void stream::RecalibrateModule::BeforeModuleStart()
{
}

void stream::RecalibrateModule::AfterModuleStop()
{
}

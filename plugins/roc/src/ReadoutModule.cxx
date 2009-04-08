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

#include "roc/ReadoutModule.h"

#include "roc/Board.h"

#include "dabc/Port.h"

roc::ReadoutModule::ReadoutModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16384, cmd);
   int numoutputs = GetCfgInt(dabc::xmlNumOutputs, 1, cmd);

   DOUT1(("new roc::ReadoutModule %s buff %d", GetName(), fBufferSize));

   CreatePoolHandle(roc::xmlRocPool, fBufferSize, 10);

   CreateInput("Input", Pool(), 10);
   CreateRateParameter("InputRate", false, 3., "Input")->SetDebugOutput(true);

   for(int n=0; n<numoutputs; n++)
      CreateOutput(FORMAT(("Output%d", n)), Pool(), 10);
}

roc::ReadoutModule::~ReadoutModule()
{
}

void roc::ReadoutModule::ProcessInputEvent(dabc::Port* inport)
{
   dabc::Buffer* buf = inport->Recv();
   dabc::Buffer::Release(buf);
}

void roc::ReadoutModule::ProcessOutputEvent(dabc::Port* inport)
{
}

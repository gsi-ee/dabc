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
#include "dabc_root/RootTreeOutput.h"

#include "TTree.h"


dabc_root::RootTreeOutput::RootTreeOutput() :
   DataOutput(),
   fTree(0)
{
}

dabc_root::RootTreeOutput::~RootTreeOutput()
{
   delete fTree;
   fTree = 0;
}

bool dabc_root::RootTreeOutput::Write_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
   dabc::ConfigSource cfg(cmd, port);

   return false;
}


bool dabc_root::RootTreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   return true;
}

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


dabc_root::RootTreeOutput::RootTreeOutput(const dabc::Url& url) :
   DataOutput(url),
   fTree(0)
{
}

dabc_root::RootTreeOutput::~RootTreeOutput()
{
   delete fTree;
   fTree = 0;
}


unsigned dabc_root::RootTreeOutput::Write_Buffer(dabc::Buffer& buf)
{
   return dabc::do_Ok;
}

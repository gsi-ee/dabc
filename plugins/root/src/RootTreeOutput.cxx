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

bool dabc_root::RootTreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   return true;
}

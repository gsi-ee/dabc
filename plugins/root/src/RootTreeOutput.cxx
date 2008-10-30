#include "dabc/RootTreeOutput.h"

#include "TTree.h"


dabc::RootTreeOutput::RootTreeOutput() : 
   DataOutput(),
   fTree(0)
{
}

dabc::RootTreeOutput::~RootTreeOutput()
{
   delete fTree;
   fTree = 0;
}
         
bool dabc::RootTreeOutput::WriteBuffer(dabc::Buffer* buf)
{
   return true;
}

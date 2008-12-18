#include "mbs/CombinerModule.h"

mbs::CombinerModule::CombinerModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd)
{

   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 8*1024, cmd);

   fPool = CreatePoolHandle(dabc::xmlWorkPool, fBufferSize, 10);

   int numinp = GetCfgInt(dabc::xmlNumInputs, 1, cmd);

   int numout = GetCfgInt(dabc::xmlNumOutputs, 1, cmd);

   double flashtmout = GetCfgDouble(dabc::xmlFlashTimeout, 1., cmd);

   for (int n=0;n<numinp;n++)
      CreateInput(FORMAT(("Input%d", n)), fPool, 5);

   for (int n=0;n<numout;n++)
      CreateOutput(FORMAT(("Output%d", n)), fPool, 5);

   if (flashtmout>0.) CreateTimer("Flash", flashtmout, false);
}

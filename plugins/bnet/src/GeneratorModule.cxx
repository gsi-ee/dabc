#include "bnet/GeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"

// #include "bnet/WorkerApplication.h"
#include "bnet/common.h"


bnet::GeneratorModule::GeneratorModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleSync(name, cmd),
   fPool(0),
   fEventCnt(1),
   fUniqueId(0)
{
   SetSyncCommands(true);

   fPool = CreatePoolHandle(GetCfgStr(CfgReadoutPool, ReadoutPoolName, cmd).c_str());

   fBufferSize = GetCfgInt(xmlReadoutBuffer, 1024, cmd);

   CreateOutput("Output", fPool, ReadoutQueueSize);

   CreateParInt("UniqueId", 0);
}

void bnet::GeneratorModule::MainLoop()
{
   fUniqueId = GetParInt("UniqueId", 0);

//   DOUT1(("GeneratorModule::MainLoop fUniqueId = %llu", fUniqueId));

   dabc::BufferGuard buf;

   while (ModuleWorking()) {

      buf = TakeBuffer(fPool, fBufferSize);

      GeneratePacket(buf());

      Send(Output(0), buf);
   }
}

#include "bnet/GeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"

#include "bnet/WorkerPlugin.h"
#include "bnet/common.h"


bnet::GeneratorModule::GeneratorModule(dabc::Manager* mgr, const char* name, 
                                       WorkerPlugin* factory) : 
   dabc::ModuleSync(mgr, name),
   fPool(0),
   fEventCnt(1),
   fUniqueId(0)
{
   SetSyncCommands(true); 
    
   fPool = CreatePool(factory->ReadoutPoolName());
   
   fBufferSize = factory->ReadoutBufferSize();

   CreateOutput("Output", fPool, ReadoutQueueSize);
   
   new dabc::IntParameter(this, "UniqueId", 0);
}

void bnet::GeneratorModule::MainLoop()
{
   fUniqueId = GetParInt("UniqueId", 0); 
   
//   DOUT1(("GeneratorModule::MainLoop fUniqueId = %llu", fUniqueId));
    
   dabc::BufferGuard buf;
    
   while (TestWorking()) {

      buf = TakeBuffer(fPool, fBufferSize);
              
      GeneratePacket(buf());
      
      Send(Output(0), buf);
   }
}

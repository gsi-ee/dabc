#include "bnet/GeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"

#include "bnet/defines.h"

bnet::GeneratorModule::GeneratorModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fEventCnt(0),
   fUniqueId(0),
   fNumIds(1)
{
   CreatePoolHandle("BnetDataPool");

   CreateOutput("Output", Pool(), 10);

   fEventCnt = 1;

   fEventHandling = dabc::mgr.CreateObject("TestEventHandling", "bnet-evnt-gener");

   if (fEventHandling.null()) {
      EOUT(("Cannot create event handling!!!"));
      exit(9);
   }
}

void bnet::GeneratorModule::BeforeModuleStart()
{
   fUniqueId = dabc::mgr.NodeId();
   fNumIds = dabc::mgr.NumNodes();
}

void bnet::GeneratorModule::ProcessOutputEvent(dabc::Port* port)
{
   while (port->CanSend()) {
      dabc::Buffer buf = Pool()->TakeBuffer();

      if (!fEventHandling()->GenerateSubEvent(fEventCnt,fUniqueId,fNumIds,buf)) break;

      fEventCnt++;

      port->Send(buf);
   }
}

#include "bnet/GeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"

#include "bnet/defines.h"

bnet::GeneratorModule::GeneratorModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fEventCnt(0),
   fUniqueId(0),
   fNumIds(1)
{
   EnsurePorts(0, 1, "BnetDataPool");

   fEventCnt = 1;

   fEventHandling = dabc::mgr.CreateObject("TestEventHandling", "bnet-evnt-gener");

   if (fEventHandling.null()) {
      EOUT("Cannot create event handling!!!");
      exit(9);
   }
}

void bnet::GeneratorModule::BeforeModuleStart()
{
   fUniqueId = dabc::mgr.NodeId();
   fNumIds = dabc::mgr.NumNodes();
}

bool bnet::GeneratorModule::ProcessSend(unsigned port)
{
   dabc::Buffer buf = TakeBuffer();

   if (fEventHandling()->GenerateSubEvent(fEventCnt,fUniqueId,fNumIds,buf)) {
      fEventCnt++;

      Send(port, buf);
   }

   return true;
}

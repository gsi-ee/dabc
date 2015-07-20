#include "bnet/GeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"

bnet::GeneratorModule::GeneratorModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fEventCnt(0),
   fUniqueId(0),
   fNumIds(1)
{
   EnsurePorts(0, 1);

   fEventCnt = 1;

   fEventsProducer = dabc::mgr.CreateObject("TestEventsProducer", "bnet-evnt-gener");

   if (fEventsProducer.null()) {
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

   if (buf.null() || fEventsProducer.null()) return false;

   fEventsProducer()->Assign(buf);

   while (fEventsProducer()->GenerateEvent(fEventCnt, fUniqueId, 200)) {
      fEventCnt++;
   }

   fEventsProducer()->Close();

   DOUT0("Send buffer size %u", buf.GetTotalSize());

   Send(port, buf);

   return true;
}

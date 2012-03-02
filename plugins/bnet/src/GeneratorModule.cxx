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
   fEventCnt(1),
   fUniqueId(0)
{
   CreatePoolHandle("BnetDataPool");

   CreateOutput("Output", Pool(), 10);
}

void bnet::GeneratorModule::BeforeModuleStart()
{
   fUniqueId = dabc::mgr.NodeId();
}

void bnet::GeneratorModule::ProcessOutputEvent(dabc::Port* port)
{
   while (port->CanSend()) {
      dabc::Buffer buf = Pool()->TakeBuffer();

      if (!FillPacket(buf)) break;

      port->Send(buf);
   }
}

bool bnet::GeneratorModule::FillPacket(dabc::Buffer& buf)
{
   if (buf.GetTotalSize() < 16) return false;

   DOUT1(("Produce event %u", (unsigned)fEventCnt));

   uint64_t data[2] = { fEventCnt++, fUniqueId };

   buf.CopyFrom(data, sizeof(data));

   return true;
}

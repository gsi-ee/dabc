#include "TestGeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Pointer.h"

#include "bnet/common.h"

bnet::TestGeneratorModule::TestGeneratorModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fEventCnt(0)
{
   fUniquieId = GetCfgInt("UniqueId", 0, cmd);

   fBufferSize = GetCfgInt(xmlReadoutBuffer, 1024, cmd);

   CreatePoolHandle(GetCfgStr(CfgReadoutPool, ReadoutPoolName, cmd).c_str());

   CreateOutput("Output", Pool(), ReadoutQueueSize);

   DOUT1(("Test Generator %s: UniqueId:%llu", GetName(), fUniquieId));
}

void bnet::TestGeneratorModule::ProcessOutputEvent(dabc::Port* port)
{
   if (!port->CanSend()) {
      EOUT(("Should not happen with generator"));
      return;
   }

   dabc::Buffer* buf = Pool()->TakeBuffer(fBufferSize);
   if (buf==0) {
      EOUT(("No free buffer - generator will block"));
      return;
   }

   buf->SetDataSize(fBufferSize);

   fEventCnt++;

   dabc::Pointer ptr(buf);

   ptr.copyfrom(&fEventCnt, sizeof(fEventCnt));
   ptr+=sizeof(fEventCnt);

   ptr.copyfrom(&fUniquieId, sizeof(fUniquieId));
   ptr+=sizeof(fUniquieId);

   DOUT3(("Generate packet id %llu of size %d need %d", fUniquieId, buf->GetTotalSize(), fBufferSize));

   port->Send(buf);
}

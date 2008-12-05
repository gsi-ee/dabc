#include "TestGeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Pointer.h"

#include "bnet/common.h"

bnet::TestGeneratorModule::TestGeneratorModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0),
   fEventCnt(0)
{
   DOUT1(("Generator %s: Create pool %s", GetName(), GetCfgStr(CfgReadoutPool, ReadoutPoolName, cmd).c_str()));

   fPool = CreatePool(GetCfgStr(CfgReadoutPool, ReadoutPoolName, cmd));

   fBufferSize = GetCfgInt(xmlReadoutBuffer, 1024, cmd);

   CreateOutput("Output", fPool, ReadoutQueueSize);

   CreateParInt("UniqueId", 0);
}

void bnet::TestGeneratorModule::BeforeModuleStart()
{
   fUniquieId = GetParInt("UniqueId", 0);

//   DOUT1(("TestGeneratorModule::BeforeModuleStart fUniquieId = %d blocked %s size %u",
//            fUniquieId, DBOOL(Output(0)->OutputBlocked()), Output(0)->OutputQueueCapacity()));

    DOUT1(("TestGeneratorModule::BeforeModuleStart %s %p %u", GetName(), Output(0), Output(0)->OutputQueueCapacity()));

   for (unsigned n=0;n<Output(0)->OutputQueueCapacity();n++)
     GeneratePacket();
}

void bnet::TestGeneratorModule::AfterModuleStop()
{
//   DOUT1(("TestGeneratorModule::AfterModuleStart fUniquieId = %d blocked %s",
//          fUniquieId, DBOOL(Output(0)->OutputBlocked())));
}

void bnet::TestGeneratorModule::ProcessOutputEvent(dabc::Port* port)
{
   GeneratePacket();
}

void bnet::TestGeneratorModule::ProcessInputEvent(dabc::Port* port)
{
   GeneratePacket();
}

void bnet::TestGeneratorModule::ProcessPoolEvent(dabc::PoolHandle* pool)
{
   GeneratePacket();
}

bool bnet::TestGeneratorModule::GeneratePacket()
{
   if (Output(0)->OutputBlocked()) return false;

   dabc::Buffer* buf = fPool->TakeBuffer(fBufferSize, true);
   if (buf==0) return false;

   buf->SetDataSize(fBufferSize);

   fEventCnt++;

   dabc::Pointer ptr(buf);

   ptr.copyfrom(&fEventCnt, sizeof(fEventCnt));
   ptr+=sizeof(fEventCnt);

   ptr.copyfrom(&fUniquieId, sizeof(fUniquieId));
   ptr+=sizeof(fUniquieId);

   DOUT3(("Generate packet id %llu of size %d need %d", fUniquieId, buf->GetTotalSize(), fBufferSize));

   Output(0)->Send(buf);

   return true;
}

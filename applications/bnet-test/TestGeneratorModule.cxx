#include "TestGeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Pointer.h"

#include "bnet/WorkerApplication.h"

bnet::TestGeneratorModule::TestGeneratorModule(const char* name,
                                               WorkerApplication* factory) :
   dabc::ModuleAsync(name),
   fPool(0),
   fEventCnt(0)
{
   fPool = CreatePool(factory->ReadoutPoolName());

   fBufferSize = factory->ReadoutBufferSize();

   CreateOutput("Output", fPool, ReadoutQueueSize);

   CreateIntPar("UniqueId", 0);
}

void bnet::TestGeneratorModule::BeforeModuleStart()
{
   fUniquieId = GetParInt("UniqueId", 0);

//   DOUT1(("TestGeneratorModule::BeforeModuleStart fUniquieId = %d blocked %s",
//          fUniquieId, DBOOL(Output(0)->OutputBlocked())));

   for (int n=0;n<ReadoutQueueSize;n++)
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

//   DOUT1(("Generate packet id %llu of size %d need %d", fUniquieId, buf->GetTotalSize(), fBufferSize));

   Output(0)->Send(buf);

   return true;
}

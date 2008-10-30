#include "dabc/PoolHandle.h"

#include "dabc/Module.h"
#include "dabc/MemoryPool.h"
#include "dabc/Parameter.h"
#include "dabc/threads.h"

dabc::PoolHandle::PoolHandle(Basic* parent, const char* name,
                                 BufferNum_t number, BufferNum_t increment, BufferSize_t size) :
   ModuleItem(mitPool, parent, name),
   MemoryPoolRequster(),
   fPool(0),
   fRequiredNumber(number),
   fRequiredIncrement(increment),
   fRequiredSize(size),
   fRequiredHeaderSize(0),
   fUsagePar(0),
   fUpdateTimeout(-1)
{
}

dabc::PoolHandle::~PoolHandle()
{
   DisconnectPool(); 
}

void dabc::PoolHandle::SetUsageParameter(Parameter* par, double interval) 
{ 
   fUsagePar = par; 
   
   fUpdateTimeout = interval;
   
   ActivateTimeout(fUsagePar!=0 ? fUpdateTimeout : -1);
}

void dabc::PoolHandle::AssignPool(MemoryPool *pool)
{
   DisconnectPool(); 
   
   fPool = pool;
}

void dabc::PoolHandle::DisconnectPool()
{
   if (fPool) fPool->RemoveRequester(this);
   fPool = 0;
}

double dabc::PoolHandle::ProcessTimeout(double)
{
   if (fUsagePar==0) return -1;
   
   fUsagePar->SetDouble(UsedRatio());
   
   return fUpdateTimeout;
}

dabc::Buffer* dabc::PoolHandle::TakeEmptyBuffer(BufferSize_t hdrsize)
{
   return fPool ? fPool->TakeEmptyBuffer(hdrsize) : 0;
}

dabc::Buffer* dabc::PoolHandle::TakeBuffer(BufferSize_t size, bool withrequest)
{
   if (fPool==0) return 0; 
    
   return withrequest ? fPool->TakeBufferReq(this, size, 0) : fPool->TakeBuffer(size, 0);
}

dabc::Buffer* dabc::PoolHandle::TakeRequestedBuffer()
{
   return fPool->TakeRequestedBuffer(this); 
}

bool dabc::PoolHandle::ProcessPoolRequest() 
{ 
   // inform our thread that we have buffer from the pool
   FireEvent(evntPool);
   
   return true;
}

void dabc::PoolHandle::ProcessEvent(EventId evid)
{
   switch (GetEventCode(evid)) {
      case evntPool: 
         ProduceUserEvent(evntPool);
         break; 
         
      default:
         ModuleItem::ProcessEvent(evid);
   }
}

void dabc::PoolHandle::AddPortRequirements(BufferNum_t number, BufferSize_t headersize)
{
   fRequiredNumber += number;
   if (fRequiredHeaderSize<headersize) fRequiredHeaderSize = headersize;
}

double dabc::PoolHandle::UsedRatio() const
{
   if (fPool==0) return 0.; 
    
   uint64_t totalsize = fPool->GetTotalSize();
   uint64_t usedsize = fPool->GetUsedSize();
   
   return totalsize>0 ? 1.* usedsize / totalsize : 0.;
}

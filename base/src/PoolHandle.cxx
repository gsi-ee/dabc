#include "dabc/PoolHandle.h"

#include "dabc/Module.h"
#include "dabc/MemoryPool.h"
#include "dabc/Parameter.h"
#include "dabc/threads.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

dabc::PoolHandle::PoolHandle(Basic* parent, const char* name, MemoryPool* pool,
                             BufferSize_t size, BufferNum_t number, BufferNum_t increment) :
   ModuleItem(mitPool, parent, name),
   MemoryPoolRequester(),
   fPool(pool),
   fRequiredNumber(number),
   fRequiredIncrement(increment),
   fRequiredSize(size),
   fRequiredHeaderSize(0),
   fUsagePar(0),
   fUpdateTimeout(-1),
   fStoreHandle(false)
{
   if ((number>0) || (increment>0) || (size>0)) {
      fStoreHandle = (pool==0) || !pool->IsMemLayoutFixed();
      if (size>0) fRequiredSize = GetCfgInt(xmlBufferSize, size);
      if (number>0) fRequiredNumber = GetCfgInt(xmlNumBuffers, number);
      if (increment>0) fRequiredIncrement = GetCfgInt(xmlNumIncrement, increment);
   }
}

dabc::PoolHandle::~PoolHandle()
{
   DisconnectPool();
}

void dabc::PoolHandle::SetUsageParameter(Parameter* par, double interval)
{
   fUsagePar = dynamic_cast<DoubleParameter*> (par);

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

bool dabc::PoolHandle::Store(ConfigIO &cfg)
{
   if (!fStoreHandle) return true;

   cfg.CreateItem(clPoolHandle);

   cfg.CreateAttr(xmlNameAttr, GetName());

   StoreChilds(cfg);

   cfg.PopItem();

   return true;
}

bool dabc::PoolHandle::Find(ConfigIO &cfg)
{
   if (!fStoreHandle) return true;

   if (!cfg.FindItem(clPoolHandle)) return false;

   return cfg.CheckAttr(xmlNameAttr, GetName());
}

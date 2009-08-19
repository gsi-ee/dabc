/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
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
   if ((number>0) || (increment>0) || (size>0))
      fStoreHandle = !fPool->IsMemLayoutFixed();

   if (size>0) fRequiredSize = GetCfgInt(xmlBufferSize, size);
   if (number>0) fRequiredNumber = GetCfgInt(xmlNumBuffers, number);
   if (increment>0) fRequiredIncrement = GetCfgInt(xmlNumIncrement, increment);

   if (fStoreHandle) {
      if (fRequiredSize==0) fRequiredSize = dabc::DefaultBufferSize;
      fPool->AddMemReq(fRequiredSize, fRequiredNumber, fRequiredIncrement, 0);
      fPool->AddRefReq(0, fRequiredNumber*2, fRequiredIncrement, 0);
   }
}

dabc::PoolHandle::~PoolHandle()
{
   if (fPool) fPool->RemoveRequester(this);
}

void dabc::PoolHandle::SetUsageParameter(Parameter* par, double interval)
{
   fUsagePar = dynamic_cast<DoubleParameter*> (par);

   fUpdateTimeout = interval;

   ActivateTimeout(fUsagePar!=0 ? fUpdateTimeout : -1);
}

double dabc::PoolHandle::ProcessTimeout(double)
{
   if (fUsagePar==0) return -1;

   fUsagePar->SetDouble(UsedRatio());

   return fUpdateTimeout;
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
   // we must return false if we not intend to search handle,
   // if return true, dabc::Configuration class think that hierarchy exists and will try to travel that hierarchy back
   if (!fStoreHandle) return false;

   if (!cfg.FindItem(clPoolHandle)) return false;

   return cfg.CheckAttr(xmlNameAttr, GetName());
}

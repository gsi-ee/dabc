// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/PoolHandle.h"

#include "dabc/Module.h"
#include "dabc/MemoryPool.h"
#include "dabc/Parameter.h"
#include "dabc/threads.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

dabc::PoolHandle::PoolHandle(Reference parent, const char* name, Reference pool) :
   ModuleItem(mitPool, parent, name),
   MemoryPoolRequester(),
   fPool(pool),
   fUsagePar(),
   fUpdateTimeout(-1)
{
   // FIXME - should never happen

   if (dabc::mgr())
      dabc::mgr()->RegisterDependency(this, fPool());
   else
      EOUT(("dabc::mgr() empty when pool handle %s tries to register dependency", GetName()));
}

dabc::PoolHandle::~PoolHandle()
{
}

dabc::Reference dabc::PoolHandle::GetPoolRef()
{
   return fPool.Ref();
}

void dabc::PoolHandle::ObjectCleanup()
{
   DOUT4(("PoolHandle::ObjectCleanup pool = %p", fPool()));

   if (!fPool.null()) {
      fPool()->RemoveRequester(this);

   // FIXME - should never happen
      if (dabc::mgr()) 
         dabc::mgr()->UnregisterDependency(this, fPool());
      else
         EOUT(("dabc::mgr() no longer exists when cleanup of pool handle %s ptr:%p", GetName(), this));
      fPool.Release();
   }

   dabc::ModuleItem::ObjectCleanup();
}

void dabc::PoolHandle::ObjectDestroyed(Object* obj)
{
   if (obj==fPool()) {

      DOUT4(("PoolHandle::ObjectDestroyed pool = %p", fPool()));

      ThisItemCleaned();
      fPool()->RemoveRequester(this);
      fPool.Release();
   }

   dabc::ModuleItem::ObjectDestroyed(obj);
}

void dabc::PoolHandle::SetUsageParameter(const std::string& parname, double interval)
{
   fUsagePar = parname;

   fUpdateTimeout = interval;

   ActivateTimeout(fUsagePar.empty() ? -1: fUpdateTimeout);
}

double dabc::PoolHandle::ProcessTimeout(double)
{
   if (fUsagePar.empty()) return -1;

   GetModule()->Par(fUsagePar).SetDouble(UsedRatio());

   return fUpdateTimeout;
}

bool dabc::PoolHandle::ProcessPoolRequest()
{
   // inform our thread that we have buffer from the pool
   FireEvent(evntPool);

   return true;
}

void dabc::PoolHandle::ProcessEvent(const EventId& evid)
{
   switch (evid.GetCode()) {
      case evntPool:
         ProduceUserEvent(evntPool);
         break;

      default:
         ModuleItem::ProcessEvent(evid);
   }
}

double dabc::PoolHandle::UsedRatio() const
{
   return fPool.GetUsedRatio();
}

// FIXME: do we need this method at all??? PoolHandle no use in xml file

bool dabc::PoolHandle::Find(ConfigIO &cfg)
{
   // we must return false if we not intend to search handle,
   // if return true, dabc::Configuration class think that hierarchy exists and will try to travel that hierarchy back

   while (cfg.FindItem(clPoolHandle)) {
      if (cfg.CheckAttr(xmlNameAttr, GetName())) return true;
   }

   return false;


}

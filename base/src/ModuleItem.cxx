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

#include "dabc/ModuleItem.h"

#include "dabc/Module.h"
#include "dabc/Manager.h"


dabc::ModuleItem::ModuleItem(int typ, Reference parent, const std::string &name) :
   Worker(parent, name.c_str()),
   fItemType(typ),
   fItemId(0),
   fSubId(0)
{
   // we should reinitialize variable, while it will be cleared by constructor
   parent = GetParentRef();

   Module* m = dynamic_cast<Module*> (GetParent());

   if (m==0)
      throw dabc::Exception(ex_Generic, "Item created without module or in wrong place of hierarchy", ItemName());

   // use module priority for the all port items
   SetItemPriority(m->WorkerPriority());
}

dabc::ModuleItem::~ModuleItem()
{
   DOUT2("ModuleItem:%p start destructor", this);

   Module* m = dynamic_cast<Module*> (GetParent());

   if (m) m->RemoveModuleItem(this);

   DOUT2("ModuleItem:%p did destructor", this);
}


void dabc::ModuleItem::StopModule()
{
   Module* m = dynamic_cast<Module*> (GetParent());
   if (m) m->DoStop();
}


void dabc::ModuleItem::StartModule()
{
   Module* m = dynamic_cast<Module*> (GetParent());
   if (m) m->DoStart();
}


// ========================================================

dabc::WorkerRef dabc::ModuleItemRef::GetModule() const
{
   Reference parent = GetParent();

   while (!parent.null()) {
      ModuleRef module = parent;
      if (!module.null()) return module;
      parent = parent.GetParent();
   }

   return dabc::WorkerRef();
}

std::string dabc::ModuleItemRef::InfoParName() const
{
   ModuleRef module = GetModule();
   return module.InfoParName();
}

// ==============================================================================


dabc::Timer::Timer(Reference parent, bool systimer, const std::string &name, double period, bool synchron) :
   ModuleItem(mitTimer, parent, name),
   fSysTimer(systimer),
   fPeriod(period),
   fCounter(0),
   fActive(false),
   fSynhron(synchron),
   fInaccuracy(0.)
{
}


dabc::Timer::~Timer()
{
}

void dabc::Timer::DoStart()
{
   fActive = true;

   // for sys timer timeout will be activated by module itself
   if ((fPeriod>0.) && !IsSysTimer())
      ActivateTimeout(fPeriod);
}

void dabc::Timer::DoStop()
{
   fActive = false;

   // for sys timer timeout will be deactivated by module itself
   if (!IsSysTimer()) ActivateTimeout(-1);
}


double dabc::Timer::ProcessTimeout(double last_diff)
{
   if (!fActive) return -1.;

   fCounter++;

   Module* m = (Module*) GetParent();

   if (m && m->IsRunning())
      m->ProcessItemEvent(this, evntTimeout);

   // if we have not synchron timer, just fire next event
   if (!fSynhron) return fPeriod;

   fInaccuracy += (last_diff - fPeriod);

   double res = fPeriod - fInaccuracy;
   if (res < 0) res = 0;

   //DOUT0("Timer %s lastdif = %5.4f next %5.4f", GetName(), last_diff, res);

   return res;
}


// ==============================================================================

dabc::ConnTimer::ConnTimer(Reference parent, const std::string &name, const std::string &portname) :
   ModuleItem(mitConnTimer, parent, name),
   fPortName(portname),
   fErrorFlag(false)
{
}

double dabc::ConnTimer::ProcessTimeout(double)
{
   Module* m = dynamic_cast<Module*> (GetParent());

   // DOUT0("ConnTimer::ProcessTimeout m = %s", DNAME(m));

   if (m==0) return -1;

   PortRef port = m->FindPort(fPortName);
   if (port.null()) return -1.;

   std::string itemname = port.ItemName();

   if (!port()->IsDoingReconnect()) return -1;

   // DOUT0("Trying to reconnect port %s", itemname.c_str());

   if (port.IsConnected() || dabc::mgr.CreateTransport(itemname)) {
      port()->SetDoingReconnect(false);
      // DOUT0("Port %s connected again", itemname.c_str());
      return -1.;
   }

   if (!port()->TryNextReconnect(fErrorFlag)) return -1;

   return port()->GetReconnectPeriod() > 0 ? port()->GetReconnectPeriod() : 1.;
}

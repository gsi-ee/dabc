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

#include "dabc/Module.h"

#include <vector>

#include "dabc/logging.h"
#include "dabc/threads.h"

#include "dabc/ModuleItem.h"
#include "dabc/PoolHandle.h"
#include "dabc/Manager.h"
#include "dabc/Iterator.h"
#include "dabc/Device.h"
#include "dabc/Port.h"
#include "dabc/Timer.h"
#include "dabc/Application.h"

// __________________________________________________________________

dabc::Module::Module(const char* name, Command cmd) :
   Worker(MakePair(name ? name : cmd.GetStr("Name","module"))),
   fRunState(false),
   fInputPorts(),
   fOutputPorts(),
   fPorts(),
   fPoolHandels(),
   fLostEvents(128)
{
   // we will use 3 priority levels:
   //  0 - normal for i/o ,
   //  1 - for commands and replies,
   //  2 - for sys commands (in modules thread itself)

//   CreateCmdDef("SetPriority").AddArg("Priority", "int", true);
//
//   CreateCmdDef(CmdSetParameter::CmdName()).AddArg("ParName", "string", true).AddArg("ParValue", "string", true);
}

dabc::Module::~Module()
{
   DOUT2(( "dabc::Module::~Module() %s starts", GetName()));

   if (fRunState) EOUT(("Module %s destructed in running state", GetName()));

   DOUT2((" dabc::Module::~Module() %s done", GetName()));
}

void dabc::Module::OnThreadAssigned()
{
   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item && !item->HasThread())
         item->AssignToThread(thread(), true);
   }
}


bool dabc::Module::CreatePoolUsageParameter(const char* name, double interval, const char* poolname)
{
   PoolHandle* pool = FindPool(poolname);
   if (pool==0) return false;

   CreatePar(name).SetDouble(0.);
   pool->SetUsageParameter(name, interval);

   return true;
}

dabc::Timer* dabc::Module::CreateTimer(const char* name, double period_sec, bool synchron)
{
   return new Timer(GetTimersFolder(true), name, period_sec, synchron);
}

dabc::Timer* dabc::Module::FindTimer(const char* name)
{
   Reference ref = GetTimersFolder().FindChild(name);

   return dynamic_cast<dabc::Timer*> (ref());
}

bool dabc::Module::ShootTimer(const char* name, double delay_sec)
{
   Timer* t = FindTimer(name);

   if (t==0) return false;

   t->SingleShoot(delay_sec);

   return true;
}

dabc::ModuleItem* dabc::Module::CreateUserItem(const char* name)
{
   return new ModuleItem(mitUser, this, name);
}

dabc::ModuleItem* dabc::Module::FindUserItem(const char* name)
{
   Reference ref = FindChild(name);

   ModuleItem* item = dynamic_cast<dabc::ModuleItem*> (ref());

   return item && (item->GetType()==mitUser) ? item : 0;

}

bool dabc::Module::ProduceUserItemEvent(const char* name)
{
   ModuleItem* item = FindUserItem(name);

   if (item==0) return false;

   item->FireUserEvent();

   return true;
}


void dabc::Module::Start()
{
   DOUT3(("Start module %s thrd %s", GetName(), ThreadName().c_str()));

   Execute("StartModule");
}

void dabc::Module::Stop()
{
   Execute("StopModule");

   DOUT4(("Stop module %s thrd %s done", GetName(), ThreadName().c_str()));
}

int dabc::Module::PreviewCommand(Command cmd)
{
   // this hook in command execution routine allows us to "preview"
   // command before it actually executed
   // if it is system command, just execute it immediately

   int cmd_res = cmd_ignore;

   DOUT3(("Module:%s PreviewCommand %s", GetName(), cmd.GetName()));

   if (cmd.IsName("StartModule"))
      cmd_res = cmd_bool(DoStart());
   else
   if (cmd.IsName("StopModule"))
      cmd_res = cmd_bool(DoStop());
   else
   if (cmd.IsName("SetPriority")) {
      if (fThread()) {
         fThread()->SetPriority(cmd.GetInt("Priority",0));
         cmd_res = cmd_true;
      } else
         cmd_res = cmd_false;
   } else
   if (cmd.IsName("CheckConnected")) {
      cmd_res = cmd_true;
      for (unsigned n=0;n<NumIOPorts();n++)
         if (IOPort(n) && !IOPort(n)->IsConnected()) cmd_res = cmd_false;
   } else
   if (cmd.IsName("IsInputConnect")) {
      unsigned ninp = cmd.GetUInt("Number");
      cmd_res = cmd_bool((ninp<NumInputs()) && Input(ninp)->IsConnected());
   } else
   if (cmd.IsName("IsOutputConnect")) {
      unsigned nout = cmd.GetUInt("Number");
      cmd_res = cmd_bool((nout<NumOutputs()) && Output(nout)->IsConnected());
   } else
      cmd_res = Worker::PreviewCommand(cmd);

   if (cmd_res!=cmd_ignore)
      DOUT3(("Module:%s PreviewCommand %s res=%d", GetName(), cmd.GetName(), cmd_res));

   return cmd_res;
}


void dabc::Module::ObjectCleanup()
{
   DOUT3(("Module cleanup %s numchilds %u", GetName(), NumChilds()));

   if (IsRunning()) DoStop();

   ModuleCleanup();

   dabc::Worker::ObjectCleanup();

   DOUT3(("Module cleanup %s done numchilds %u", GetName(), NumChilds()));

}

bool dabc::Module::DoStart()
{
   if (IsRunning()) return true;

   BeforeModuleStart();

   fRunState = true;

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item) item->DoStart();
   }

   if (fLostEvents.Size()>0) FireEvent(evntReinjectlost);

   return true;
}

bool dabc::Module::DoStop()
{
   DOUT3(("Module::DoStop %s", GetName()));

   if (!IsRunning()) return true;

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item) item->DoStop();
   }

   fRunState = false;

   AfterModuleStop();

   DOUT3(("Module::DoStop %s done", GetName()));

   return true;
}

dabc::PoolHandle* dabc::Module::CreatePoolHandle(const char* poolname)
{
   dabc::PoolHandle* handle = FindPool(poolname);
   if (handle!=0) return handle;

   dabc::Reference pool = dabc::mgr()->FindPool(poolname);
   if (pool.null()) {
      dabc::mgr.CreateMemoryPool(poolname);
      pool = dabc::mgr()->FindPool(poolname);
   }

   if (pool.null()) {
      EOUT(("Cannot create/find pool with name %s", poolname));
      return 0;
   }

   handle = new dabc::PoolHandle(this, poolname, pool);
   fPoolHandels.push_back(handle->ItemId());

   return handle;
}

dabc::Reference dabc::Module::GetTimersFolder(bool force)
{
   return GetFolder("Timers", force, true);
}

void dabc::Module::ItemCleaned(ModuleItem* item)
{
   PoolHandle* p = dynamic_cast<PoolHandle*> (item);

   if (p!=0) PoolHandleCleaned(p);
}


void dabc::Module::ItemCreated(ModuleItem* item)
{
   // at that place one cannot use any dynamic_cast to inherited types,
   // while constructor of item is not completely finished

   unsigned id = fItems.size();

   fItems.push_back(item);

   item->SetItemId(id);

   if (id>moduleitemMaxId) {
      EOUT(("Item id is too big, event propagation will not work"));
      exit(104);
   }

   if (HasThread())
      item->AssignToThread(thread(), true);

//   DOUT1(("Module:%s Add item:%s Id:%d", GetName(), item->GetName(), id));
}

void dabc::Module::ItemDestroyed(ModuleItem* item)
{
   unsigned id = item->ItemId();

   for (unsigned n=0;n<fInputPorts.size();n++)
      if (fInputPorts[n] == id) {
         fInputPorts.erase(fInputPorts.begin()+n);
         break;
      }

   for (unsigned n=0;n<fOutputPorts.size();n++)
      if (fOutputPorts[n] == id) {
         fOutputPorts.erase(fOutputPorts.begin()+n);
         break;
      }

   for (unsigned n=0;n<fPorts.size();n++)
      if (fPorts[n] == id) {
         fPorts.erase(fPorts.begin()+n);
         break;
      }

   for (unsigned n=0;n<fPoolHandels.size();n++)
      if (fPoolHandels[n] == id) {
         fPoolHandels.erase(fPoolHandels.begin()+n);
         break;
      }

   unsigned n = 0;
   while (n<fLostEvents.Size())
      if (id == fLostEvents.Item(n).GetItem())
         fLostEvents.RemoveItem(n);
      else
         n++;

   fItems[id] = 0;
}

unsigned dabc::Module::InputNumber(Port* port)
{
   for (unsigned n=0;n<fInputPorts.size();n++)
      if (GetPortItem(fInputPorts[n]) == port) return n;
   return (unsigned) -1;
}

unsigned dabc::Module::OutputNumber(Port* port)
{
   for (unsigned n=0;n<fOutputPorts.size();n++)
      if (GetPortItem(fOutputPorts[n]) == port) return n;
   return (unsigned) -1;
}

unsigned dabc::Module::IOPortNumber(Port* port)
{
   for (unsigned n=0;n<fPorts.size();n++)
      if (GetPortItem(fPorts[n]) == port) return n;
   return (unsigned) -1;
}

dabc::Port* dabc::Module::FindPort(const char* name)
{
   return dynamic_cast<Port*> (FindChild(name));
}

dabc::PoolHandle* dabc::Module::FindPool(const char* name)
{
   return dynamic_cast<dabc::PoolHandle*> (FindChild(name));
}

dabc::PoolHandle* dabc::Module::Pool(unsigned n) const
{
   if (n>=fPoolHandels.size()) return 0;
   ModuleItem* item = GetItem(fPoolHandels[n]);
   return item && (item->GetType()==mitPool) ? (PoolHandle*) item : 0;
}

dabc::Port* dabc::Module::GetPortItem(unsigned id) const
{
   ModuleItem* item = GetItem(id);
   return item && (item->GetType()==mitPort) ? (Port*) item : 0;
}

dabc::Port* dabc::Module::CreateIOPort(const char* name, PoolHandle* handle, unsigned recvqueue, unsigned sendqueue)
{
   if ((recvqueue==0) && (sendqueue==0)) {
      EOUT(("Both receive and send queue length are zero - port %s not created", name));
      return 0;
   }

   if (handle==0) handle = Pool();

   Port* port = new Port(this, name, handle, recvqueue, sendqueue);

   fPorts.push_back(port->ItemId());
   if (recvqueue>0) fInputPorts.push_back(port->ItemId());
   if (sendqueue>0) fOutputPorts.push_back(port->ItemId());

   return port;
}

void dabc::Module::GetUserEvent(ModuleItem* item, uint16_t evid)
{
   if (IsRunning())
      ProcessItemEvent(item, evid);
   else
      fLostEvents.Push(EventId(evid, item->ItemId()));
}

void dabc::Module::ProcessEvent(const EventId& evid)
{
   switch (evid.GetCode()) {
      case evntReinjectlost:
         DOUT5(("Module %s Reinject lost event num %u", GetName(), fLostEvents.Size()));

         if (fLostEvents.Size()>0) {
            EventId user_evnt = fLostEvents.Pop();

            ModuleItem* item = GetItem(user_evnt.GetItem());

            DOUT5(("Module %s Item %s Lost event %u", GetName(), DNAME(item), user_evnt.GetCode()));

            if (item!=0)
               GetUserEvent(item, user_evnt.GetCode());
            else
               EOUT(("Module:%s Event %u for item %u lost completely",
                     GetName(), user_evnt.GetCode(), user_evnt.GetItem()));
         }

         if (fLostEvents.Size() > 0) FireEvent(evntReinjectlost);
         break;

      default:
         dabc::Worker::ProcessEvent(evid);
         break;
   }
}

bool dabc::Module::CanSendToAllOutputs() const
{
   for(unsigned n=0;n<NumOutputs();n++)
      if (!Output(n)->CanSend()) return false;
   return true;
}

void dabc::Module::SendToAllOutputs(const dabc::Buffer& buf)
{
   if (buf.null()) return;
   if (NumOutputs()==0) {
      (const_cast<dabc::Buffer*>(&buf))->Release();
      return;
   }
   unsigned n = 0;
   while (n < NumOutputs()-1)
      Output(n++)->Send(buf.Duplicate());
   Output(n)->Send(buf);
}


bool dabc::ModuleRef::IsInputConnected(unsigned ninp)
{
   if (GetObject()==0) return false;

   dabc::Command cmd("IsInputConnect");
   cmd.SetInt("Number", ninp);
   return Execute(cmd) == cmd_true;
}

/** Returns true if specified output is connected */
bool dabc::ModuleRef::IsOutputConnected(unsigned ninp)
{
   if (GetObject()==0) return false;

   dabc::Command cmd("IsOutputConnect");
   cmd.SetInt("Number", ninp);
   return Execute(cmd) == cmd_true;
}

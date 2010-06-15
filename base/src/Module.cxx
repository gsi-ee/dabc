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
#include "dabc/Parameter.h"
#include "dabc/Application.h"
#include "dabc/CommandDefinition.h"

// __________________________________________________________________

dabc::Module::Module(const char* name, Command* cmd) :
   Folder(dabc::mgr(), (name ? name : (cmd ? cmd->GetStr("Name","module") : "module")), true),
   WorkingProcessor(this),
   fRunState(msStopped),
   fInputPorts(),
   fOutputPorts(),
   fPorts(),
   fPoolHandels(),
   fLostEvents(128, true)
{
   // we will use 3 priority levels:
   //  0 - normal for i/o ,
   //  1 - for commands and replies,
   //  2 - for sys commands (in modules thread itself)

//   CommandDefinition* def = NewCmdDef("SetPriority");
//   def->AddArgument("Priority", argInt, true);
//   def->Register(true);
//
//   def = NewCmdDef(CmdSetParameter::CmdName());
//   def->AddArgument("ParName", argString, true);
//   def->AddArgument("ParValue", argString, true);
//   def->Register(true);
}

dabc::Module::~Module()
{
   DOUT3(( "dabc::Module::~Module() %s starts", GetName()));

   if (fRunState != msHalted) {
      DoHalt();

      HaltProcessor();

      // by this we stop any further events processing and can be sure that
      // destroying module items will not cause seg.fault
      RemoveProcessorFromThread(true);
   }

   // one should call destructor of children here, while
   // all ModuleItem objects will try to access Module method ItemDestroyed
   // If it happens in Basic destructor, object as it is no longer exists

   DOUT3(("Module:%s deletes all parameters", GetName()));

   DestroyAllPars();

   DOUT3(("Module:%s deletes all childs", GetName()));

   DeleteChilds();

   DOUT3((" dabc::Module::~Module() %s done", GetName()));
}

dabc::WorkingProcessor* dabc::Module::GetCfgMaster()
{
   return dabc::mgr()->GetApp();
}

void dabc::Module::OnThreadAssigned()
{
   if (IsHalted()) {
      EOUT(("Module %s must be deleted", GetName()));
      return;
   }

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item && (item->ProcessorThread()==0))
         item->AssignProcessorToThread(ProcessorThread(), true);
   }
}

dabc::RateParameter* dabc::Module::CreateRateParameter(const char* name, bool sync, double interval,
                                                       const char* inpportname, const char* outportname,
                                                       const char* units, double lower, double upper,
                                                       int debug_width, int debug_prec)
{
   Port* inport = FindPort(inpportname);
   Port* outport = FindPort(outportname);

   if ((inport!=0) || (outport!=0)) units = "MB/s";

   RateParameter* par = new RateParameter(this, name, sync, interval, units, lower, upper, debug_width, debug_prec);

   if (inport) inport->SetInpRateMeter(par);

   if (outport) outport->SetOutRateMeter(par);

   return par;
}

dabc::Parameter* dabc::Module::CreatePoolUsageParameter(const char* name, double interval, const char* poolname)
{
   Parameter* par = CreateParDouble(name, 0.);

   PoolHandle* pool = FindPool(poolname);
   if (pool) pool->SetUsageParameter(par, interval);

   return par;
}

dabc::Timer* dabc::Module::CreateTimer(const char* name, double period_sec, bool synchron)
{
   return new Timer(GetTimersFolder(true), name, period_sec, synchron);
}

dabc::Timer* dabc::Module::FindTimer(const char* name)
{
   Folder* f = GetTimersFolder(false);

   return f==0 ? 0 : dynamic_cast<dabc::Timer*> (f->FindChild(name));
}

bool dabc::Module::ShootTimer(const char* name, double delay_sec)
{
   Timer* t = FindTimer(name);

   if (t==0) return false;

   t->SingleShoot(delay_sec);

   return true;
}

void dabc::Module::Start()
{
   DOUT3(("Start module %s thrd %s", GetName(), DNAME(ProcessorThread())));

   Execute("StartModule");
}

void dabc::Module::Stop()
{
   Execute("StopModule");

   DOUT4(("Stop module %s thrd %s done", GetName(), DNAME(ProcessorThread())));
}

bool dabc::Module::Halt()
{
   DOUT3(("Halt module %s  %p", GetName(), dynamic_cast<WorkingProcessor*> (this)));

   bool res = (Execute("HaltModule")==cmd_true);

   DOUT3(("Halt module %s after exec %s issamethrd %s  cmdslvl:%d", GetName(), DBOOL(res), DBOOL(dabc::mgr()->CurrentThread()==ProcessorThread()), fProcessorCommandsLevel));

//   if (IsName("Module9") || IsName("Module8")) dabc::SetDebugLevel(5);

   if (!HaltProcessor()) res = false;

   DOUT3(("Halt module %s  res %s", GetName(), DBOOL(res)));

   RemoveProcessorFromThread(true);

   DOUT3(("Remove from thread %s", GetName()));

   return res;
}

int dabc::Module::PreviewCommand(Command* cmd)
{
   // this hook in command execution routine allows us to "preview"
   // command before it actually executed
   // if it is system command, just execute it immediately

   int cmd_res = cmd_ignore;

   DOUT3(("Module:%s PreviewCommand %s", GetName(), cmd->GetName()));

   if (cmd->IsName("StartModule"))
      cmd_res = cmd_bool(DoStart());
   else
   if (cmd->IsName("StopModule"))
      cmd_res = cmd_bool(DoStop());
   else
   if (cmd->IsName("HaltModule")) {
      cmd_res = DoHalt();
   } else
   if (cmd->IsName("SetPriority")) {
      if (ProcessorThread()) {
         ProcessorThread()->SetPriority(cmd->GetInt("Priority",0));
         cmd_res = cmd_true;
      } else
         cmd_res = cmd_false;
   } else
   if (cmd->IsName(CmdCheckConnModule::CmdName())) {
      cmd_res = cmd_true;
      for (unsigned n=0;n<NumIOPorts();n++)
         if (IOPort(n) && !IOPort(n)->IsConnected()) cmd_res = cmd_false;
   } else
      cmd_res = WorkingProcessor::PreviewCommand(cmd);

   if (cmd_res!=cmd_ignore)
      DOUT3(("Module:%s PreviewCommand %s res=%d", GetName(), cmd->GetName(), cmd_res));

   return cmd_res;
}

bool dabc::Module::DoStart()
{
   if (IsRunning()) return true;

   BeforeModuleStart();

   fRunState = msRunning;

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item) item->DoStart();
   }

   if (fLostEvents.Size()>0) FireEvent(evntReinjectlost);

   return true;
}

bool dabc::Module::DoStop()
{
   DOUT4(("Module::DoStop %s", GetName()));

   if (!IsRunning()) return true;

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item) item->DoStop();
   }

   fRunState = msStopped;

   AfterModuleStop();

   DOUT4(("Module::DoStop %s done", GetName()));

   return true;
}

int dabc::Module::DoHalt()
{
   if (fRunState == msHalted) return cmd_true;

   BeforeModuleHalt();

   int res = cmd_true;

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item==0) continue;

      item->DoHalt();
      if (!item->HaltProcessor()) {
         EOUT(("Cannot halt item %s", item->GetFullName().c_str()));
         res = cmd_false;
      }
   }

   fLostEvents.Reset();

   fRunState = msHalted;

   return res;
}


dabc::PoolHandle* dabc::Module::CreatePoolHandle(const char* poolname, BufferSize_t size, BufferNum_t number, BufferNum_t increment)
{
   dabc::MemoryPool* pool = dabc::mgr()->FindPool(poolname);
   if (pool==0) {
      dabc::mgr()->CreateMemoryPool(poolname);
      pool = dabc::mgr()->FindPool(poolname);
   }

   if (pool==0) {
      EOUT(("Cannot create/find pool with name %s", poolname));
      return 0;
   }

   dabc::PoolHandle* handle = FindPool(poolname);

   if (handle==0) handle = new dabc::PoolHandle(this, poolname, pool, size, number, increment);

   fPoolHandels.push_back(handle->ItemId());

   return handle;
}

dabc::Folder* dabc::Module::GetTimersFolder(bool force)
{
   return GetFolder("Timers", force, true);
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

   if (ProcessorThread()!=0)
      item->AssignProcessorToThread(ProcessorThread(), false);

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
      if (id == GetEventItem(fLostEvents.Item(n)))
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

dabc::Port* dabc::Module::CreateIOPort(const char* name, PoolHandle* handle, unsigned recvqueue, unsigned sendqueue, BufferSize_t headersize)
{
   if ((recvqueue==0) && (sendqueue==0)) {
      EOUT(("Both receive and send queue length are zero - port %s not created", name));
      return 0;
   }

   if (handle==0) handle = Pool();

   Port* port = new Port(this, name, handle, recvqueue, sendqueue, headersize);
   if (handle) {
      BufferNum_t number = port->NumInputBuffersRequired() + port->NumOutputBuffersRequired();
      if (handle->getPool()) {
         handle->getPool()->AddMemReq(handle->GetRequiredBufferSize(), number, 0, 0);
         handle->getPool()->AddRefReq(headersize, number*2, 0, 0);
      }
   }

   fPorts.push_back(port->ItemId());
   if (recvqueue>0) fInputPorts.push_back(port->ItemId());
   if (sendqueue>0) fOutputPorts.push_back(port->ItemId());

   return port;
}

void dabc::Module::GetUserEvent(ModuleItem* item, uint16_t evid)
{
   if (IsRunning())
      ProcessUserEvent(item, evid);
   else
   if (IsStopped())
      fLostEvents.Push(CodeEvent(evid, item->ItemId()));
}

void dabc::Module::ProcessEvent(EventId evid)
{
   switch (dabc::GetEventCode(evid)) {
      case evntReinjectlost:
         DOUT5(("Module %s Reinject lost event num %u", GetName(), fLostEvents.Size()));

         if (fLostEvents.Size()>0) {
            EventId user_evnt = fLostEvents.Pop();

            ModuleItem* item = GetItem(GetEventItem(user_evnt));

            DOUT5(("Module %s Item %s Lost event %u", GetName(), DNAME(item), GetEventCode(user_evnt)));

            if (item!=0)
               GetUserEvent(item, GetEventCode(user_evnt));
            else
               EOUT(("Module:%s Event %u for item %u lost completely",
                     GetName(), GetEventCode(user_evnt), GetEventItem(user_evnt)));
         }

         if (fLostEvents.Size() > 0) FireEvent(evntReinjectlost);
         break;

      default:
         dabc::WorkingProcessor::ProcessEvent(evid);
   }
}

bool dabc::Module::CanSendToAllOutputs() const
{
   for(unsigned n=0;n<NumOutputs();n++)
      if (!Output(n)->CanSend()) return false;
   return true;
}

void dabc::Module::SendToAllOutputs(dabc::Buffer* buf)
{
   if (buf==0) return;
   if (NumOutputs()==0) {
      dabc::Buffer::Release(buf);
      return;
   }
   unsigned n = 0;
   while (n < NumOutputs()-1)
      Output(n++)->Send(buf->MakeReference());
   Output(n)->Send(buf);
}

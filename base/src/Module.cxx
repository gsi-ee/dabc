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

#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Iterator.h"
#include "dabc/Device.h"
#include "dabc/Application.h"
#include "dabc/ConfigBase.h"
#include "dabc/Hierarchy.h"

// __________________________________________________________________

dabc::Module::Module(const std::string& name, Command cmd) :
   Worker(MakePair(name.empty() ? cmd.GetStr("Name","module") : name)),
   fRunState(false),
   fInputs(),
   fOutputs(),
   fPools(),
   fTimers(),
   fSysTimerIndex((unsigned)-1),
   fAutoStop(true),
   fDfltPool(),
   fInfoParName()
{
   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStr();
   int numinp = Cfg(dabc::xmlNumInputs, cmd).AsInt(0);
   int numout = Cfg(dabc::xmlNumOutputs, cmd).AsInt(0);
   fAutoStop = Cfg("autostop", cmd).AsBool(fAutoStop);

   DOUT2("Create module %s with pool:%s numinp:%d numout:%d", GetName(), poolname.c_str(), numinp, numout);

   EnsurePorts(numinp, numout, poolname);

   // we will use 3 priority levels:
   //  0 - normal for i/o ,
   //  1 - for commands and replies,
   //  2 - for sys commands (in modules thread itself)

//   CreateCmdDef("SetPriority").AddArg("Priority", "int", true);
//
//   CreateCmdDef(CmdSetParameter::CmdName()).AddArg("ParName", "string", true).AddArg("ParValue", "string", true);

   DOUT3("++++++++++ dabc::Module::Module() %s done", GetName());
}

dabc::Module::~Module()
{
   DOUT3("dabc::Module::~Module() %s", GetName());

   if (fRunState) EOUT("Module %s destroyed in running state", GetName());
}

void dabc::Module::EnsurePorts(unsigned numinp, unsigned numout, const std::string& poolname)
{
   while (NumInputs() < numinp)
      CreateInput(format("Input%u", NumInputs()));

   while (NumOutputs() < numout)
      CreateOutput(format("Output%u", NumOutputs()));

   if (!poolname.empty() && (NumPools()==0))
      CreatePoolHandle(poolname);
}


void dabc::Module::OnThreadAssigned()
{
   DOUT5("Module %s on thread assigned", GetName());

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = fItems[n];
      if (item && !item->HasThread() && item->ItemNeedThread())
         item->AssignToThread(thread(), true);
   }
}

std::string dabc::Module::TimerName(unsigned indx, bool fullname) const
{
   if (indx >= fTimers.size()) return "";
   if (fullname) return fTimers[indx]->ItemName();
   return fTimers[indx]->GetName();
}

unsigned dabc::Module::FindTimer(const std::string& name)
{
   if (!name.empty())
      for (unsigned n=0;n<fTimers.size();n++)
         if (fTimers[n]->IsName(name.c_str())) return n;
   return (unsigned) -1;
}


unsigned dabc::Module::CreateTimer(const std::string& name, double period_sec, bool synchron)
{
   unsigned indx = FindTimer(name);
   if (IsValidTimer(indx)) return indx;

   bool systimer = !IsValidTimer(fSysTimerIndex) && (name.find("Sys")==0);

   dabc::Timer* timer = new Timer(this, systimer, name, period_sec, synchron);

   AddModuleItem(timer);

   timer->SetItemSubId(fTimers.size());

   if (systimer) fSysTimerIndex = timer->ItemSubId();

   fTimers.push_back(timer);

   return timer->ItemSubId();
}

dabc::Parameter dabc::Module::CreatePar(const std::string& name, const std::string& kind)
{
   dabc::Parameter par = dabc::Worker::CreatePar(name, kind);

   if (kind == "info") {
      std::string itemname = par.ItemName();
      dabc::LockGuard lock(ObjectMutex());
      if (fInfoParName.empty())
         fInfoParName = itemname;
   }

   return par;
}

void dabc::Module::SetInfoParName(const std::string& parname)
{
   dabc::LockGuard lock(ObjectMutex());
   fInfoParName = parname;
}


std::string dabc::Module::GetInfoParName() const
{
   dabc::LockGuard lock(ObjectMutex());
   return fInfoParName;
}


unsigned dabc::Module::FindOutput(const std::string& name) const
{
   if (!name.empty())
      for (unsigned n=0;n<fOutputs.size();n++)
         if (fOutputs[n]->IsName(name)) return n;
   return (unsigned) -1;
}

unsigned dabc::Module::FindInput(const std::string& name) const
{
   if (!name.empty())
      for (unsigned n=0;n<fInputs.size();n++)
         if (fInputs[n]->IsName(name)) return n;
   return (unsigned) -1;
}

unsigned dabc::Module::FindPool(const std::string& name) const
{
   if (!name.empty())
      for (unsigned n=0;n<fPools.size();n++)
         if (fPools[n]->IsName(name)) return n;
   return (unsigned) -1;
}

std::string dabc::Module::OutputName(unsigned indx, bool fullname) const
{
   if (indx>=fOutputs.size()) return "";
   if (fullname) return fOutputs[indx]->ItemName();
   return fOutputs[indx]->GetName();
}

std::string dabc::Module::InputName(unsigned indx, bool fullname) const
{
   if (indx>=fInputs.size()) return "";
   if (fullname) return fInputs[indx]->ItemName();
   return fInputs[indx]->GetName();
}

std::string dabc::Module::PoolName(unsigned indx, bool fullname) const
{
   if (indx>=fPools.size()) return "";
   if (fullname) return fPools[indx]->ItemName();
   return fPools[indx]->GetName();
}


unsigned dabc::Module::CreateUserItem(const std::string& name)
{
   unsigned indx = FindUserItem(name);
   if (IsValidUserItem(indx)) return indx;

   ModuleItem* item = new ModuleItem(mitUser, this, name);

   AddModuleItem(item);

   item->SetItemSubId(fUsers.size());

   fUsers.push_back(item);

   return item->ItemSubId();
}

unsigned dabc::Module::FindUserItem(const std::string& name)
{
   if (!name.empty())
      for (unsigned n=0;n<fUsers.size();n++)
         if (fUsers[n]->IsName(name)) return n;
   return (unsigned) -1;
}

std::string dabc::Module::UserItemName(unsigned indx, bool fullname) const
{
   if (indx>=fUsers.size()) return "";
   if (fullname) return fUsers[indx]->ItemName();
   return fUsers[indx]->GetName();
}

void dabc::Module::ProduceUserItemEvent(unsigned indx, unsigned cnt)
{
   while (IsValidUserItem(indx) && cnt--)
      FireEvent(evntUser, fUsers[indx]->ItemId());
}

bool dabc::Module::Start()
{
   DOUT3("Start module %s thrd %s", GetName(), ThreadName().c_str());

   if (thread().IsItself()) return DoStart();

   return Execute("StartModule") == cmd_true;
}

bool dabc::Module::Stop()
{
   DOUT3("Stop module %s thrd %s done", GetName(), ThreadName().c_str());

   if (thread().IsItself()) return DoStop();

   return Execute("StopModule") == cmd_true;
}

dabc::Buffer dabc::Module::TakeDfltBuffer()
{
   if (fDfltPool.null()) fDfltPool = dabc::mgr.FindPool(xmlWorkPool);

   MemoryPool* pool = dynamic_cast<MemoryPool*> (fDfltPool());

   if (pool!=0) return pool->TakeBuffer();

   return dabc::Buffer();
}

bool dabc::Module::DisconnectPort(const std::string& portname)
{
   PortRef port = FindPort(portname);

   if (port.null()) return false;

   port()->Disconnect();

   // we should process event (disregard running state) to allow module react on such action
   ProcessItemEvent(port(), evntPortDisconnect);

   return true;
}


int dabc::Module::PreviewCommand(Command cmd)
{
   // this hook in command execution routine allows us to "preview"
   // command before it actually executed
   // if it is system command, just execute it immediately

   int cmd_res = cmd_ignore;

   DOUT3("Module:%s PreviewCommand %s", GetName(), cmd.GetName());

   if (cmd.IsName("SetQueue")) {
      PortRef port = FindPort(cmd.GetStr("Port"));
      Reference q = cmd.GetRef("Queue");
      if (port.null()) {
         EOUT("Wrong port id when assigning queue");
         cmd_res = cmd_false;
      } else {
         port()->SetQueue(q);
         cmd_res = cmd_true;
      }
   } else
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
      for (unsigned n=0;n<NumInputs();n++)
         if (!Input(n)->IsConnected()) cmd_res = cmd_false;
      for (unsigned n=0;n<NumOutputs();n++)
         if (!Output(n)->IsConnected()) cmd_res = cmd_false;
   } else
   if (cmd.IsName("IsInputConnect")) {
      unsigned ninp = cmd.GetUInt("Number");
      cmd_res = cmd_bool((ninp<NumInputs()) && Input(ninp)->IsConnected());
   } else
   if (cmd.IsName("IsOutputConnect")) {
      unsigned nout = cmd.GetUInt("Number");
      cmd_res = cmd_bool((nout<NumOutputs()) && Output(nout)->IsConnected());
   } else
   if (cmd.IsName("DisconnectPort")) {
      cmd_res = cmd_bool(DisconnectPort(cmd.GetStr("Port")));
   } else
   if (cmd.IsName("IsPortConnected")) {
      PortRef port = FindPort(cmd.GetStr("Port"));
      if (!port.null())
         cmd_res = cmd_bool(port()->IsConnected());
      else
         cmd_res = cmd_false;
   } else
   if (cmd.IsName("GetSignallingKind")) {
      PortRef port = FindPort(cmd.GetStr("Port"));
      if (!port.null()) {
         cmd_res = cmd_true;
         cmd.SetInt("Kind", port()->SignallingKind());
      } else
         cmd_res = cmd_false;
   } else

   if (cmd.IsName("GetPoolHandle")) {
      unsigned cnt = cmd.GetUInt("Number");

      // in any case command returns true, but reference set only for the pools
      cmd_res = cmd_true;

      for (unsigned indx=0;indx<fPools.size();indx++) {
         PoolHandle* pool = fPools[indx];
         if (pool->QueueCapacity()==0) continue;

         if (cnt>0) { cnt--; continue; }

         cmd.SetRef("Port", PortRef(pool));
         cmd.SetRef("Pool", pool->fPool);
         break;
      }
   } else
   if (cmd.IsName("GetNumInputs")) {
      cmd.SetUInt("Number", NumInputs());
      cmd_res = cmd_true;
   } else
   if (cmd.IsName("GetNumOutputs")) {
      cmd.SetUInt("Number", NumOutputs());
      cmd_res = cmd_true;
   } else
   if (cmd.IsName("GetInputName")) {
      unsigned id = cmd.GetUInt("Id");
      bool asitem = cmd.GetBool("AsItem");
      if (id < NumInputs()) {
         cmd.SetStr("Name", asitem ? Input(id)->ItemName() : std::string(Input(id)->GetName()));
         cmd_res = cmd_true;
      } else {
         cmd_res = cmd_false;
      }
   } else
   if (cmd.IsName("GetOutputName")) {
      unsigned id = cmd.GetUInt("Id");
      bool asitem = cmd.GetBool("AsItem");
      if (id < NumOutputs()) {
         cmd.SetStr("Name", asitem ? Output(id)->ItemName() : std::string(Output(id)->GetName()));
         cmd_res = cmd_true;
      } else {
         cmd_res = cmd_false;
      }
   } else
   if (cmd.IsName("MakeConnReq")) {
      dabc::PortRef port = FindPort(cmd.GetStr("Port"));

      if (!port.null()) {
         dabc::ConnectionRequest req = port()->GetConnReq(true);

         req.SetInitState();

         req.SetRemoteUrl(cmd.GetStr("Url"));
         req.SetServerSide(cmd.GetBool("IsServer"));

         std::string thrdname = port.Cfg(xmlThreadAttr).AsStr();
         if (thrdname.empty())
            switch (dabc::mgr.GetThreadsLayout()) {
               case dabc::layoutMinimalistic: thrdname = dabc::mgr.ThreadName(); break;
               case dabc::layoutPerModule: thrdname = ThreadName(); break;
               case dabc::layoutBalanced: thrdname = ThreadName() + (port.IsInput() ? "Inp" : "Out"); break;
               case dabc::layoutMaximal: thrdname = ThreadName() + port.GetName(); break;
               default: thrdname = ThreadName(); break;
            }
         req.SetConnThread(thrdname);

         req.SetUseAckn(port.Cfg(xmlUseacknAttr).AsBool(false));

         req.SetOptional(port.Cfg(xmlOptionalAttr).AsBool(false));

         req.SetConnDevice(port.Cfg(xmlDeviceAttr).AsStr());

         req.SetConnTimeout(port.Cfg(xmlTimeoutAttr).AsDouble(10.));

         cmd.SetRef("ConnReq", req);

         cmd_res = cmd_true;
      } else {
         cmd_res = cmd_false;
      }
   } else
      cmd_res = Worker::PreviewCommand(cmd);

   if (cmd_res!=cmd_ignore)
      DOUT3("Module:%s PreviewCommand %s res=%d", GetName(), cmd.GetName(), cmd_res);

   return cmd_res;
}


bool dabc::Module::Find(ConfigIO &cfg)
{
   DOUT4("Module::Find %p name = %s parent %p", this, GetName(), GetParent());

   if (GetParent()==0) return false;

   // module will always have tag "Module", class could be specified with attribute
   while (cfg.FindItem(xmlModuleNode)) {
      if (cfg.CheckAttr(xmlNameAttr, GetName())) return true;
   }

   return false;
}

void dabc::Module::BuildFieldsMap(RecordFieldsMap* cont)
{
   cont->Field(xmlNumInputs).SetInt(NumInputs());
   cont->Field(xmlNumOutputs).SetInt(NumOutputs());
}

void dabc::Module::ObjectCleanup()
{
   if (IsRunning()) DoStop();

   ModuleCleanup();

   DOUT3("Module cleanup %s numchilds %u", GetName(), NumChilds());

   for (unsigned n=0;n<fItems.size();n++)
      if (fItems[n]) fItems[n]->DoCleanup();

   fDfltPool.Release();

   fSysTimerIndex = -1;

   dabc::Worker::ObjectCleanup();
}

double dabc::Module::ProcessTimeout(double last_diff)
{
   if (fSysTimerIndex < fTimers.size())
      return fTimers[fSysTimerIndex]->ProcessTimeout(last_diff);

   return -1.;
}


bool dabc::Module::DoStart()
{
   if (IsRunning()) return true;

   DOUT3("dabc::Module::DoStart() %s", GetName());

   BeforeModuleStart();

   fRunState = true;

   for (unsigned n=0;n<fItems.size();n++)
      if (fItems[n]) fItems[n]->DoStart();

   // treat special case of sys timer here - enable timeout of module itself
   if ((fSysTimerIndex < fTimers.size()) && (fTimers[fSysTimerIndex]->fPeriod>0))
      ActivateTimeout(fTimers[fSysTimerIndex]->fPeriod);


   DOUT3("dabc::Module::DoStart() %s done", GetName());

   return true;
}

bool dabc::Module::DoStop()
{
   DOUT3("dabc::Module::DoStop() %s", GetName());

   if (!IsRunning()) return true;

   // treat special case of sys timer here - disable timeout of module
   if (fSysTimerIndex < fTimers.size()) ActivateTimeout(-1);

   for (unsigned n=0;n<fItems.size();n++)
      if (fItems[n]) fItems[n]->DoStop();

   fRunState = false;

   AfterModuleStop();

   DOUT3("dabc::Module::DoStop() %s done", GetName());

   return true;
}

unsigned dabc::Module::CreatePoolHandle(const std::string& poolname, unsigned queue)
{
   unsigned index = FindPool(poolname);
   if (IsValidPool(index)) return index;

   dabc::MemoryPoolRef pool = dabc::mgr.FindPool(poolname);

   if (pool.null()) {
      EOUT("Pool %s not exists - cannot connect to module %s", poolname.c_str(), GetName());
      return (unsigned) -1;
   }

   PoolHandle* handle = new PoolHandle(this, pool, poolname, queue);

   AddModuleItem(handle);

   handle->SetItemSubId(fPools.size());
   fPools.push_back(handle);

   return handle->ItemSubId();
}

void dabc::Module::AddModuleItem(ModuleItem* item)
{
   // at that place one cannot use any dynamic_cast to inherited types,
   // while constructor of item is not completely finished

   unsigned id = fItems.size();

   fItems.push_back(item);

   item->SetItemId(id);

   if (id>moduleitemMaxId) {
      EOUT("Item id is too big, event propagation will not work");
      exit(104);
   }

   if (HasThread() && item->ItemNeedThread())
      item->AssignToThread(thread(), true);

//   DOUT0("Module:%s Add item:%s Id:%d", GetName(), item->GetName(), id);
}

void dabc::Module::RemoveModuleItem(ModuleItem* item)
{
   unsigned id = item->ItemId();

   for (unsigned n=0;n<fInputs.size();n++) {
      if (fInputs[n] == item) {
         fInputs.erase(fInputs.begin()+n);
      } else {
         fInputs[n]->SetItemSubId(n);
      }
   }

   for (unsigned n=0;n<fOutputs.size();n++) {
      if (fOutputs[n] == item) {
         fOutputs.erase(fOutputs.begin()+n);
      } else {
         fOutputs[n]->SetItemSubId(n);
      }
   }

   for (unsigned n=0;n<fPools.size();n++) {
      if (fPools[n] == item) {
         fPools.erase(fPools.begin()+n);
      } else {
         fPools[n]->SetItemSubId(n);
      }
   }

   for (unsigned n=0;n<fTimers.size();n++) {
      if (fTimers[n] == item) {
         fTimers.erase(fTimers.begin()+n);
      } else {
         fTimers[n]->SetItemSubId(n);
      }
   }

   for (unsigned n=0;n<fUsers.size();n++) {
      if (fUsers[n] == item) {
         fUsers.erase(fUsers.begin()+n);
      } else {
         fUsers[n]->SetItemSubId(n);
      }
   }

   fItems[id] = 0;
}


dabc::PortRef dabc::Module::FindPort(const std::string& name) const
{
   return FindChildRef(name.c_str());
}

unsigned dabc::Module::CreateInput(const std::string& name, unsigned queue)
{
   unsigned indx = FindInput(name);
   if (IsValidInput(indx)) return indx;

   if (queue == 0) return (unsigned) -1;

   InputPort* port = new InputPort(this, name, queue);

   AddModuleItem(port);

   port->SetItemSubId(fInputs.size());
   fInputs.push_back(port);

   if (!port->fRateName.empty() && port->fRate.null()) {
      if (Par(port->fRateName).null())
         CreatePar(port->fRateName).SetRatemeter(false, 3.).SetUnits("MB");
      port->SetRateMeter(Par(port->fRateName));
   }

   return port->ItemSubId();
}

unsigned dabc::Module::CreateOutput(const std::string& name, unsigned queue)
{
   unsigned indx = FindOutput(name);
   if (IsValidOutput(indx)) return indx;

   if (queue == 0) return (unsigned) -1;

   OutputPort* port = new OutputPort(this, name, queue);

   AddModuleItem(port);

   port->SetItemSubId(fOutputs.size());
   fOutputs.push_back(port);

   if (!port->fRateName.empty() && port->fRate.null()) {
      if (Par(port->fRateName).null())
         CreatePar(port->fRateName).SetRatemeter(false, 3.).SetUnits("MB");
      port->SetRateMeter(Par(port->fRateName));
   }

   return port->ItemSubId();
}

bool dabc::Module::BindPorts(const std::string& inpname, const std::string& outname)
{
   unsigned inpindx = FindInput(inpname);
   unsigned outindx = FindOutput(outname);

   if (IsValidInput(inpindx) && IsValidOutput(outindx)) {
      fInputs[inpindx]->SetBindName(outname);
      fOutputs[outindx]->SetBindName(inpname);
      return true;
   }

   return false;
}

void dabc::Module::ProduceInputEvent(unsigned indx, unsigned cnt)
{
   // TODO: should we produce such event automatically ???

   while (IsValidInput(indx) && cnt--)
      FireEvent(evntInput, fInputs[indx]->ItemId());
}

void dabc::Module::ProducePoolEvent(unsigned indx, unsigned cnt)
{
   while (IsValidPool(indx) && cnt--)
      FireEvent(evntInput, fPools[indx]->ItemId());
}


void dabc::Module::ProduceOutputEvent(unsigned indx, unsigned cnt)
{
   while (IsValidOutput(indx) && cnt--)
      FireEvent(evntOutput, fOutputs[indx]->ItemId());
}

bool dabc::Module::IsPortConnected(const std::string& name) const
{
   return FindPort(name).IsConnected();
}


bool dabc::Module::SetPortSignalling(const std::string& name, Port::EventsProducing signal)
{
   PortRef port = FindPort(name);
   if (!port.null()) {
      port()->SetSignalling(signal);
      return true;
   }

   return false;
}

bool dabc::Module::SetPortRatemeter(const std::string& name, const Parameter& ref)
{
   PortRef port = FindPort(name);
   if (!port.null()) {
      port()->SetRateMeter(ref);
      return true;
   }
   return true;
}

bool dabc::Module::SetPortLoopLength(const std::string& name, unsigned cnt)
{
   PortRef port = FindPort(name);
   if (!port.null()) {
      port()->SetMaxLoopLength(cnt);
      return true;
   }
   return true;
}

void dabc::Module::ProcessEvent(const EventId& evid)
{
   switch (evid.GetCode()) {
      case evntInput:
      case evntOutput:
      case evntInputReinj:
      case evntOutputReinj:
      case evntTimeout:
         if (IsRunning())
            ProcessItemEvent(GetItem(evid.GetArg()), evid.GetCode());
         break;
      case evntPortConnect: {
         // deliver event to the user disregard running state

         Port* port = dynamic_cast<Port*> (GetItem(evid.GetArg()));

         if (port) port->GetConnReq().ChangeState(ConnectionObject::sConnected, true);

         ProcessItemEvent(GetItem(evid.GetArg()), evid.GetCode());

         break;
      }

      case evntPortDisconnect: {

         Port* port = dynamic_cast<Port*> (GetItem(evid.GetArg()));

         if (port) port->GetConnReq().ChangeState(ConnectionObject::sDisconnected, true);

         DOUT2("Module %s running %s get disconnect event for port %s connected %s", GetName(), DBOOL(IsRunning()), port->ItemName().c_str(), DBOOL(port->IsConnected()));

         //InputPort* inp = dynamic_cast<InputPort*> (port);
         //if (inp) DOUT0("Input still can recv %u buffers", inp->NumCanRecv());

         // deliver event to the user disregard running state
         ProcessItemEvent(GetItem(evid.GetArg()), evid.GetCode());

         // if reconnect is specified and port is not declared as non-automatic
         if (port->GetReconnectPeriod() > 0) {
            std::string timername = dabc::format("ConnTimer_%s", port->GetName());

            ConnTimer* timer = dynamic_cast<ConnTimer*> (FindChild(timername.c_str()));

            if (timer==0) {
               timer = new ConnTimer(this, timername, port->GetName());
               AddModuleItem(timer);
            }
            port->SetDoingReconnect(true);
            timer->Activate(port->GetReconnectPeriod());

            DOUT0("Module %s will try to reconnect port %s with period %f", GetName(), port->ItemName().c_str(), port->GetReconnectPeriod());

            return;
         }

         if (fAutoStop && IsRunning()) {
            for (unsigned n=0;n<NumOutputs();n++)
               if (Output(n)->IsConnected() || Output(n)->IsDoingReconnect()) return;

            for (unsigned n=0;n<NumInputs();n++)
               if (Input(n)->IsConnected() || Input(n)->IsDoingReconnect()) return;

            DOUT0("Module %s automatically stopped while all connections are now disconnected", GetName());
            DoStop();
            // dabc::mgr.app().CheckWorkDone();
         }

         break;
      }
      case evntConnStart:
      case evntConnStop: {
         Port* port = dynamic_cast<Port*> (GetItem(evid.GetArg()));

         ProcessConnectionActivated(port->GetName(), evid.GetCode() == evntConnStart);

         break;
      }

      default:
         dabc::Worker::ProcessEvent(evid);
         break;
   }
}

bool dabc::Module::CanSendToAllOutputs(bool exclude_disconnected) const
{
   for(unsigned n=0;n<NumOutputs();n++) {
      OutputPort* out = Output(n);
      if (exclude_disconnected && !out->IsConnected()) continue;
      if (!out->CanSend()) return false;
   }
   return true;
}

void dabc::Module::SendToAllOutputs(Buffer& buf)
{
   if (buf.null()) return;

   unsigned last_can_send = NumOutputs();
   for(unsigned n=0;n<NumOutputs();n++) {
      OutputPort* out = Output(n);
      out->fSendallFlag = out->CanSend();
      if (out->fSendallFlag) last_can_send = n;
   }

   for(unsigned n=0;n<NumOutputs();n++) {
      OutputPort* out = Output(n);
      if (!out->fSendallFlag) continue;

      if (n==last_can_send) {
         Output(n)->Send(buf);
         if (!buf.null()) { EOUT("buffer not null after sending to output %u", n); exit(333); }
      } else {
         dabc::Buffer dupl = buf.Duplicate();
         Output(n)->Send(dupl);
         if (!dupl.null()) { EOUT("buffer not null after sending to output %u", n); exit(333); }
      }
   }

   if ((last_can_send != NumOutputs()) && !buf.null()) {
      EOUT("Should never happens buf %u!!!", buf.GetTotalSize());
      exit(333);
   }

   buf.Release();
}

double dabc::Module::ProcessConnTimer(ConnTimer* timer)
{
   PortRef port = FindPort(timer->fPortName);
   if (port.null()) return -1.;

   if (!port()->IsDoingReconnect()) return -1;

   DOUT0("Trying to reconnect port %s", port.GetName());

   if (port.IsConnected() || dabc::mgr.CreateTransport(port.ItemName())) {
      port()->SetDoingReconnect(false);
      return -1.;
   }

   return port()->GetReconnectPeriod();
}


// ==========================================================================


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


bool dabc::ModuleRef::ConnectPoolHandles()
{
   if (GetObject()==0) return false;

   unsigned cnt(0);

   while (true) {
      dabc::Command cmd("GetPoolHandle");
      cmd.SetUInt("Number", cnt++);

      if (Execute(cmd) != cmd_true) return false;

      PortRef portinp = cmd.GetRef("Port");
      MemoryPoolRef poolref = cmd.GetRef("Pool");

      if (portinp.null()) break;

      if (poolref.null()) {
         EOUT("Something went wrong with connection to the pools");
         exit(543);
      }

      DOUT3("@@@@@ Create requester for item %s", portinp.ItemName().c_str());

      PortRef portout = poolref.CreateNewRequester();

      dabc::LocalTransport::ConnectPorts(portout, portinp);
   }

   return true;
}

dabc::PortRef dabc::ModuleRef::FindPort(const std::string& name)
{
   return FindChild(name.c_str());
}

bool dabc::ModuleRef::IsPortConnected(const std::string& name)
{
   dabc::Command cmd("IsPortConnected");
   cmd.SetStr("Port", name);
   return Execute(cmd) == cmd_true;
}

unsigned dabc::ModuleRef::NumInputs()
{
   dabc::Command cmd("GetNumInputs");
   return (Execute(cmd)==cmd_true) ? cmd.GetUInt("Number") : 0;
}

unsigned dabc::ModuleRef::NumOutputs()
{
   dabc::Command cmd("GetNumOutputs");
   return (Execute(cmd)==cmd_true) ? cmd.GetUInt("Number") : 0;
}


std::string dabc::ModuleRef::InputName(unsigned n, bool itemname)
{
   dabc::Command cmd("GetInputName");
   cmd.SetUInt("Id", n);
   cmd.SetBool("AsItem", itemname);

   if (Execute(cmd)==cmd_true) return cmd.GetStr("Name");
   return std::string();
}

std::string dabc::ModuleRef::OutputName(unsigned n, bool itemname)
{
   dabc::Command cmd("GetOutputName");
   cmd.SetUInt("Id", n);
   cmd.SetBool("AsItem", itemname);

   if (Execute(cmd)==cmd_true) return cmd.GetStr("Name");
   return std::string();
}

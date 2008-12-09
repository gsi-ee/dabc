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
#include "dabc/CommandDefinition.h"
#include "dabc/Application.h"

// __________________________________________________________________

dabc::Module::Module(const char* name, Command* cmd) :
   Folder(dabc::mgr(), (name ? name : (cmd ? cmd->GetStr("Name","module") : "module")), true),
   WorkingProcessor(),
   CommandClientBase(),
   fRunState(msStopped),
   fInputPorts(),
   fOutputPorts(),
   fPorts(),
   fReplyes(true, true),
   fLostEvents(128, true)
{
   SetParsHolder(this);

   // we will use 3 priority levels:
   //  0 - normal for i/o ,
   //  1 - for commands and replies,
   //  2 - for sys commands (in modules thread itself)

   CommandDefinition* def = NewCmdDef("SetPriority");
   def->AddArgument("Priority", argInt, true);
   def->Register(true);

   def = NewCmdDef(CmdSetParameter::CmdName());
   def->AddArgument("ParName", argString, true);
   def->AddArgument("ParValue", argString, true);
   def->Register(true);
}

dabc::Module::~Module()
{
   DOUT3(( "dabc::Module::~Module() %s thrd = %s mgr = %p",
             GetName(), DNAME(ProcessorThread()), dabc::mgr()));

   fRunState = msHalted;

   // by this we stop any further events processing and can be sure that
   // destroying module items will not cause seg.fault
   RemoveProcessorFromThread(true);

   // one should call destructor of children here, while
   // all ModuleItem objects will try to access Module method ItemDestroyed
   // If it happens in Basic destructor, object as it is no longer exists

   DOUT5(("Module:%s deletes all childs", GetName()));

   DestroyAllPars();

   DeleteChilds();

   DOUT5((" dabc::Module::~Module() %s done", GetName()));
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

dabc::RateParameter* dabc::Module::CreateRateParameter(const char* name, bool sync, double interval, const char* inpportname, const char* outportname)
{
   RateParameter* par = new RateParameter(this, name, sync, interval);

   bool isanyport = false;

   Port* port = FindPort(inpportname);
   if (port) {
      port->SetInpRateMeter(par);
      isanyport = true;
   }

   port = FindPort(outportname);
   if (port) {
       port->SetOutRateMeter(par);
       isanyport = true;
   }

   if (isanyport) par->SetUnits("MB/s");

   return par;
}

dabc::Parameter* dabc::Module::CreatePoolUsageParameter(const char* name, double interval, const char* poolname)
{
   Parameter* par = CreateParDouble(name, 0.);

   PoolHandle* pool = FindPool(poolname);
   if (pool) pool->SetUsageParameter(par, interval);

   return par;
}

dabc::CommandDefinition* dabc::Module::NewCmdDef(const char* cmdname)
{
   if (cmdname==0) return 0;

   return new dabc::CommandDefinition(GetCmdDefFolder(true), cmdname);
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
   if (cmd->IsName("SetPriority")) {
      if (ProcessorThread()) {
         ProcessorThread()->SetPriority(cmd->GetInt("Priority",0));
         cmd_res = cmd_true;
      } else
         cmd_res = cmd_false;
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
   if (!IsRunning()) return false;

   for (unsigned n=0;n<fItems.size();n++) {
      ModuleItem* item = (ModuleItem*) fItems.at(n);
      if (item) item->DoStop();
   }

   fRunState = msStopped;

   AfterModuleStop();

   return true;
}

int dabc::Module::ExecuteCommand(Command* cmd)
{
   // return false if command cannot be processed

   return CommandReceiver::ExecuteCommand(cmd);
}

dabc::PoolHandle* dabc::Module::CreatePool(const std::string &name, BufferNum_t number, BufferSize_t size, BufferNum_t increment)
{
   return new dabc::PoolHandle(this, name.c_str(), number, increment, size);
}

dabc::Folder* dabc::Module::GetObjFolder(bool force)
{
   return GetFolder("Objects", force, true);
}

dabc::Folder* dabc::Module::GetCmdDefFolder(bool force)
{
   return GetFolder("Commands", force, true);
}

dabc::Folder* dabc::Module::GetTimersFolder(bool force)
{
   return GetFolder("Timers", force, true);
}

bool dabc::Module::_ProcessReply(Command* cmd)
{
   // here one get completion from command
   // one should redirect it to the signal

   if (cmd==0) return false;
   fReplyes.Push(cmd);
   FireEvent(evntReplyCommand);
   return true;
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
      exit(1);
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

dabc::Port* dabc::Module::GetPortItem(unsigned id) const
{
   ModuleItem* item = GetItem(id);
   return item && (item->GetType()==mitPort) ? (Port*) item : 0;
}

dabc::Port* dabc::Module::CreatePort(const char* name, PoolHandle* pool, unsigned recvqueue, unsigned sendqueue, BufferSize_t headersize)
{
   if ((recvqueue==0) && (sendqueue==0)) {
      EOUT(("Both receive and send queue length are zero - port %s not created", name));
      return 0;
   }

   Port* port = new Port(this, name, pool, recvqueue, sendqueue, headersize);
   if (pool)
     pool->AddPortRequirements(port->NumInputBuffersRequired() + port->NumOutputBuffersRequired(), port->UserHeaderSize());

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
      fLostEvents.Push(CodeEvent(evid, item->ItemId()));
}


void dabc::Module::ProcessEvent(EventId evid)
{
   switch (dabc::GetEventCode(evid)) {
      case evntReinjectlost:
         if (fLostEvents.Size()>0) {
            EventId user_evnt = fLostEvents.Pop();

            ModuleItem* item = GetItem(GetEventItem(user_evnt));

            if (item!=0)
               GetUserEvent(item, GetEventCode(user_evnt));
            else
               EOUT(("Module:%s Event %u for item %u lost completely",
                     GetName(), GetEventCode(user_evnt), GetEventItem(user_evnt)));
         }

         if (fLostEvents.Size() > 0) FireEvent(evntReinjectlost);
         break;

      case evntReplyCommand: {
         dabc::Command* cmd = fReplyes.Pop();
         if (ReplyCommand(cmd))
            dabc::Command::Finalise(cmd);
         break;
      }

      default:
         dabc::WorkingProcessor::ProcessEvent(evid);
   }
}

dabc::Buffer* dabc::Module::TakeBuffer(const char* poolname, BufferSize_t size)
{
   PoolHandle* pool = FindPool(poolname);

   return pool ? pool->TakeBuffer(size, false) : 0;
}



bool dabc::Module::IsAnyOutputBlocked() const
{
   for(unsigned n=0;n<NumOutputs();n++)
      if (Output(n)->OutputBlocked()) return true;
   return false;
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

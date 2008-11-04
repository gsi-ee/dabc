#include "bnet/WorkerApplication.h"

#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"

dabc::String bnet::WorkerApplication::fPluginName = "";


const char* bnet::WorkerApplication::ItemName()
{
   if (fPluginName.length()==0) {
      fPluginName = dabc::Manager::PluginFolderName();
      fPluginName += "/";
      fPluginName += PluginName();
   }
   return fPluginName.c_str();
}


bnet::WorkerApplication::WorkerApplication(dabc::Basic* parent, const char* name) :
   dabc::Application(parent, name)
{
   new dabc::IntParameter(this, "IsGenerator", 1);
   new dabc::IntParameter(this, "IsSender", 0);
   new dabc::IntParameter(this, "IsReceiver", 0);
   new dabc::IntParameter(this, "IsFilter", 0);
   new dabc::IntParameter(this, "CombinerModus", 0);
   new dabc::IntParameter(this, "NumReadouts", 1);
   for (int nr=0;nr<5;nr++)
      new dabc::StrParameter(this, FORMAT(("Input%dCfg", nr)), "");
   new dabc::StrParameter(this, "StoragePar", "");

   new dabc::StrParameter(this, "Thread1Name", "Thread1");
   new dabc::StrParameter(this, "Thread2Name", "Thread2");
   new dabc::StrParameter(this, "ThreadCtrlName", "ThreadCtrl");

   new dabc::IntParameter(this, "CombinerInQueueSize", 4);
   new dabc::IntParameter(this, "CombinerOutQueueSize", 4);

   new dabc::IntParameter(this, "ReadoutBuffer",         2*1024);
   new dabc::IntParameter(this, "ReadoutPoolSize",   4*0x100000);
   new dabc::IntParameter(this, "TransportBuffer",       8*1024);
   new dabc::IntParameter(this, "TransportPoolSize",16*0x100000);
   new dabc::IntParameter(this, "EventBuffer",          32*1024);
   new dabc::IntParameter(this, "EventPoolSize",     4*0x100000);
   new dabc::IntParameter(this, "CtrlBuffer",            2*1024);
   new dabc::IntParameter(this, "CtrlPoolSize",      2*0x100000);

   new dabc::IntParameter(this, "CfgNumNodes", 1, false);
   new dabc::IntParameter(this, "CfgNodeId", 0, false);
   new dabc::IntParameter(this, "CfgController", 0, false);
   new dabc::StrParameter(this, "CfgSendMask", "", false);
   new dabc::StrParameter(this, "CfgRecvMask", "", false);
   new dabc::StrParameter(this, "CfgClusterMgr", "", false);
   new dabc::StrParameter(this, "CfgNetDevice", "", false);
   new dabc::IntParameter(this, "CfgConnected", 0, false);
   new dabc::IntParameter(this, "CfgEventsCombine", 1, false);

   new dabc::StrParameter(this, "Status", "Off");
   new dabc::StrParameter(this, "SendStatus", "oooo");
   new dabc::StrParameter(this, "RecvStatus", "oooo");

   DOUT1(("!!!! Wroker plugin created !!!!"));
}

const char* bnet::WorkerApplication::ReadoutPar(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumReadouts())) return 0;
   return GetParCharStar(FORMAT(("Input%dCfg", nreadout)));
}

dabc::Module* bnet::WorkerApplication::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{
    if ((classname==0) || (cmd==0)) return 0;

    DOUT4(( "Create module:%s name:%s", classname, modulename));

    if (strcmp(classname,"SenderModule")==0)
       return new bnet::SenderModule(GetManager(), modulename, this);
    else
    if (strcmp(classname,"ReceiverModule")==0)
       return new bnet::ReceiverModule(GetManager(), modulename, this);

    return 0;
}

bool bnet::WorkerApplication::CreateStorage(const char* portname)
{
   dabc::Command* cmd = new dabc::Command("NullConnect");
   cmd->SetStr("Port", portname);

   return GetManager()->Execute(cmd, 5);
}


void bnet::WorkerApplication::DiscoverNodeConfig(dabc::Command* cmd)
{
   SetParValue("CfgController", cmd->GetBool("WithController") ? 1 : 0);
   SetParValue("CfgEventsCombine", cmd->GetInt("EventsCombine", 1));
   SetParValue("CtrlBuffer", cmd->GetInt("ControlBuffer", 1024));
   SetParValue("CfgNetDevice", cmd->GetStr("NetDevice",""));

   int TransportBufferSize = cmd->GetInt("TransportBuffer", 1024);
   int ReadoutBufferSize = TransportBufferSize / NumReadouts();
   int EventBufferSize = TransportBufferSize * (CfgNumNodes() - 1);

   SetParValue("ReadoutBuffer", ReadoutBufferSize);
   SetParValue("TransportBuffer", TransportBufferSize);
   SetParValue("EventBuffer", EventBufferSize);

   cmd->SetBool("IsSender", IsSender());
   cmd->SetBool("IsReceiver", IsReceiver());

   if (IsSender()) {

      dabc::Module* m = GetManager()->FindModule("Sender");
      if (m) {
         dabc::String res = m->ExecuteStr("GetConfig", "RecvMask", 5);
         cmd->SetStr("RecvMask", res.c_str());
      }
   }

   if (IsReceiver()) {
      dabc::Module* m = GetManager()->FindModule("Receiver");
      if (m) {
         dabc::String res = m->ExecuteStr("GetConfig", "SendMask", 5);
         cmd->SetStr("SendMask", res.c_str());
      }
   }
}

void bnet::WorkerApplication::ApplyNodeConfig(dabc::Command* cmd)
{
//   LockUnlockPars(false);

   SetParValue("CfgController", cmd->GetInt("GlobalCtrl", 0));
   SetParValue("CfgSendMask", cmd->GetStr("SendMask", "xxxx"));
   SetParValue("CfgRecvMask", cmd->GetStr("RecvMask", "xxxx"));
   SetParValue("CfgClusterMgr", cmd->GetStr("ClusterMgr",""));

//   LockUnlockPars(true);

   dabc::CommandsSet* set = new dabc::CommandsSet(GetManager(), cmd, false);

   dabc::Module* m = 0;

   if (IsSender()) {

      m = GetManager()->FindModule("Sender");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetInt("Standalone", CfgController() ? 0 : 1);
         cmd->SetStr("RecvMask", CfgRecvMask());

         set->Add(GetManager()->LocalCmd(cmd, m));
//         m->Submit(*set, cmd);
      } else
         EOUT(("Cannot find Sender module"));
   }

   if (IsReceiver()) {
      m = GetManager()->FindModule("Receiver");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetStr("SendMask", CfgSendMask());
         set->Add(GetManager()->LocalCmd(cmd, m));
//         m->Submit(*set, cmd);
      } else
         EOUT(("Cannot find receiever module"));

      m = GetManager()->FindModule("Builder");
      if (m) {
         dabc::Command* cmd = new dabc::CommandSetParameter("SendMask", CfgSendMask());
         set->Add(GetManager()->LocalCmd(cmd, m));

//         m->Submit(*set, new dabc::CommandSetParameter("SendMask", CfgSendMask()));
      } else
         EOUT(("Cannot find builder module"));
   }

   set->Add(GetManager()->LocalCmd(new dabc::CommandSetParameter("CfgConnected", 1), this));

   dabc::CommandsSet::Completed(set, SMCommandTimeout());

   DOUT3(("ApplyNodeConfig invoked"));
}

int bnet::WorkerApplication::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_true;

   if (cmd->IsName("DiscoverWorkerConfig")) {
      DiscoverNodeConfig(cmd);
   } else
   if (cmd->IsName("ApplyConfigNode")) {
      DOUT3(( "Get reconfigure recvmask = %s", cmd->GetStr("RecvMask")));
      ApplyNodeConfig(cmd);
      cmd_res = cmd_postponed;
   } else
   if (cmd->IsName("CheckModulesStatus")) {
      cmd_res = CheckWorkerModules();
   } else
      cmd_res = dabc::Application::ExecuteCommand(cmd);

   return cmd_res;
}

bool bnet::WorkerApplication::CheckWorkerModules()
{
   bool isstopped = true;

   if (IsSender()) {

//      if (GetManager()->IsModuleRunning("Combiner")) isstopped = false;
      if (GetManager()->IsModuleRunning("Sender")) isstopped = false;
   }

   if (IsReceiver()) {
      if (GetManager()->IsModuleRunning("Receiver")) isstopped = false;
//      if (GetManager()->IsModuleRunning("Builder")) isstopped = false;

//      if (IsFilter())
//         if (GetManager()->IsModuleRunning("Filter")) isstopped = false;
   }

   if (isstopped) {
      if (strcmp(GetManager()->CurrentState(), dabc::Manager::stReady) != 0) {
         DOUT1(("!!!!! ******** !!!!!!!!  All main modules are stopped - we can switch to Stop state"));
         GetManager()->InvokeStateTransition(dabc::Manager::stcmdDoStop);
      }
   }

   return true;
}

void bnet::WorkerApplication::SetPars(bool is_all_to_all,
                                 int numreadouts,
                                 int combinermodus)
{

   int nodeid = GetManager()->NodeId();
   int numnodes = GetManager()->NumNodes();

   if ((nodeid<0) || (nodeid>=numnodes)) return;

   const char* thrd1name = "Thread1";
   const char* thrd2name = "Thread2";
   const char* thrdctrl = "ThrdContrl";

   int numsrcnodes(1), numbldnodes(1);

   if (is_all_to_all) {
      numsrcnodes = numnodes - 1;
      numbldnodes = numsrcnodes;
   } else {
      numsrcnodes = (numnodes - 1) / 2;
      numbldnodes = numnodes - 1 - numsrcnodes;
   }

   // first, set common parameters
   bool issender(false), isreceiver(false);

   if (is_all_to_all) {
      issender = true;
      isreceiver = true;
   } else {
      issender = (nodeid <= numsrcnodes);
      isreceiver = !issender;
   }

#define SETPAR(name, value) SetParValue(name, value);

      SETPAR("CfgNodeId", nodeid)
      SETPAR("CfgNumNodes", numnodes)
      SETPAR("IsGenerator", 1)
      SETPAR("IsSender", issender ? 1 : 0)
      SETPAR("IsReceiver", isreceiver ? 1 : 0)
      SETPAR("IsFilter", isreceiver ? 1 : 0)
      SETPAR("CombinerModus", combinermodus)
      SETPAR("NumReadouts", numreadouts)

      SETPAR("Thread1Name", thrd1name)
      SETPAR("Thread2Name", thrd2name)
      SETPAR("ThreadCtrlName", thrdctrl)
      SETPAR("NetDevice", bnet::NetDevice)
      SETPAR("ReadoutBuffer",           8*1024)
      SETPAR("ReadoutPoolSize",     20*0x100000)
      SETPAR("TransportBuffer",        32*1024)
      SETPAR("TransportPoolSize",  16*0x100000)
      SETPAR("EventBuffer",           128*1024)
      SETPAR("EventPoolSize",      16*0x100000)
      SETPAR("CtrlBuffer",              4*1024)
      SETPAR("CtrlPoolSize",       32*0x100000)

#undef SETPAR
}

bool bnet::WorkerApplication::CreateAppModules()
{
//   LockUnlockPars(true);

   DOUT1(("CreateAppModules starts dev = %s", CfgNetDevice()));

   GetManager()->CreateDevice(CfgNetDevice(), "BnetDev");

   if (IsSender() && (CombinerModus()==0)) {
      GetManager()->CreateMemoryPool(ReadoutPoolName(), ReadoutBufferSize(), ReadoutPoolSize()/ReadoutBufferSize());
      GetManager()->ConfigurePool(ReadoutPoolName(), true);
   }

   GetManager()->CreateMemoryPool(TransportPoolName(), TransportBufferSize(), TransportPoolSize()/TransportBufferSize(), 0, sizeof(bnet::SubEventNetHeader));
   GetManager()->ConfigurePool(TransportPoolName(), true);

   if (IsReceiver()) {
      GetManager()->CreateMemoryPool(EventPoolName(), EventBufferSize(), EventPoolSize()/EventBufferSize());
      GetManager()->ConfigurePool(EventPoolName(), true);
   }

   if (IsSender()) {
      GetManager()->CreateMemoryPool(ControlPoolName(), ControlBufferSize(), ControlPoolSize() / ControlBufferSize() );
      GetManager()->ConfigurePool(ControlPoolName(), true);
   }

   dabc::Module* m = 0;

   if (IsSender()) {

      m = CreateCombiner("Combiner");
      if (m==0) {
         EOUT(("Combiner module not created"));
         return false;
      }

      m = new bnet::SenderModule(GetManager(), "Sender", this);
      GetManager()->MakeThreadForModule(m, Thrd1Name());
   }

   if (IsReceiver()) {

      m = new bnet::ReceiverModule(GetManager(), "Receiver", this);
      GetManager()->MakeThreadForModule(m, Thrd2Name());

      m = CreateBuilder("Builder");
      if (m==0) {
         EOUT(("EventBuilder is not created"));
         return false;
      }

      if (IsFilter()) {
         m = CreateFilter("Filter");
         if (m==0) {
            EOUT(("EventFilter is not created"));
           return false;
         }
      }
   }

   GetManager()->CreateMemoryPools();

   if (IsSender()) {

      for (int nr=0;nr<NumReadouts();nr++)
        if (!CreateReadout(FORMAT(("Combiner/Ports/Input%d", nr)), nr)) {
           EOUT(("Cannot create readout channel %d", nr));
           return false;
        }

      GetManager()->ConnectPorts("Combiner/Ports/Output", "Sender/Ports/Input");
   }

   if (IsReceiver()) {
      GetManager()->ConnectPorts("Receiver/Ports/Output", "Builder/Ports/Input");

      const char* outportname = 0;

      if (IsFilter()) {
         GetManager()->ConnectPorts("Builder/Ports/Output", "Filter/Ports/Input");
         outportname = "Filter/Ports/Output";
      } else
         outportname = "Builder/Ports/Output";

      if (!CreateStorage(outportname)) {
         EOUT(("Not able to create storage for port %s", outportname));
      }
   }

//   SetParFixed("Status",  false);
   SetParValue("Status", "Ready");

//   SetParFixed("Status",  true);

   SetParValue("CfgConnected", 0);

   return true;
}

int bnet::WorkerApplication::IsAppModulesConnected()
{
   return GetParInt("CfgConnected")>0 ? 1 : 2;
}

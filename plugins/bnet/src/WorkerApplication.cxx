#include "bnet/WorkerApplication.h"

#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"

const char* bnet::WorkerApplication::ItemName()
{
   return PluginName();
}

bnet::WorkerApplication::WorkerApplication(const char* name) :
   dabc::Application(name ? name : PluginName())
{
   CreateParInt("IsGenerator", 1);
   CreateParInt("IsSender", 0);
   CreateParInt("IsReceiver", 0);
   CreateParInt("IsFilter", 0);
   CreateParInt("CombinerModus", 0);
   CreateParInt("NumReadouts", 1);
   for (int nr=0;nr<NumReadouts();nr++)
      CreateParStr(FORMAT(("Input%dCfg", nr)), "");
   CreateParStr("StoragePar", "");

   CreateParStr("Thread1Name", "Thread1");
   CreateParStr("Thread2Name", "Thread2");
   CreateParStr("ThreadCtrlName", "ThreadCtrl");

   CreateParInt("CombinerInQueueSize", 4);
   CreateParInt("CombinerOutQueueSize", 4);

   CreateParInt("ReadoutBuffer",         2*1024);
   CreateParInt("ReadoutPoolSize",   4*0x100000);
   CreateParInt("TransportBuffer",       8*1024);
   CreateParInt("TransportPoolSize",16*0x100000);
   CreateParInt("EventBuffer",          32*1024);
   CreateParInt("EventPoolSize",     4*0x100000);
   CreateParInt("CtrlBuffer",            2*1024);
   CreateParInt("CtrlPoolSize",      2*0x100000);

   CreateParStr("Status", "Off");
   CreateParStr("SendStatus", "oooo");
   CreateParStr("RecvStatus", "oooo");


   SetParDflts(0);  // make next parameters not visible outside

   CreateParInt("CfgNumNodes", 1);
   CreateParInt("CfgNodeId", 0);
   CreateParInt("CfgController", 0);
   CreateParStr("CfgSendMask", "");
   CreateParStr("CfgRecvMask", "");
   CreateParStr("CfgClusterMgr", "");
   CreateParStr("CfgNetDevice", "");
   CreateParInt("CfgConnected", 0);
   CreateParInt("CfgEventsCombine", 1);

   SetParDflts();

   DOUT1(("!!!! Worker plugin created !!!!"));
}

std::string bnet::WorkerApplication::ReadoutPar(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumReadouts())) return "";
   return GetParStr(FORMAT(("Input%dCfg", nreadout)));
}

dabc::Module* bnet::WorkerApplication::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{
    if ((classname==0) || (cmd==0)) return 0;

    DOUT4(( "Create module:%s name:%s", classname, modulename));

    if (strcmp(classname,"SenderModule")==0)
       return new bnet::SenderModule(modulename, this);
    else
    if (strcmp(classname,"ReceiverModule")==0)
       return new bnet::ReceiverModule(modulename, this);

    return 0;
}

bool bnet::WorkerApplication::CreateStorage(const char* portname)
{
   dabc::Command* cmd = new dabc::Command("NullConnect");
   cmd->SetStr("Port", portname);

   return dabc::mgr()->Execute(cmd, 5);
}


void bnet::WorkerApplication::DiscoverNodeConfig(dabc::Command* cmd)
{
   SetParInt("CfgController", cmd->GetBool("WithController") ? 1 : 0);
   SetParInt("CfgEventsCombine", cmd->GetInt("EventsCombine", 1));
   SetParInt("CtrlBuffer", cmd->GetInt("ControlBuffer", 1024));
   SetParStr("CfgNetDevice", cmd->GetStr("NetDevice",""));

   int TransportBufferSize = cmd->GetInt("TransportBuffer", 1024);
   int ReadoutBufferSize = TransportBufferSize / NumReadouts();
   int EventBufferSize = TransportBufferSize * (CfgNumNodes() - 1);

   SetParInt("ReadoutBuffer", ReadoutBufferSize);
   SetParInt("TransportBuffer", TransportBufferSize);
   SetParInt("EventBuffer", EventBufferSize);

   cmd->SetBool("IsSender", IsSender());
   cmd->SetBool("IsReceiver", IsReceiver());

   if (IsSender()) {

      dabc::Module* m = dabc::mgr()->FindModule("Sender");
      if (m) {
         std::string res = m->ExecuteStr("GetConfig", "RecvMask", 5);
         cmd->SetStr("RecvMask", res.c_str());
      }
   }

   if (IsReceiver()) {
      dabc::Module* m = dabc::mgr()->FindModule("Receiver");
      if (m) {
         std::string res = m->ExecuteStr("GetConfig", "SendMask", 5);
         cmd->SetStr("SendMask", res.c_str());
      }
   }
}

void bnet::WorkerApplication::ApplyNodeConfig(dabc::Command* cmd)
{
//   LockUnlockPars(false);

   SetParInt("CfgController", cmd->GetInt("GlobalCtrl", 0));
   SetParStr("CfgSendMask", cmd->GetStr("SendMask", "xxxx"));
   SetParStr("CfgRecvMask", cmd->GetStr("RecvMask", "xxxx"));
   SetParStr("CfgClusterMgr", cmd->GetStr("ClusterMgr",""));

//   LockUnlockPars(true);

   dabc::CommandsSet* set = new dabc::CommandsSet(cmd, false);

   dabc::Module* m = 0;

   if (IsSender()) {

      m = dabc::mgr()->FindModule("Sender");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetInt("Standalone", CfgController() ? 0 : 1);
         cmd->SetStr("RecvMask", CfgRecvMask());

         set->Add(dabc::mgr()->LocalCmd(cmd, m));
//         m->Submit(*set, cmd);
      } else
         EOUT(("Cannot find Sender module"));
   }

   if (IsReceiver()) {
      m = dabc::mgr()->FindModule("Receiver");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetStr("SendMask", CfgSendMask());
         set->Add(dabc::mgr()->LocalCmd(cmd, m));
//         m->Submit(*set, cmd);
      } else
         EOUT(("Cannot find receiever module"));

      m = dabc::mgr()->FindModule("Builder");
      if (m) {
         dabc::Command* cmd = new dabc::CommandSetParameter("SendMask", CfgSendMask().c_str());
         set->Add(dabc::mgr()->LocalCmd(cmd, m));

//         m->Submit(*set, new dabc::CommandSetParameter("SendMask", CfgSendMask()));
      } else
         EOUT(("Cannot find builder module"));
   }

   set->Add(dabc::mgr()->LocalCmd(new dabc::CommandSetParameter("CfgConnected", 1), this));

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
//   if (cmd->IsName("CheckModulesStatus")) {
//      cmd_res = CheckWorkerModules();
//   } else
      cmd_res = dabc::Application::ExecuteCommand(cmd);

   return cmd_res;
}

bool bnet::WorkerApplication::IsModulesRunning()
{
   if (IsSender())
      if (dabc::mgr()->IsModuleRunning("Sender")) return true;

   if (IsReceiver())
      if (dabc::mgr()->IsModuleRunning("Receiver")) return true;

   return false;
}

bool bnet::WorkerApplication::CheckWorkerModules()
{
   if (!IsModulesRunning()) {
      if (dabc::mgr()->CurrentState() != dabc::Manager::stReady) {
         DOUT1(("!!!!! ******** !!!!!!!!  All main modules are stopped - we can switch to Stop state"));
         dabc::mgr()->InvokeStateTransition(dabc::Manager::stcmdDoStop);
      }
   }

   return true;
}

void bnet::WorkerApplication::SetPars(bool is_all_to_all,
                                      int numreadouts,
                                      int combinermodus)
{

   int nodeid = dabc::mgr()->NodeId();
   int numnodes = dabc::mgr()->NumNodes();

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

   SetParInt("CfgNodeId", nodeid);
   SetParInt("CfgNumNodes", numnodes);
   SetParInt("IsGenerator", 1);
   SetParInt("IsSender", issender ? 1 : 0);
   SetParInt("IsReceiver", isreceiver ? 1 : 0);
   SetParInt("IsFilter", isreceiver ? 1 : 0);
   SetParInt("CombinerModus", combinermodus);
   SetParInt("NumReadouts", numreadouts);

   SetParStr("Thread1Name", thrd1name);
   SetParStr("Thread2Name", thrd2name);
   SetParStr("ThreadCtrlName", thrdctrl);
   SetParStr("NetDevice", bnet::NetDevice);
   SetParInt("ReadoutBuffer",           8*1024);
   SetParInt("ReadoutPoolSize",     20*0x100000);
   SetParInt("TransportBuffer",        32*1024);
   SetParInt("TransportPoolSize",  16*0x100000);
   SetParInt("EventBuffer",           128*1024);
   SetParInt("EventPoolSize",      16*0x100000);
   SetParInt("CtrlBuffer",              4*1024);
   SetParInt("CtrlPoolSize",       32*0x100000);
}

bool bnet::WorkerApplication::CreateAppModules()
{
//   LockUnlockPars(true);

   DOUT1(("CreateAppModules starts dev = %s", CfgNetDevice().c_str()));

   dabc::mgr()->CreateDevice(CfgNetDevice().c_str(), "BnetDev");

   if (IsSender() && (CombinerModus()==0)) {
      dabc::mgr()->CreateMemoryPool(ReadoutPoolName(), ReadoutBufferSize(), ReadoutPoolSize()/ReadoutBufferSize());
      dabc::mgr()->ConfigurePool(ReadoutPoolName(), true);
   }

   dabc::mgr()->CreateMemoryPool(TransportPoolName(), TransportBufferSize(), TransportPoolSize()/TransportBufferSize(), 0, sizeof(bnet::SubEventNetHeader));
   dabc::mgr()->ConfigurePool(TransportPoolName(), true);

   if (IsReceiver()) {
      dabc::mgr()->CreateMemoryPool(EventPoolName(), EventBufferSize(), EventPoolSize()/EventBufferSize());
      dabc::mgr()->ConfigurePool(EventPoolName(), true);
   }

   if (IsSender()) {
      dabc::mgr()->CreateMemoryPool(ControlPoolName(), ControlBufferSize(), ControlPoolSize() / ControlBufferSize() );
      dabc::mgr()->ConfigurePool(ControlPoolName(), true);
   }

   dabc::Module* m = 0;

   if (IsSender()) {

      m = CreateCombiner("Combiner");
      if (m==0) {
         EOUT(("Combiner module not created"));
         return false;
      }

      m = new bnet::SenderModule("Sender", this);
      dabc::mgr()->MakeThreadForModule(m, Thrd1Name().c_str());
   }

   if (IsReceiver()) {

      m = new bnet::ReceiverModule("Receiver", this);
      dabc::mgr()->MakeThreadForModule(m, Thrd2Name().c_str());

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

   dabc::mgr()->CreateMemoryPools();

   if (IsSender()) {

      for (int nr=0;nr<NumReadouts();nr++)
        if (!CreateReadout(FORMAT(("Combiner/Input%d", nr)), nr)) {
           EOUT(("Cannot create readout channel %d", nr));
           return false;
        }

      dabc::mgr()->ConnectPorts("Combiner/Output", "Sender/Input");
   }

   if (IsReceiver()) {
      dabc::mgr()->ConnectPorts("Receiver/Output", "Builder/Input");

      const char* outportname = 0;

      if (IsFilter()) {
         dabc::mgr()->ConnectPorts("Builder/Output", "Filter/Input");
         outportname = "Filter/Output";
      } else
         outportname = "Builder/Output";

      if (!CreateStorage(outportname)) {
         EOUT(("Not able to create storage for port %s", outportname));
      }
   }

//   SetParFixed("Status",  false);
   SetParStr("Status", "Ready");

//   SetParFixed("Status",  true);

   SetParInt("CfgConnected", 0);

   return true;
}

int bnet::WorkerApplication::IsAppModulesConnected()
{
   return GetParInt("CfgConnected")>0 ? 1 : 2;
}

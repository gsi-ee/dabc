#include "bnet/WorkerApplication.h"

#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"


bnet::WorkerApplication::WorkerApplication(const char* classname) :
   dabc::Application(classname)
{
   CreateParBool(xmlIsGenerator, false);
   CreateParBool(xmlIsSender, false);
   CreateParBool(xmlIsReceiever, false);
   CreateParBool(xmlIsFilter, false);
   CreateParInt(xmlCombinerModus, 0);
   CreateParInt(xmlNumReadouts, 0);
   for (int nr=0;nr<NumReadouts();nr++)
      CreateParStr(ReadoutParName(nr).c_str(), "");
   CreateParStr(xmlStoragePar, "");

   CreateParInt(xmlReadoutBuffer,          2*1024);
   CreateParInt(xmlReadoutPoolSize,    4*0x100000);
   CreateParInt(xmlTransportBuffer,        8*1024);
   CreateParInt(xmlTransportPoolSize, 16*0x100000);
   CreateParInt(xmlEventBuffer,           32*1024);
   CreateParInt(xmlEventPoolSize,      4*0x100000);
   CreateParInt(xmlCtrlBuffer,             2*1024);
   CreateParInt(xmlCtrlPoolSize,       2*0x100000);

   CreateParStr(parStatus, "Off");
   CreateParStr(parSendStatus, "oooo");
   CreateParStr(parRecvStatus, "oooo");


   SetParDflts(0);  // make next parameters not visible outside

   CreateParInt(CfgNodeId, dabc::mgr()->NodeId());
   CreateParInt(CfgNumNodes, dabc::mgr()->NumNodes());
   CreateParBool(CfgController, 0);
   CreateParStr(CfgSendMask, "");
   CreateParStr(CfgRecvMask, "");
   CreateParStr(CfgClusterMgr, "");
   CreateParStr(CfgNetDevice, "");
   CreateParBool(CfgConnected, false);
   CreateParInt(CfgEventsCombine, 1);
   CreateParStr(CfgReadoutPool, ReadoutPoolName);

   SetParDflts();

   DOUT1(("!!!! Worker plugin created name = %s!!!!", GetName()));
}

std::string bnet::WorkerApplication::ReadoutParName(int nreadout)
{
   std::string name;
   dabc::formats(name, "Input%dCfg", nreadout);
   return name;
}

std::string bnet::WorkerApplication::ReadoutPar(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumReadouts())) return "";
   return GetParStr(ReadoutParName(nreadout).c_str());
}

bool bnet::WorkerApplication::CreateStorage(const char* portname)
{
   dabc::Command* cmd = new dabc::Command("NullConnect");
   cmd->SetStr("Port", portname);

   return dabc::mgr()->Execute(cmd, 5);
}


void bnet::WorkerApplication::DiscoverNodeConfig(dabc::Command* cmd)
{

   DOUT1(("Process DiscoverNodeConfig sender:%s recv:%s", DBOOL(IsSender()), DBOOL(IsReceiver())));

   SetParBool(CfgController, cmd->GetBool(xmlWithController));
   SetParInt(CfgEventsCombine, cmd->GetInt(xmlNumEventsCombine, 1));
   SetParStr(CfgNetDevice, cmd->GetStr(xmlNetDevice,""));

   SetParInt(xmlCtrlBuffer, cmd->GetInt(xmlCtrlBuffer, 1024));
   SetParInt(xmlTransportBuffer, cmd->GetInt(xmlTransportBuffer, 1024));

   cmd->SetBool(xmlIsSender, IsSender());
   cmd->SetBool(xmlIsReceiever, IsReceiver());

   if (IsSender()) {

      dabc::Module* m = dabc::mgr()->FindModule("Sender");
      if (m) {
         std::string res = m->ExecuteStr("GetConfig", parRecvMask, 5);
         cmd->SetStr(parRecvMask, res.c_str());
      }
   }

   if (IsReceiver()) {
      dabc::Module* m = dabc::mgr()->FindModule("Receiver");
      if (m) {
         std::string res = m->ExecuteStr("GetConfig", parSendMask, 5);
         cmd->SetStr(parSendMask, res.c_str());
      }
   }
}

void bnet::WorkerApplication::ApplyNodeConfig(dabc::Command* cmd)
{
//   LockUnlockPars(false);

   SetParBool(CfgController, cmd->GetBool(CfgController, 0));
   SetParStr(CfgSendMask, cmd->GetStr(parSendMask, "xxxx"));
   SetParStr(CfgRecvMask, cmd->GetStr(parRecvMask, "xxxx"));
   SetParStr(CfgClusterMgr, cmd->GetStr(CfgClusterMgr,""));

//   LockUnlockPars(true);

   dabc::CommandsSet* set = new dabc::CommandsSet(cmd, false);

   dabc::Module* m = 0;

   if (IsSender()) {

      m = dabc::mgr()->FindModule("Sender");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetBool("Standalone", !GetParBool(CfgController));
         cmd->SetStr(parRecvMask, GetParStr(CfgRecvMask));

         set->Add(dabc::mgr()->LocalCmd(cmd, m));
//         m->Submit(*set, cmd);
      } else
         EOUT(("Cannot find Sender module"));
   }

   if (IsReceiver()) {
      m = dabc::mgr()->FindModule("Receiver");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetStr(parSendMask, GetParStr(CfgSendMask));
         set->Add(dabc::mgr()->LocalCmd(cmd, m));
//         m->Submit(*set, cmd);
      } else
         EOUT(("Cannot find receiver module"));

      m = dabc::mgr()->FindModule("Builder");
      if (m) {
         dabc::Command* cmd = new dabc::CmdSetParameter(parSendMask, GetParStr(CfgSendMask).c_str());
         set->Add(dabc::mgr()->LocalCmd(cmd, m));
      } else
         EOUT(("Cannot find builder module"));
   }

   set->Add(dabc::mgr()->LocalCmd(new dabc::CmdSetParameter(CfgConnected, true), this));

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
      DOUT3(( "Get reconfigure recvmask = %s", cmd->GetStr(parRecvMask)));
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

bool bnet::WorkerApplication::CreateAppModules()
{
//   LockUnlockPars(true);

   DOUT1(("CreateAppModules starts dev = %s", GetParStr(CfgNetDevice).c_str()));

   SetParStr(bnet::CfgReadoutPool, CombinerModus()==0 ? bnet::ReadoutPoolName : bnet::TransportPoolName);

   dabc::mgr()->CreateDevice(GetParStr(CfgNetDevice).c_str(), "BnetDev");

   dabc::BufferSize_t size = 0;
   dabc::BufferNum_t num = 0;


   if (IsSender() && (CombinerModus()==0)) {
      size = GetParInt(xmlReadoutBuffer, 1024);
      num = GetParInt(xmlReadoutPoolSize, 0x100000) / size;
      dabc::mgr()->CreateMemoryPool(GetParStr(CfgReadoutPool, ReadoutPoolName).c_str(), size, num);
   }

   size = GetParInt(xmlTransportBuffer, 1024);
   num = GetParInt(xmlTransportPoolSize, 16*0x100000)/ size;

   dabc::mgr()->CreateMemoryPool(bnet::TransportPoolName, size, num, 0, sizeof(bnet::SubEventNetHeader));

   if (IsReceiver()) {
      size = GetParInt(xmlEventBuffer, 2048*16);
      num = GetParInt(xmlEventPoolSize, 4*0x100000) / size;
      dabc::mgr()->CreateMemoryPool(bnet::EventPoolName, size, num);
   }

   if (IsSender()) {
      size = GetParInt(xmlCtrlBuffer, 2*1024);
      num = GetParInt(xmlCtrlPoolSize, 4*0x100000) / size;

      dabc::mgr()->CreateMemoryPool(bnet::ControlPoolName, size, num);
   }

   if (IsSender()) {

      if (!CreateCombiner("Combiner")) {
         EOUT(("Combiner module not created"));
         return false;
      }

      if (!dabc::mgr()->CreateModule("bnet::SenderModule", "Sender", SenderThreadName)) {
         EOUT(("Sender module not created"));
         return false;
      }
   }

   if (IsReceiver()) {

      if (!dabc::mgr()->CreateModule("bnet::ReceiverModule", "Receiver", ReceiverThreadName)) {
         EOUT(("Receiver module not created"));
         return false;
      }

      if (!CreateBuilder("Builder")) {
         EOUT(("EventBuilder is not created"));
         return false;
      }

      if (IsFilter() && !CreateFilter("Filter")) {
         EOUT(("EventFilter is not created"));
         return false;
      }
   }

//   dabc::mgr()->CreateMemoryPools();

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

//   SetParFixed(parStatus,  false);
   SetParStr(parStatus, "Ready");

//   SetParFixed(parStatus,  true);

   SetParBool(CfgConnected, false);

   return true;
}

int bnet::WorkerApplication::IsAppModulesConnected()
{
   return GetParBool(CfgConnected) ? 1 : 2;
}

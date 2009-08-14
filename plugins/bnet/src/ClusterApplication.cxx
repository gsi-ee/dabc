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
#include "bnet/ClusterApplication.h"

#include "dabc/Manager.h"
#include "dabc/Command.h"
#include "dabc/CommandClient.h"
#include "dabc/CommandsSet.h"
#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/Configuration.h"
#include "dabc/CommandDefinition.h"

#include "bnet/GlobalDFCModule.h"
#include "bnet/WorkerApplication.h"

#define DiscoverCmdName "DiscoverClusterConfig"

// ______________________________________________________________


namespace bnet {

   class ClusterDiscoverSet : public dabc::CommandsSet {
      public:
         ClusterDiscoverSet(ClusterApplication* plugin, dabc::Command* mastercmd) :
            dabc::CommandsSet(mastercmd, false),
            fPlugin(plugin)
         {
         }

         virtual bool ProcessCommandReply(dabc::Command* cmd)
         {
            cmd->SetBool("_ReplyRes_", cmd->GetResult());
            fPlugin->Submit(cmd);
            return true;
         }

      protected:
         ClusterApplication*  fPlugin;
   };
}

// _______________________________________________________________


bnet::ClusterApplication::ClusterApplication() :
   dabc::Application(xmlClusterClass),
   fSMMutex(),
   fSMRunningSMCmd(),
   fNumNodes(0)
{
   fNodeNames.clear();
   fNodeMask.clear();

   fNumNodes = dabc::mgr()->NumNodes();
   if (fNumNodes<1) fNumNodes = 1;

   SetParDflts(0); // mark all newly created parameters as invisible

   // register dependency against all states in the cluster, include ourself
   for (int id=0; id<fNumNodes; id++) {
      std::string parname = FORMAT(("State_%d", id));
      dabc::Parameter* par = CreateParStr(parname.c_str(), dabc::Manager::stNull);

      DOUT1(("Create parameter %s", par->GetFullName().c_str()));
      dabc::mgr()->Subscribe(par, id, dabc::Manager::stParName);
   }

   // subscribe to observe status changing on all worker nodes
   for (int n=0;n<fNumNodes;n++) {

      // no need to subscribe on status of itself - it is not exists
      if (n == dabc::Manager::Instance()->NodeId()) continue;

      const char* holdername = dabc::xmlAppDfltName;

      std::string parname, remname;

      dabc::formats(parname,"Worker%dStatus",n);
      dabc::Parameter* par = CreateParStr(parname.c_str(), "Off");
      dabc::formats(remname,"%s.%s", parStatus, holdername);
      dabc::Manager::Instance()->Subscribe(par, n, remname.c_str());

      dabc::formats(parname,"Worker%dRecvStatus",n);
      par = CreateParStr(parname.c_str(), "oooo");
      dabc::formats(remname,"%s.%s", parRecvStatus, holdername);
      dabc::Manager::Instance()->Subscribe(par, n, remname.c_str());

      dabc::formats(parname,"Worker%dSendStatus",n);
      par = CreateParStr(parname.c_str(), "oooo");
      dabc::formats(remname,"%s.%s", parSendStatus, holdername);
      dabc::Manager::Instance()->Subscribe(par, n, remname.c_str());
   }

   SetParDflts(3, false, false); // mark parameters as non-changeable from control

   CreateParInt(CfgNumNodes, fNumNodes);
   CreateParInt(CfgNodeId, dabc::mgr()->NodeId());

   SetParDflts();

   CreateParStr(xmlNetDevice, dabc::typeSocketDevice);
   CreateParBool(xmlWithController, false);
   CreateParInt(xmlNumEventsCombine, 1);
   CreateParInt(xmlCtrlBuffer,            2*1024);
   CreateParInt(xmlCtrlPoolSize,      2*0x100000);
   CreateParInt(xmlTransportBuffer,       8*1024);
   CreateParBool(xmlIsRunning, false);

   CreateParStr(xmlFileBase, "");

   CreateParInfo("Info", 1, "Green");

   dabc::CommandDefinition* def = NewCmdDef("StartFiles");
   def->AddArgument("FileBase", dabc::argString, true);
   def->Register();

   NewCmdDef("StopFiles")->Register();

   DOUT1(("Net device = %s numnodes = %d",NetDevice().c_str(), GetParInt(CfgNumNodes)));
}

bnet::ClusterApplication::~ClusterApplication()
{
   // unregister dependency for all state parameters

   for (int id=0;id<fNumNodes;id++) {
      std::string parname = FORMAT(("State_%d", id));
      dabc::Parameter* par = FindPar(parname.c_str());
      if (par) {
         dabc::mgr()->Unsubscribe(par);
         DestroyPar(par);
      }
   }
}

std::string bnet::ClusterApplication::NodeCurrentState(int nodeid)
{
   return GetParStr(FORMAT(("State_%d", nodeid)), dabc::Manager::stNull);
}

bool bnet::ClusterApplication::CreateAppModules()
{
   if (WithController()) {

      dabc::mgr()->CreateDevice(NetDevice().c_str(), "BnetDev");

      dabc::BufferSize_t size = GetParInt(xmlCtrlBuffer, 2*1024);
      dabc::BufferNum_t num = GetParInt(xmlCtrlPoolSize, 4*0x100000) / size;

      dabc::mgr()->CreateMemoryPool(bnet::ControlPoolName, size, num);

      bnet::GlobalDFCModule* m = new bnet::GlobalDFCModule("GlobalContr");
      dabc::mgr()->MakeThreadForModule(m, "GlobalContr");
   }

//   dabc::CommandDefinition* def = NewCmdDef("StartFiles");
//   def->AddArgument("FileBase", dabc::argString, true);
//   def->Register();
//
//   NewCmdDef("StopFiles")->Register();

   return true;
}

bool bnet::ClusterApplication::BeforeAppModulesDestroyed()
{
//   DeleteCmdDef("StartFiles");
//   DeleteCmdDef("StopFiles");
   return true;
}


int bnet::ClusterApplication::ExecuteCommand(dabc::Command* cmd)
{
   // This is actual place, where commands is executed or
   // slave commands are submitted and awaited certain time

   int cmd_res = cmd_false;

//   dabc::Manager* mgr = dabc::mgr();

   DOUT3(("~~~~~~~~~~~~~~~~~~~~ Process command %s", cmd->GetName()));

   if (cmd->IsName(DiscoverCmdName)) {
      // first, try to start discover, if required
      if (StartDiscoverConfig(cmd)) cmd_res = cmd_postponed;
   } else
   if (cmd->IsName("ConnectModules")) {

      // return true while no connection is required
      if (StartModulesConnect(cmd)) cmd_res = cmd_postponed;
                               else cmd_res = cmd_true;
   } else
   if (cmd->IsName("ClusterSMCommand")) {
      if (StartClusterSMCommand(cmd)) cmd_res = cmd_postponed;
   } else
   if (cmd->IsName("ApplyConfig")) {
      if (StartConfigureApply(cmd)) cmd_res = cmd_postponed;
   } else
   if (cmd->IsName("DiscoverWorkerConfig")) {
      if (cmd->GetPar("_ReplyRes_")==0) {
         EOUT(("AAAAAAAAAAAAA Something wrong with discover AAAAAAAAAAA"));
         return cmd_false;
      }

      int nodeid = cmd->GetInt("DestId");

      int mask = 0;

      if ((nodeid<0) || ((unsigned) nodeid >= fNodeMask.size())) {
         EOUT(("Discovery problem - wrong node id:%d", nodeid));
         return cmd_false;
      }

      if (cmd->GetBool("_ReplyRes_", false)) {
         if (cmd->GetBool(xmlIsSender, false))
            mask = mask | mask_Sender;

         if (cmd->GetBool(xmlIsReceiever, false))
            mask = mask | mask_Receiever;

         const char* recvmask = cmd->GetPar(parRecvMask);
         if (recvmask) fRecvMatrix[nodeid] = recvmask;
         const char* sendmask = cmd->GetPar(parSendMask);
         if (sendmask) fSendMatrix[nodeid] = sendmask;

         DOUT1(("Discover node: %d mask: %x send:%s recv:%s", nodeid, mask, sendmask, recvmask));
      } else {
         DOUT1((">>>>>> !!!!!!!! Node %d has no worker plugin or make an error", nodeid));
      }

      fNodeMask[nodeid] = mask;

      cmd_res = cmd_true;

   } else
   if (cmd->IsName("TestClusterStates")) {

      cmd_res = cmd_true;

      for (int nodeid=0;nodeid<fNumNodes; nodeid++) {
         if (!dabc::mgr()->IsNodeActive(nodeid)) continue;

         if (dabc::mgr()->CurrentState() != NodeCurrentState(nodeid)) {
            cmd_res = cmd_false;
            break;
         }
      }
   } else
   if (cmd->IsName("StartFiles") || cmd->IsName("StopFiles")) {

      if (cmd->IsName("StartFiles"))
         SetParStr("Info", dabc::format("Start files %s on all builder nodes", cmd->GetStr("FileBase", "")));
      else
         SetParStr("Info", "Stopping files on all builder nodes");

      dabc::CommandsSet* set = 0;

      for (unsigned nodeid = 0; nodeid < fNodeNames.size(); nodeid++) {
         if (!(fNodeMask[nodeid] & mask_Receiever)) continue;

         const char* nodename = fNodeNames[nodeid].c_str();

         std::string filename = cmd->GetStr("FileBase", "");
         if (filename.length() > 0)
            filename += dabc::format("_%u", nodeid);

         dabc::Command* wcmd = 0;
         if (filename.length()>0) {
            wcmd = new dabc::Command("StartFile");
            wcmd->SetStr("FileName", filename);
         } else
            wcmd = new dabc::Command("StopFile");

         dabc::SetCmdReceiver(wcmd, nodename, dabc::xmlAppDfltName);

         if (set==0) set = new dabc::CommandsSet(cmd);
         dabc::mgr()->Submit(set->Assign(wcmd));
      }

      dabc::CommandsSet::Completed(set, SMCommandTimeout());

      cmd_res = set ? cmd_postponed : cmd_true;

   } else

      cmd_res = dabc::Application::ExecuteCommand(cmd);

   return cmd_res;
}

bool bnet::ClusterApplication::StartDiscoverConfig(dabc::Command* mastercmd)
{
   DOUT3((" ClusterPlugin::StartDiscoverConfig"));

   if (!dabc::mgr()->IsMainManager()) return false;

   fNodeNames.clear();
   fNodeMask.clear();
   fSendMatrix.clear();
   fRecvMatrix.clear();

   std::string nullmask(fNumNodes, 'o');

   ClusterDiscoverSet* set = 0;

   for (int nodeid=0;nodeid<fNumNodes; nodeid++) {
      const char* nodename = dabc::mgr()->GetNodeName(nodeid);

      fNodeNames.push_back(nodename);
      fNodeMask.push_back(0);
      fSendMatrix.push_back(nullmask);
      fRecvMatrix.push_back(nullmask);

      if (!dabc::mgr()->IsNodeActive(nodeid) || (nodeid == dabc::mgr()->NodeId())) continue;

      dabc::Command* cmd = new dabc::Command("DiscoverWorkerConfig");

      cmd->SetInt("DestId", nodeid);

      cmd->SetBool(xmlWithController, WithController());
      cmd->SetInt(xmlNumEventsCombine, GetParInt(xmlNumEventsCombine, 1));
      cmd->SetInt(xmlTransportBuffer, GetParInt(xmlTransportBuffer));
      cmd->SetInt(xmlCtrlBuffer, GetParInt(xmlCtrlBuffer));
      cmd->SetStr(xmlNetDevice, NetDevice().c_str());

      dabc::SetCmdReceiver(cmd, nodename, dabc::xmlAppDfltName);

      if (set==0) set = new ClusterDiscoverSet(this, mastercmd);

      dabc::mgr()->Submit(set->Assign(cmd));
   }

   dabc::CommandsSet::Completed(set, SMCommandTimeout());

   DOUT3((" ClusterPlugin::StartDiscoverConfig"));

   return set!=0;
}

bool bnet::ClusterApplication::StartClusterSMCommand(dabc::Command* mastercmd)
{
   const char* smcmdname = mastercmd->GetStr("CmdName");
   int selectid = mastercmd->GetInt("NodeId", -1);

   DOUT2(("StartClusterSMCommand cmd:%s selected:%d", smcmdname, selectid));

   dabc::CommandsSet* set = 0;

   for (unsigned node = 0; node < fNodeNames.size(); node++) {

      DOUT4(("Node %u has mask %d", node, fNodeMask[node]));

      if (fNodeMask[node]==0) continue;

      if ((selectid>=0) && (node != (unsigned) selectid)) continue;

      dabc::Command* cmd = new dabc::CmdStateTransition(smcmdname);

      const char* nodename = fNodeNames[node].c_str();

      DOUT4(("Submit SMcmd:%s to node %s", smcmdname, nodename));

      dabc::SetCmdReceiver(cmd, nodename, "");

      if (set==0) set = new dabc::CommandsSet(mastercmd);

      dabc::mgr()->Submit(set->Assign(cmd));
   }

   DOUT3(("StartSMClusterCommand %s sel:%d", smcmdname, selectid));

   dabc::CommandsSet::Completed(set, SMCommandTimeout());

   return set!=0;
}

bool bnet::ClusterApplication::StartConfigureApply(dabc::Command* mastercmd)
{
   std::string send_mask, recv_mask;

   for (unsigned node = 0; node < fNodeNames.size(); node++) {
      send_mask += ((fNodeMask[node] & mask_Sender) ? "x" : "o");
      recv_mask += ((fNodeMask[node] & mask_Receiever) ? "x" : "o");
   }

   dabc::CommandsSet* set = 0;

   for (unsigned node = 0; node < fNodeNames.size(); node++) {
      if (fNodeMask[node]==0) continue;

      dabc::Command* cmd = new dabc::Command("ApplyConfigNode");
      const char* nodename = fNodeNames[node].c_str();

      cmd->SetBool(CfgController, WithController());
      cmd->SetStr(CfgClusterMgr, dabc::mgr()->GetName());
      cmd->SetStr(parSendMask, send_mask.c_str());
      cmd->SetStr(parRecvMask, recv_mask.c_str());

      dabc::SetCmdReceiver(cmd, nodename, dabc::xmlAppDfltName);

      if (set==0) set = new dabc::CommandsSet(mastercmd);

      dabc::mgr()->Submit(set->Assign(cmd));
   }

   if (WithController()) {
      dabc::Module* m = dabc::mgr()->FindModule("GlobalContr");
      if (m) {
         dabc::Command* cmd = new dabc::Command("Configure");
         cmd->SetStr(parSendMask, send_mask.c_str());
         cmd->SetStr(parRecvMask, recv_mask.c_str());
         if (set==0) set = new dabc::CommandsSet(mastercmd);
         m->Submit(set->Assign(cmd));
      }
   }

   DOUT3(("StartConfigureApply done"));

   dabc::CommandsSet::Completed(set, SMCommandTimeout());

   return set!=0;
}

void bnet::ClusterApplication::ParameterChanged(dabc::Parameter* par)
{
   int nodeid = -1;
   int res = sscanf(par->GetName(), "State_%d", &nodeid);

   if ((res!=1) || (nodeid<0)) return;

   std::string state;
   if (!par->GetValue(state)) return;

   bool isnormalsmcmd = false;

   DOUT2(("STATE CHANGED node:%d name:%s", nodeid, state.c_str()));

   {
      dabc::LockGuard lock(fSMMutex);
      if (fSMRunningSMCmd.length() > 0) {
        if (state.compare(dabc::Manager::TargetStateName(fSMRunningSMCmd.c_str()))!=0)
           EOUT(("Wrong result state %s of node %d during cmd %s",
                  state.c_str(), nodeid, fSMRunningSMCmd.c_str()));
        else
           isnormalsmcmd = true;
      }
   }

   // if this is just expected state change or our our own state, do nothing
   if (isnormalsmcmd || (nodeid==dabc::mgr()->NodeId())) return;

   if ((nodeid<0) || ((unsigned) nodeid >= fNodeMask.size())) {
      if (fNodeMask.size()>0)
         EOUT(("Strange node id %d changed its state", nodeid));
      return;
   }

   if ((state == dabc::Manager::stNull) && (fNodeMask[nodeid]!=0)) {
      DOUT1(("@@@@@@@@@@@@@ NODE %d changed to OFF. Need reconfigure !!!!!!!!!!!", nodeid));

      dabc::CommandsSet* set = new dabc::CommandsSet(0, false);

      set->Add(dabc::SetCmdReceiver(new dabc::Command(DiscoverCmdName), this));
      set->Add(dabc::SetCmdReceiver(new dabc::Command("ConnectModules"), this));
      set->Add(dabc::SetCmdReceiver(new dabc::Command("ApplyConfig"), this));

      dabc::CommandsSet::Completed(set, SMCommandTimeout());

      return;
   }


   if ((state == dabc::Manager::stHalted) && (fNodeMask[nodeid]==0)) {

      DOUT1(("@@@@@@@@@@@@ REALY NEW NODE %d !!!!!!!!!", nodeid));

      // this command only need to get reply when all set commands are completed
      // in this case active command will be replyed
      dabc::CommandsSet* set = new dabc::CommandsSet(0, false);

      set->Add(dabc::SetCmdReceiver(new dabc::Command(DiscoverCmdName), this));

      dabc::Command* dcmd = new dabc::Command("ClusterSMCommand");
      dcmd->SetStr("CmdName", dabc::Manager::stcmdDoConfigure);
      dcmd->SetInt("NodeId", nodeid);
      set->Add(dabc::SetCmdReceiver(dcmd, this));

      set->Add(dabc::SetCmdReceiver(new dabc::Command("ConnectModules"), this));
      set->Add(dabc::SetCmdReceiver(new dabc::Command("ApplyConfig"), this));

      dcmd = new dabc::Command("ClusterSMCommand");
      dcmd->SetStr("CmdName", dabc::Manager::stcmdDoEnable);
      dcmd->SetInt("NodeId", nodeid);
      set->Add(dabc::SetCmdReceiver(dcmd, this));

      dcmd = new dabc::Command("ClusterSMCommand");
      dcmd->SetStr("CmdName", dabc::Manager::stcmdDoStart);
      dcmd->SetInt("NodeId", nodeid);
      set->Add(dabc::SetCmdReceiver(dcmd, this));

      dabc::CommandsSet::Completed(set, SMCommandTimeout());

      return;

   }

   if (state == dabc::Manager::stReady) {
      // check if all other nodes switch to ready, that we invoke change of ourself
      bool isallready = true;

      for (int nodeid=0; nodeid < (int)fNodeMask.size(); nodeid++)
        if (nodeid!=dabc::mgr()->NodeId())
           if (NodeCurrentState(nodeid) != dabc::Manager::stReady) isallready = false;

      if (isallready && (fNodeMask.size() > 0) && IsRunning()) {
         DOUT1(("All nodes changed their states to Ready, switch cluster itself"));
         dabc::mgr()->InvokeStateTransition(dabc::Manager::stcmdDoStop);
      }
   }

}

bool bnet::ClusterApplication::StartModulesConnect(dabc::Command* mastercmd)
{
   DOUT1((" StartModulesConnect via BnetDev of class %s", NetDevice().c_str()));

   dabc::CommandsSet* set = 0;

   for (unsigned nsender = 0; nsender < fNodeNames.size(); nsender++) {
      for (unsigned nreceiver = 0; nreceiver < fNodeNames.size(); nreceiver++) {

         if (!((fNodeMask[nsender] & mask_Sender) &&
               (fNodeMask[nreceiver] & mask_Receiever))) continue;

         NodesVector sendnodes(fSendMatrix[nreceiver].c_str());
         NodesVector recvnodes(fRecvMatrix[nsender].c_str());

         if (sendnodes.HasNode(nsender) &&
             recvnodes.HasNode(nreceiver)) {
                DOUT3(("Connection %d -> %d exists", nsender, nreceiver));
                continue;
             }

         if (sendnodes.HasNode(nsender) ||
             recvnodes.HasNode(nreceiver)) {
                EOUT(("Connection %d -> %d known only from one side !!!!", nsender, nreceiver));
             }

         std::string port1name, port2name;

         const char* node1name = fNodeNames[nsender].c_str();
         const char* node2name = fNodeNames[nreceiver].c_str();

         dabc::formats(port1name, "%s$Sender/Output%u", node1name, nreceiver);
         dabc::formats(port2name, "%s$Receiver/Input%u", node2name, nsender);

         dabc::Command* cmd =
             new dabc::CmdConnectPorts(port1name.c_str(),
                                       port2name.c_str(),
                                      "BnetDev", "BnetTransport");
         cmd->SetBool(dabc::xmlUseAcknowledge, BnetUseAcknowledge);

         DOUT3(( "DoConnection %d -> %d ", nsender, nreceiver));

         if (set==0) set = new dabc::CommandsSet(mastercmd);

         dabc::mgr()->Submit(set->Assign(cmd));
      }
   }

   if (WithController())
      for (unsigned nsender = 0; nsender < fNodeNames.size(); nsender++) {

         if ((fNodeMask[nsender] & mask_Sender) == 0) continue;
         std::string port1name, port2name;

         const char* node1name = fNodeNames[nsender].c_str();
         const char* node2name = dabc::mgr()->GetName();

         dabc::formats(port1name, "%s$Sender/CtrlPort", node1name);
         dabc::formats(port2name, "%s$GlobalContr/Sender%u", node2name, nsender);

         dabc::Command* cmd =
            new dabc::CmdConnectPorts(port1name.c_str(),
                                         port2name.c_str(),
                                         "BnetDev");

         if (set==0) set = new dabc::CommandsSet(mastercmd);

         dabc::mgr()->Submit(set->Assign(cmd));
      }

   dabc::CommandsSet::Completed(set, SMCommandTimeout());

   return set!=0;
}

bool bnet::ClusterApplication::ExecuteClusterSMCommand(const char* smcmdname)
{
   dabc::Command* cmd = new dabc::Command("ClusterSMCommand");
   cmd->SetStr("CmdName", smcmdname);
   return Execute(cmd);
}

bool bnet::ClusterApplication::ActualTransition(const char* state_trans_name)
{
   std::string filebase = GetParStr(xmlFileBase,"");

   if (strcmp(state_trans_name, dabc::Manager::stcmdDoConfigure)==0) {
      if (!Execute(DiscoverCmdName)) return false;
      if (!Execute("CreateAppModules")) return false;
      if (!ExecuteClusterSMCommand(state_trans_name)) return false;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoEnable)==0) {
      if (!Execute("ConnectModules")) return false;
      if (!Execute("ApplyConfig")) return false;
      if (!ExecuteClusterSMCommand(state_trans_name)) return false;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoStart)==0) {
      if (!ExecuteClusterSMCommand(state_trans_name)) return false;
      if (filebase.length()>0) {
         DOUT0(("Start files with base  = %s", filebase.c_str()));
         dabc::Command* cmd = new dabc::Command("StartFiles");
         cmd->SetStr("FileBase", filebase);
         if (!Execute(cmd)) return false;
      }
      if (!Execute("BeforeAppModulesStarted", SMCommandTimeout())) return false;
      if (!dabc::mgr()->StartAllModules()) return false;
      if (!Execute(new dabc::CmdSetParameter(xmlIsRunning, true))) return false;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoStop)==0) {
      if (!Execute(new dabc::CmdSetParameter(xmlIsRunning, false))) return false;
      if (!dabc::mgr()->StopAllModules()) return false;
      if (!Execute("AfterAppModulesStopped", SMCommandTimeout())) return false;
      if (filebase.length()>0)
         if (!Execute("StopFiles")) return false;
      if (!ExecuteClusterSMCommand(state_trans_name)) return false;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoHalt)==0) {
      if (!Execute("BeforeAppModulesDestroyed", SMCommandTimeout())) return false;
      if (!dabc::mgr()->CleanupManager()) return false;
      if (!ExecuteClusterSMCommand(state_trans_name)) return false;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoError)==0) {
      dabc::mgr()->StopAllModules();

      if (!ExecuteClusterSMCommand(state_trans_name)) {
         EOUT(("Cannot move cluster to Error state !"));
         return false;
      }
   } else
      return false;

   return true;
}


bool bnet::ClusterApplication::DoStateTransition(const char* state_trans_cmd)
{
   {
      dabc::LockGuard lock(fSMMutex);
      fSMRunningSMCmd = state_trans_cmd;
   }

   SetParStr("Info", dabc::format("Do transition %s", state_trans_cmd));

   bool res = ActualTransition(state_trans_cmd);

   {
      dabc::LockGuard lock(fSMMutex);
      fSMRunningSMCmd = "";
   }

   return res;
}



extern "C" void RunTestBnet()
{
   DOUT1(("TestBnet start"));

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoConfigure);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoEnable);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Main loop", 10);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Again main loop", 10);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoHalt);

   DOUT1(("TestBnet done"));
}


extern "C" void RunTestBnetFiles()
{

   DOUT1(("TestBnetFiles start"));

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoConfigure);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoEnable);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Before files", 5); //15

   dabc::Command* cmd = new dabc::Command("StartFiles");
   cmd->SetStr("FileBase","abc");
   dabc::mgr()->GetApp()->Execute(cmd);

   dabc::ShowLongSleep("Writing files", 5); //15

   dabc::mgr()->GetApp()->Execute("StopFiles");

   dabc::ShowLongSleep("After files", 5); //15

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Again main loop", 15); //10

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoHalt);

   DOUT1(("TestBnetFiles done"));
}


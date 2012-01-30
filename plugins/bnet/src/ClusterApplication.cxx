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
#include "dabc/CommandsSet.h"
#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/Configuration.h"
#include "dabc/Url.h"

#include "bnet/GlobalDFCModule.h"
#include "bnet/WorkerApplication.h"

#define DiscoverCmdName "DiscoverClusterConfig"

// ______________________________________________________________


namespace bnet {

   class ClusterDiscoverSet : public dabc::CommandsSet {
      public:
         ClusterDiscoverSet(ClusterApplication* plugin) :
            dabc::CommandsSet(plugin->thread()),
            fPlugin(plugin)
         {
         }

         virtual void SetCommandCompleted(dabc::Command cmd)
         {
            fPlugin->DiscoverCommandCompleted(cmd);
         }

      protected:
         ClusterApplication*  fPlugin;
   };
}

// _______________________________________________________________


bnet::ClusterApplication::ClusterApplication(const char* clname) :
   dabc::Application(clname ? clname : xmlClusterClass),
   fNumNodes(0)
{
   fNodeNames.clear();
   fNodeMask.clear();

   fNumNodes = dabc::mgr()->NumNodes();
   if (fNumNodes<1) fNumNodes = 1;

   // register dependency against all states in the cluster, include ourself
   for (int id=0; id<fNumNodes; id++) {
      std::string parname = dabc::format("State_%d", id);
      CreatePar(parname).DfltStr(stNull());

      DOUT1(("Create parameter %s", parname.c_str()));

      // FIXME: should we support this interface ??
      // dabc::mgr()->Subscribe(par, id, dabc::Manager::stParName);
   }

   // subscribe to observe status changing on all worker nodes
   for (int n=0;n<fNumNodes;n++) {

      // no need to subscribe on status of itself - it is not exists
      if (n == dabc::mgr()->NodeId()) continue;

      //const char* holdername = dabc::xmlAppDfltName;

      std::string parname, remname;

      dabc::formats(parname,"Worker%dStatus",n);
      CreatePar(parname).SetStr("Off");
      //dabc::formats(remname,"%s.%s", parStatus, holdername);
      //dabc::mgr()->Subscribe(par, n, remname.c_str());

      dabc::formats(parname,"Worker%dRecvStatus",n);
      CreatePar(parname).SetStr("oooo");
      //dabc::formats(remname,"%s.%s", parRecvStatus, holdername);
      //dabc::mgr()->Subscribe(par, n, remname.c_str());

      dabc::formats(parname,"Worker%dSendStatus",n);
      CreatePar(parname).SetStr("oooo");
      //dabc::formats(remname,"%s.%s", parSendStatus, holdername);
      //dabc::mgr()->Subscribe(par, n, remname.c_str());
   }

   CreatePar(CfgNumNodes).DfltInt(fNumNodes);
   CreatePar(CfgNodeId).DfltInt(dabc::mgr()->NodeId());

   CreatePar(xmlNetDevice).DfltStr(dabc::typeSocketDevice);
   CreatePar(xmlWithController).DfltBool(false);
   CreatePar(xmlNumEventsCombine).DfltInt(1);
   CreatePar(xmlCtrlBuffer).DfltInt(2*1024);
   CreatePar(xmlCtrlPoolSize).DfltInt(2*0x100000);
   CreatePar(xmlTransportBuffer).DfltInt(8*1024);
   CreatePar(xmlIsRunning).DfltBool(false);

   CreatePar(xmlFileBase).DfltStr("");

   CreatePar("Info", "info");

   CreateCmdDef("StartFiles").AddArg("FileBase", "string", true);

   CreateCmdDef("StopFiles");

   DOUT1(("Net device = %s numnodes = %d",NetDevice().c_str(), Par(CfgNumNodes).AsInt()));
}

bnet::ClusterApplication::~ClusterApplication()
{
   // unregister dependency for all state parameters

   DOUT0(("Start ~ClusterApplication"));
/*
   for (int id=0;id<fNumNodes;id++) {
      std::string parname = FORMAT(("State_%d", id));
      dabc::Parameter* par = FindPar(parname.c_str());
      if (par) {
         dabc::mgr()->Unsubscribe(par);
         DestroyPar(parname);
      }
   }
*/
   DOUT0(("Finish ~ClusterApplication"));
}

std::string bnet::ClusterApplication::NodeCurrentState(int nodeid)
{
   return Par(FORMAT(("State_%d", nodeid))).AsStdStr(stNull());
}

int bnet::ClusterApplication::NumWorkers()
{
   return fNumNodes;
}

bool bnet::ClusterApplication::IsWorkerActive(int nodeid)
{
   if ((nodeid<0) || (nodeid>=(int)fNodeMask.size())) return false;

   return fNodeMask[nodeid] != 0;
}

const char* bnet::ClusterApplication::GetWorkerNodeName(int nodeid)
{
   if ((nodeid<0) || (nodeid>=(int)fNodeNames.size())) return 0;

   return fNodeNames[nodeid].c_str();
}


bool bnet::ClusterApplication::CreateAppModules()
{
   if (WithController()) {

      dabc::mgr.CreateDevice(NetDevice().c_str(), "BnetDev");

      dabc::BufferSize_t size = Par(xmlCtrlBuffer).AsInt(2*1024);
      dabc::unsigned num = Par(xmlCtrlPoolSize).AsInt(4*0x100000) / size;

      dabc::mgr.CreateMemoryPool(bnet::ControlPoolName, size, num);

      bnet::GlobalDFCModule* m = new bnet::GlobalDFCModule("GlobalContr");
      dabc::mgr()->MakeThreadFor(m, "GlobalContr");
   }

   return true;
}

bool bnet::ClusterApplication::BeforeAppModulesDestroyed()
{
//   DeleteCmdDef("StartFiles");
//   DeleteCmdDef("StopFiles");
   return true;
}

void bnet::ClusterApplication::DiscoverCommandCompleted(dabc::Command cmd)
{
   int nodeid = cmd.GetInt("DestId");

   if ((nodeid<0) || ((unsigned) nodeid >= fNodeMask.size())) {
      EOUT(("Discovery problem - wrong node id:%d", nodeid));
      return;
   }

   int mask = 0;

   if (cmd.GetResult()==dabc::cmd_true) {
      if (cmd.GetBool(xmlIsSender, false))
         mask = mask | mask_Sender;

      if (cmd.GetBool(xmlIsReceiever, false))
         mask = mask | mask_Receiever;

      const char* recvmask = cmd.GetField(parRecvMask);
      if (recvmask) fRecvMatrix[nodeid] = recvmask;
      const char* sendmask = cmd.GetField(parSendMask);
      if (sendmask) fSendMatrix[nodeid] = sendmask;

      DOUT1(("Discover node: %d mask: %x send:%s recv:%s", nodeid, mask, sendmask, recvmask));
   } else {
      DOUT1((">>>>>> !!!!!!!! Node %d has no worker plugin or make an error", nodeid));
   }

   fNodeMask[nodeid] = mask;
}


int bnet::ClusterApplication::ExecuteCommand(dabc::Command cmd)
{
   // This is actual place, where commands is executed or
   // slave commands are submitted and awaited certain time

   int cmd_res = dabc::cmd_false;

//   dabc::Manager* mgr = dabc::mgr();

   DOUT3(("~~~~~~~~~~~~~~~~~~~~ Process command %s", cmd.GetName()));

   if (cmd.IsName(DiscoverCmdName)) {
      // first, try to start discover, if required
      if (StartDiscoverConfig(cmd)) cmd_res = dabc::cmd_postponed;
   } else
   if (cmd.IsName("ConnectModules")) {

      // return true while no connection is required
      if (StartModulesConnect(cmd)) cmd_res = dabc::cmd_postponed;
                               else cmd_res = dabc::cmd_true;
   } else
   if (cmd.IsName("ClusterSMCommand")) {
      if (StartClusterSMCommand(cmd)) cmd_res = dabc::cmd_postponed;
   } else
   if (cmd.IsName("ApplyConfig")) {
      if (StartConfigureApply(cmd)) cmd_res = dabc::cmd_postponed;
   } else
   if (cmd.IsName("TestClusterStates")) {

      cmd_res = dabc::cmd_true;

      for (int nodeid=0;nodeid<fNumNodes; nodeid++) {
         if (!dabc::mgr()->IsNodeActive(nodeid)) continue;

         //TODO: change logic
/*         if (dabc::mgr()->CurrentState() != NodeCurrentState(nodeid)) {
            cmd_res = dabc::cmd_false;
            break;
         }
*/
      }
   } else
   if (cmd.IsName("StartFiles") || cmd.IsName("StopFiles")) {

      if (cmd.IsName("StartFiles"))
         Par("Info").SetStr(dabc::format("Start files %s on all builder nodes", cmd.GetStr("FileBase", "")));
      else
         Par("Info").SetStr("Stopping files on all builder nodes");

      dabc::CommandsSet* set = 0;

      for (unsigned nodeid = 0; nodeid < fNodeNames.size(); nodeid++) {
         if (!(fNodeMask[nodeid] & mask_Receiever)) continue;

//         const char* nodename = fNodeNames[nodeid].c_str();

         std::string filename = cmd.GetStr("FileBase", "");
         if (filename.length() > 0)
            filename += dabc::format("_%u", nodeid);

         dabc::Command wcmd;
         if (filename.length()>0) {
            wcmd = dabc::Command("StartFile");
            wcmd.SetStr("FileName", filename);
         } else
            wcmd = dabc::Command("StopFile");

         wcmd.SetReceiver(nodeid, dabc::xmlAppDfltName);

         if (set==0) set = new dabc::CommandsSet(thread());
         set->Add(wcmd, dabc::mgr());
      }

      if (set) set->SubmitSet(cmd, SMCommandTimeout());

      cmd_res = set ? dabc::cmd_postponed : dabc::cmd_true;

   } else

      cmd_res = dabc::Application::ExecuteCommand(cmd);

   return cmd_res;
}

bool bnet::ClusterApplication::StartDiscoverConfig(dabc::Command mastercmd)
{
   DOUT3((" ClusterPlugin::StartDiscoverConfig"));

   if (dabc::mgr()->NodeId()!=0) return false;

   fNodeNames.clear();
   fNodeMask.clear();
   fSendMatrix.clear();
   fRecvMatrix.clear();

   std::string nullmask(fNumNodes, 'o');

   ClusterDiscoverSet* set = 0;

   for (int nodeid=0;nodeid<fNumNodes; nodeid++) {
      std::string nodename = dabc::Url::ComposeItemName(nodeid);

      fNodeNames.push_back(nodename);
      fNodeMask.push_back(0);
      fSendMatrix.push_back(nullmask);
      fRecvMatrix.push_back(nullmask);

      if (!dabc::mgr()->IsNodeActive(nodeid) || (nodeid == dabc::mgr()->NodeId())) continue;

      dabc::Command cmd("DiscoverWorkerConfig");

      cmd.SetInt("DestId", nodeid);

      cmd.SetBool(xmlWithController, WithController());
      cmd.SetInt(xmlNumEventsCombine, Par(xmlNumEventsCombine).AsInt(1));
      cmd.SetInt(xmlTransportBuffer, Par(xmlTransportBuffer).AsInt());
      cmd.SetInt(xmlCtrlBuffer, Par(xmlCtrlBuffer).AsInt());
      cmd.Field(xmlNetDevice).SetStr(NetDevice());

      cmd.SetReceiver(nodeid, dabc::xmlAppDfltName);

      if (set==0) set = new ClusterDiscoverSet(this);

      set->Add(cmd, dabc::mgr());
   }

   if (set) set->SubmitSet(mastercmd, SMCommandTimeout());

   DOUT3((" ClusterPlugin::StartDiscoverConfig"));

   return set!=0;
}

bool bnet::ClusterApplication::StartClusterSMCommand(dabc::Command mastercmd)
{
   const char* smcmdname = mastercmd.GetStr("CmdName");
   int selectid = mastercmd.GetInt("NodeId", -1);

   DOUT2(("StartClusterSMCommand cmd:%s selected:%d", smcmdname, selectid));

   dabc::CommandsSet* set = 0;

   for (unsigned node = 0; node < fNodeNames.size(); node++) {

      DOUT4(("Node %u has mask %d", node, fNodeMask[node]));

      if (fNodeMask[node]==0) continue;

      if ((selectid>=0) && (node != (unsigned) selectid)) continue;

//      dabc::CmdStateTransition cmd(smcmdname);
//      cmd.SetReceiver(node, "");

      if (set==0) set = new dabc::CommandsSet(thread());

//      set->Add(cmd, dabc::mgr());
   }

   DOUT3(("StartSMClusterCommand %s sel:%d", smcmdname, selectid));

   if (set) set->SubmitSet(mastercmd, SMCommandTimeout());

   return set!=0;
}

bool bnet::ClusterApplication::StartConfigureApply(dabc::Command mastercmd)
{
   std::string send_mask, recv_mask;

   for (unsigned node = 0; node < fNodeNames.size(); node++) {
      send_mask += ((fNodeMask[node] & mask_Sender) ? "x" : "o");
      recv_mask += ((fNodeMask[node] & mask_Receiever) ? "x" : "o");
   }

   dabc::CommandsSet* set = 0;

   for (unsigned node = 0; node < fNodeNames.size(); node++) {
      if (fNodeMask[node]==0) continue;

      dabc::Command cmd("ApplyConfigNode");
//      const char* nodename = fNodeNames[node].c_str();

      cmd.SetBool(CfgController, WithController());
      cmd.SetStr(CfgClusterMgr, dabc::mgr()->GetName());
      cmd.SetStr(parSendMask, send_mask.c_str());
      cmd.SetStr(parRecvMask, recv_mask.c_str());

      cmd.SetReceiver(node, dabc::xmlAppDfltName);

      if (set==0) set = new dabc::CommandsSet(thread());

      set->Add(cmd, dabc::mgr());
   }

   if (WithController()) {
      dabc::ModuleRef m = dabc::mgr.FindModule("GlobalContr");
      if (!m.null()) {
         dabc::Command cmd("Configure");
         cmd.SetStr(parSendMask, send_mask.c_str());
         cmd.SetStr(parRecvMask, recv_mask.c_str());
         if (set==0) set = new dabc::CommandsSet(thread());
         set->Add(cmd, m());
      }
   }

   DOUT3(("StartConfigureApply done"));

   if (set) set->SubmitSet(mastercmd, SMCommandTimeout());

   return set!=0;
}

/*  TODO: replace reaction on remote changes in ProceeParameterEvent method


void bnet::ClusterApplication::ParameterChanged(dabc::Parameter* par)
{
   int nodeid = -1;
   int res = sscanf(par->GetName(), "State_%d", &nodeid);

   if ((res!=1) || (nodeid<0)) return;

   std::string state;
   if (!par->GetValue(state)) return;

   bool isnormalsmcmd = false;

   DOUT0(("STATE CHANGED node:%d name:%s", nodeid, state.c_str()));

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

      dabc::CommandsSet* set = new dabc::CommandsSet(thread(), false);

      set->Add(dabc::Command(DiscoverCmdName).SetReceiver(this));
      set->Add(dabc::Command("ConnectModules").SetReceiver(this));
      set->Add(dabc::Command("ApplyConfig").SetReceiver(this));

      set->SubmitSet(0, SMCommandTimeout());

      return;
   }


   if ((state == dabc::Manager::stHalted) && (fNodeMask[nodeid]==0)) {

      DOUT1(("@@@@@@@@@@@@ REALY NEW NODE %d !!!!!!!!!", nodeid));

      // this command only need to get reply when all set commands are completed
      // in this case active command will be replyed
      dabc::CommandsSet* set = new dabc::CommandsSet(thread(), false);

      set->Add(dabc::Command(DiscoverCmdName).SetReceiver(this));

      dabc::Command dcmd("ClusterSMCommand");
      dcmd.SetStr("CmdName", dabc::Manager::stcmdDoConfigure);
      dcmd.SetInt("NodeId", nodeid);
      set->Add(dcmd.SetReceiver(this));

      set->Add(dabc::Command("ConnectModules").SetReceiver(this));
      set->Add(dabc::Command("ApplyConfig").SetReceiver(this));

      dcmd = dabc::Command("ClusterSMCommand");
      dcmd.SetStr("CmdName", dabc::Manager::stcmdDoEnable);
      dcmd.SetInt("NodeId", nodeid);
      set->Add(dcmd.SetReceiver(this));

      dcmd = dabc::Command("ClusterSMCommand");
      dcmd.SetStr("CmdName", dabc::Manager::stcmdDoStart);
      dcmd.SetInt("NodeId", nodeid);
      set->Add(dcmd.SetReceiver(this));

      set->SubmitSet(0, SMCommandTimeout());

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

*/

bool bnet::ClusterApplication::StartModulesConnect(dabc::Command mastercmd)
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

         dabc::CmdConnectPorts cmd(port1name.c_str(),
                                   port2name.c_str(),
                                   "BnetDev", "BnetTransport");
         cmd.SetBool(dabc::xmlUseAcknowledge, BnetUseAcknowledge);

         DOUT3(( "DoConnection %d -> %d ", nsender, nreceiver));

         if (set==0) set = new dabc::CommandsSet(thread());

         set->Add(cmd, dabc::mgr());
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

         dabc::CmdConnectPorts cmd(port1name.c_str(),
                                   port2name.c_str(),
                                   "BnetDev");

         if (set==0) set = new dabc::CommandsSet(thread());

         set->Add(cmd, dabc::mgr());
      }

   if (set) set->SubmitSet(mastercmd, SMCommandTimeout());

   return set!=0;
}

bool bnet::ClusterApplication::ExecuteClusterSMCommand(const std::string& smcmdname)
{
   dabc::Command cmd("ClusterSMCommand");
   cmd.SetStr("CmdName", smcmdname);
   return Execute(cmd);
}

bool bnet::ClusterApplication::ActualTransition(const std::string& trans_name)
{
   std::string filebase = Par(xmlFileBase).AsStdStr();

   if (trans_name == stcmdDoConfigure()) {
      if (!Execute(DiscoverCmdName)) return false;
      if (!Execute("CreateAppModules")) return false;
      if (!ExecuteClusterSMCommand(trans_name)) return false;
   } else
   if (trans_name == stcmdDoEnable()) {
      if (!Execute("ConnectModules")) return false;
      if (!Execute("ApplyConfig")) return false;
      if (!ExecuteClusterSMCommand(trans_name)) return false;
   } else
   if (trans_name == stcmdDoStart()) {
      if (!ExecuteClusterSMCommand(trans_name)) return false;
      if (filebase.length()>0) {
         DOUT0(("Start files with base  = %s", filebase.c_str()));
         dabc::Command cmd("StartFiles");
         cmd.SetStr("FileBase", filebase);
         if (!Execute(cmd)) return false;
      }
      if (!Execute("BeforeAppModulesStarted", SMCommandTimeout())) return false;
      if (!dabc::mgr.StartAllModules()) return false;
      if (!Par(xmlIsRunning).SetBool(true)) return false;
   } else
   if (trans_name == stcmdDoStop()) {
      if (!Par(xmlIsRunning).SetBool(false)) return false;
      if (!dabc::mgr.StopAllModules()) return false;
      if (!Execute("AfterAppModulesStopped", SMCommandTimeout())) return false;
      if (filebase.length()>0)
         if (!Execute("StopFiles")) return false;
      if (!ExecuteClusterSMCommand(trans_name)) return false;
   } else
   if (trans_name == stcmdDoHalt()) {
      if (!Execute("BeforeAppModulesDestroyed", SMCommandTimeout())) return false;
      if (!dabc::mgr.CleanupApplication()) return false;
      if (!ExecuteClusterSMCommand(trans_name)) return false;
   } else
   if (trans_name == stcmdDoError()) {
      dabc::mgr.StopAllModules();

      if (!ExecuteClusterSMCommand(trans_name)) {
         EOUT(("Cannot move cluster to Error state !"));
         return false;
      }
   } else
      return false;

   return true;
}


extern "C" void RunTestBnet()
{
/*

   DOUT1(("TestBnet start"));

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoConfigure);

   dabc::mgr()->Sleep(1);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoEnable);

   dabc::mgr()->Sleep(1);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStart);

   dabc::mgr()->Sleep(10, "Main loop");

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr()->Sleep(1);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStart);

   dabc::mgr()->Sleep(10, "Again main loop");

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr()->Sleep(1);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoHalt);

   DOUT1(("TestBnet done"));
*/
}


extern "C" void RunTestBnetFiles()
{

   DOUT1(("TestBnetFiles start"));
/*
   dabc::mgr.ChangeState(dabc::Manager::stcmdDoConfigure);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoEnable);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStart);

   dabc::mgr()->Sleep(5, "Before files"); //15

   dabc::Command cmd("StartFiles");
   cmd.SetStr("FileBase","abc");
   dabc::mgr.GetApp().Execute(cmd);

   dabc::mgr()->Sleep(5, "Writing files"); //15

   dabc::mgr.GetApp().Execute("StopFiles");

   dabc::mgr()->Sleep(5, "After files"); //15

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStart);

   dabc::mgr()->Sleep(15, "Again main loop"); //10

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoStop);

   dabc::mgr.ChangeState(dabc::Manager::stcmdDoHalt);

*/
   DOUT1(("TestBnetFiles done"));
}


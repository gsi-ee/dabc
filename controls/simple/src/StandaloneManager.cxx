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
#include "dabc/StandaloneManager.h"

#include <fstream>

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"
#include "dabc/Configuration.h"

#include "dabc/CommandChannelModule.h"

namespace dabc {
   class StandaloneMgrFactory : public Factory {
      public:
         StandaloneMgrFactory(const char* name) : Factory(name) { }

      protected:
         virtual bool CreateManagerInstance(const char* kind, Configuration* cfg);
   };

}

dabc::StandaloneMgrFactory standalonemgrfactory("standalone");

bool dabc::StandaloneMgrFactory::CreateManagerInstance(const char* kind, Configuration* cfg)
{
   if ((kind!=0) && (strcmp(kind,"Standalone")==0)) {
      new dabc::StandaloneManager(cfg->MgrName(), cfg->MgrNodeId(), cfg->MgrNumNodes(), true, cfg);
      return true;
   }
   return false;
}

const char* GetMyHostName()
{
   static char MyHostName[500];

   strcpy(MyHostName, "");

   if (gethostname(MyHostName, sizeof(MyHostName))==0) return MyHostName;

   strcpy(MyHostName, getenv("HOSTNAME"));

   if (strlen(MyHostName)==0) strcpy(MyHostName, "host");

   return MyHostName;
}

dabc::StandaloneManager::StandaloneManager(const char* name, int nodeid, int numnodes, bool usesm, Configuration* cfg) :
   Manager(name ? name : FORMAT(("%s-%d", GetMyHostName(), nodeid)), false, cfg),
   fIsMainMgr(false),
   fMainMgrName(),
   fClusterNames(),
   fClusterActive(),
   fNodeId(nodeid),
   fCmdChannel(0),
   fCmdDeviceId(),
   fCmdDevName(),
   fStopCond(),
   fParReg()
{
   if (numnodes<=0) numnodes = 1;
   if (nodeid>=numnodes) nodeid = 0;

   fIsMainMgr = (nodeid==0);

   while (fClusterNames.size() < (unsigned) numnodes) {
      fClusterNames.push_back("");
      fClusterActive.push_back(false);
   }

   fClusterNames[NodeId()] = GetName();
   fClusterActive[NodeId()] = true;

   // if this is main manager, we know that we are alive
   if (fIsMainMgr)
      fMainMgrName = GetName();

   if (usesm) InitSMmodule();

   init();
}

dabc::StandaloneManager::~StandaloneManager()
{
   DOUT3(("dabc::StandaloneManager::~StandaloneManager() starts"));

   if (IsMainManager())
      DisconnectCmdChannels();
   else
      WaitDisconnectCmdChannel();

   fCmdChannel = 0;
   CleanupManager(77);

   destroy();

   DOUT3(("dabc::StandaloneManager::~StandaloneManager() done"));
}

bool dabc::StandaloneManager::ConnectControl(const char* connid)
{
   ConnectCmdChannel(NumNodes(), 1, connid);
   return true;
}

void dabc::StandaloneManager::ConnectCmdChannel(int numnodes, int deviceid, const char* controllerID)
{
   const char* devname = "MgrDev";
   const char* devclass = "";

   if (deviceid==1) devclass = typeSocketDevice; else
   if (deviceid==2) devclass = "verbs::Device";

   Device* dev = 0;
   if (CreateDevice(devclass, devname))
      dev = FindDevice(devname);

   if ((dev==0) || (dev->ProcessorThread()==0)) {
      EOUT(("Cannot create device %s of class %s  for command channel. Abort", devname, devclass));
      exit(1);
   }

   fCmdDevName = dev->GetFullName(this);
   DOUT1(("Device name = %s", fCmdDevName.c_str()));

   fCmdChannel = new CommandChannelModule(IsMainManager() ? numnodes-1 : 1);

   fCmdChannel->AssignProcessorToThread(dev->ProcessorThread());

   fCmdChannel->SetAppId(77);
   fCmdChannel->GetPool()->SetAppId(77);
   dev->SetAppId(77);

   fCmdChannel->Start();
   DOUT3(( "Started CommandChannelModule module"));

   dabc::CommandClient cli;

   if (IsMainManager()) {

      Command* scmd = new Command("StartServer");
      scmd->SetStr("CmdChannel", "StdMgrCmd");
      scmd->SetKeepAlive(true);
      SubmitLocal(cli, scmd, fCmdDevName.c_str());

      if (cli.WaitCommands(10)) {
         fCmdDeviceId = scmd->GetPar("ConnId");

         std::ofstream f(controllerID);
         f << fCmdDeviceId << std::endl;
         dabc::Command::Finalise(scmd);
      } else {
         EOUT(("Cannot start server. HALT"));
         dabc::Command::Finalise(scmd);
         exit(1);
      }

/*
      int cnt = 100;

      while ((cnt-->0) && (NumActiveNodes()<numnodes)) {
         MicroSleep(100000);
      }

      if (NumActiveNodes() < numnodes) {
         EOUT(("Not all nodes are connected. Try to continue"));
      }
*/

   } else {

      Command* cmdr = new Command("RegisterSlave");
      cmdr->SetStr("SlaveName", GetName());
      cmdr->SetInt("SlaveNodeId", fNodeId);
      cmdr->SetKeepAlive(true);

      SubmitLocal(cli, Device::MakeRemoteCommand(cmdr, controllerID, "StdMgrCmd"), fCmdDevName.c_str());

      if (cli.WaitCommands(7)) {
         DOUT2(("RegisterSlave execution OK serv = %s connid = %s",
           cmdr->GetStr("ServerId","null"), cmdr->GetPar("ConnId")));
      } else {
         EOUT(("RegisterSlave command fail. Halt"));
         exit(1);
      }

      fMainMgrName = cmdr->GetStr("MainMgr","Node0");
      fClusterNames[0] = fMainMgrName;
      fClusterActive[0] = true;

      DOUT2(("Main manager = %s", fMainMgrName.c_str()));

      Command* cmd = new CmdDirectConnect(false, "CommandChannel/Port0");
      cmd->SetPar("ConnId", cmdr->GetPar("ConnId"));
      cmd->SetPar("ServerId", cmdr->GetPar("ServerId"));
      cmd->SetBool("ServerUseAckn", true);
      SubmitLocal(cli, cmd, fCmdDevName.c_str());

      dabc::Command::Finalise(cmdr);

      bool res = cli.WaitCommands(5);

      if (!res) {
         EOUT(("Not able to connect commands channel"));
         exit(1);
      }
   }

/*
   DOUT3(("Is it OK ?????"));


   if (IsMainManager()) {

      MicroSleep(1000000);
      DOUT1(("Now numactive = %d", NumActiveNodes()));

      if (NumActiveNodes()<numnodes)
        for (int node = 0; node <NumNodes();node++)
           if ((node!=NodeId()) && !IsNodeActive(node))
              DOUT1(("  Node %d is not active", node));
   }
*/
}

void dabc::StandaloneManager::ConnectCmdChannelOld(int numnodes, int deviceid, const char* controllerID)
{
   const char* devname = "MgrDev";
   const char* devclass = "";

   if (deviceid==1) devclass = typeSocketDevice; else
   if (deviceid==2) devclass = "verbs::Device";

   Device* dev = 0;
   if (CreateDevice(devclass, devname))
      dev = FindDevice(devname);

   if ((dev==0) || (dev->ProcessorThread()==0)) {
      EOUT(("Cannot create device %s of class %s  for command channel. Abort", devname, devclass));
      exit(1);
   }

   fCmdDevName = dev->GetFullName(this /*dev->GetParent()*/);
   DOUT1(("Device name = %s", fCmdDevName.c_str()));

   fCmdChannel = new CommandChannelModule(IsMainManager() ? numnodes-1 : 1);

   fCmdChannel->AssignProcessorToThread(dev->ProcessorThread());

   fCmdChannel->SetAppId(77);
   fCmdChannel->GetPool()->SetAppId(77);
   dev->SetAppId(77);

   // up to now we are in normal situation

   fCmdChannel->Start();
   DOUT3(( "Started CommandChannelModule module"));

   dabc::CommandClient cli;

   ChangeManagerName(FORMAT(("Node%d", fNodeId)));

   fMainMgrName = "Node0";

   if (IsMainManager()) {

      // start server without extra cmd channel
      Command* scmd = new Command("StartServer");
//      scmd->SetStr("CmdChannel","StdMgrCmd");
      scmd->SetKeepAlive(true);
      SubmitLocal(cli, scmd, fCmdDevName.c_str());

      if (cli.WaitCommands(10)) {
         fCmdDeviceId = scmd->GetPar("ConnId");

         std::ofstream f(controllerID);
         f << fCmdDeviceId << std::endl;
         dabc::Command::Finalise(scmd);
      } else {
         EOUT(("Cannot start server. HALT"));
         dabc::Command::Finalise(scmd);
         exit(1);
      }

      fClusterNames[0] = "Node0";

      for (int nrem = 1; nrem < numnodes; nrem++)
         fClusterNames[nrem] = FORMAT(("Node%d", nrem));

      for (int nslave=1; nslave < NumNodes(); nslave++) {
         Command* cmd = new CmdDirectConnect(true, FORMAT(("CommandChannel/Port%d", nslave-1)));
         cmd->SetPar("ConnId", FORMAT(("CommandChannel-node%d", nslave)));
         SubmitLocal(cli, cmd, fCmdDevName.c_str());
      }

   } else {

      fClusterNames[0] = fMainMgrName;
      fClusterActive[0] = true;

      Command* cmd = new CmdDirectConnect(false, "CommandChannel/Port0");
      cmd->SetPar("ConnId", FORMAT(("CommandChannel-node%d", fNodeId)));
      cmd->SetPar("ServerId", controllerID);
      cmd->SetBool("ServerUseAckn", true);
      SubmitLocal(cli, cmd, fCmdDevName.c_str());
   }

   bool res = cli.WaitCommands(5);

   if (!res) {
      EOUT(("Not able to connect commands channel"));
      exit(1);
   }

   MicroSleep(1000000);
   DOUT1(("Is it OK ?????"));

   if (IsMainManager()) {
      DOUT1(("Now ready numactive = %d ????? ", NumActiveNodes()));
   }
}

void dabc::StandaloneManager::DisconnectCmdChannels()
{

   DOUT1(("StandaloneManager::DisconnectCmdChannels chnl = %p", fCmdChannel));

   if (fCmdChannel==0) return;

   dabc::CommandClient cli;

   for (int node=1;node<NumNodes();node++)
      if (IsNodeActive(node))
         SubmitRemote(cli, new dabc::Command("DisconnectCmdChannel"), node);

   bool res = cli.WaitCommands(10);

   if (!res) {
      EOUT(("Cannot disconnect correctly command channels"));
   }
}

void dabc::StandaloneManager::WaitDisconnectCmdChannel()
{
   if (fCmdChannel==0) return;

   // first, wait that "DisconnectCmdChannel" is coming
   fStopCond.DoWait();

   // second, wait several seconds that connection is broken
   bool res = fStopCond.DoWait(5.);

   if (!res) {
      EOUT(("Stop condition was not correctly fired"));
   }

   DOUT4(("WaitDisconnectCmdChannel done"));
}

bool dabc::StandaloneManager::CanSendCmdToManager(const char* mgrname)
{
   if ((mgrname==0) || (*mgrname==0)) return false;

   int findnodeid = -1;

   for (unsigned n=0;n<fClusterNames.size();n++)
      if (fClusterNames[n].compare(mgrname)==0) {
         findnodeid = n;
         break;
      }

   if (findnodeid<0) return false;

   if (IsMainManager()) {
      return fClusterActive[findnodeid];
   } else {
      return (findnodeid==0) || (findnodeid==NodeId());
   }

   return false;
}

int dabc::StandaloneManager::DefinePortId(const char* managername)
{
   if (IsMainManager()) {
      for (int n=1;n<NumNodes();n++)
         if (fClusterNames[n].compare(managername) == 0)
            if (fClusterActive[n]) return n-1;
   } else
      // we can only send command to master
      if (fMainMgrName.compare(managername) == 0) return 0;

   return -1;
}


bool dabc::StandaloneManager::SendOverCommandChannel(const char* managername, const char* cmddata)
{
   int portid = DefinePortId(managername);

   if (portid<0) {
      EOUT(("Cannot submit command to %s", managername));
      return false;
   }

   dabc::Command* cmd = new dabc::Command("SendCommand");
   cmd->SetInt("PortId", portid);
   cmd->SetPar("Value", cmddata);

   return fCmdChannel->Submit(cmd);
}

int dabc::StandaloneManager::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("DoCommandRecv")) {
      const char* cmddata = cmd->GetPar("cmddata");

      DOUT3(("DoCommandRecv cmd:%s", cmddata));

      RecvOverCommandChannel(cmddata);
   } else
   if (cmd->IsName("ActivateSlave")) {
      int nodeid = cmd->GetInt("PortId",0);
      bool isactive = (cmd->GetInt("IsActive", 0) > 0);
      if (IsMainManager()) {
          nodeid++;
          if ((nodeid >= 0) && (nodeid<NumNodes())) {
             fClusterActive[nodeid] = isactive;
             CheckSubscriptionList();
          }
      } else {
         if (nodeid!=0) EOUT(("Strange cmd channel node id = %d", nodeid));
         // second time fire condition when node is disconnected
         // we fire two time to be sure that even in case of error application
         // will be halted
         if ((nodeid==0) && !isactive) {
            DOUT1(("Master is disconnected. Can cleanup"));
            fStopCond.DoFire();
            fStopCond.DoFire();
         }
      }

   } else
   if (cmd->IsName("RegisterSlave")) {

      if (fCmdChannel==0) return cmd_false;

      const char* slavename = cmd->GetPar("SlaveName");
      int slaveid = cmd->GetInt("SlaveNodeId", 0);

      if ((slaveid<=0) || (slaveid>=NumNodes())) {
         EOUT(("Wrong id of node %s %d", slavename, slaveid));
         return cmd_false;
      }

      fClusterNames[slaveid] = slavename;

      std::string connid;
      formats(connid, "CmdChl_%s", slavename);

      // prepare server for connection
      Command* ccmd = new CmdDirectConnect(true, FORMAT(("CommandChannel/Port%d", slaveid-1)));
      ccmd->SetPar("ConnId", connid.c_str());
      ccmd->SetBool("ServerUseAckn", true);
      SubmitLocal(*this, ccmd, fCmdDevName.c_str());

      // reply with complete info that client can start connection
      cmd->SetStr("MainMgr", GetName());
      cmd->SetStr("DeviceId", fCmdDevName);
      cmd->SetStr("ServerId", fCmdDeviceId);
      cmd->SetStr("ConnId", connid);
   } else
   if (cmd->IsName("DisconnectCmdChannel")) {
      // release condition first time
      fStopCond.DoFire();
   } else
   if (cmd->IsName("SubscribeParam")) {

      const char* srcname = cmd->GetStr("SrcName", "");
      const char* tgtname = cmd->GetStr("TgtName", "");

      dabc::Parameter* par = dynamic_cast<dabc::Parameter*> (FindControlObject(srcname));
      // dynamic_cast<dabc::Parameter*>(FindChild(srcname));

      if (par==0) {
         EOUT(("Cannot find parameter with name %s", srcname));
         return cmd_false;
      }

      ParamReg reg(par);
      reg.ismaster = true;
      reg.srcnode = NodeId();
      reg.tgtnode = cmd->GetInt("TgtNode", 0);
      reg.remname = tgtname;
      reg.active = true;

      // add entry to the list
      {
         LockGuard guard(GetMutex());
         fParReg.push_back(reg);
      }

      DOUT3(( "Subscribe to %s par:%p from node:%d", srcname, par, reg.tgtnode));

      // probably, submit parameter change already now?
      ParameterEvent(par, 1);

   } else
   if (cmd->IsName("UnsubscribeParam")) {
      const char* tgtname = cmd->GetStr("TgtName", "");
      int tgtnode = cmd->GetInt("TgtNode", 0);

      LockGuard guard(GetMutex());

      ParamRegList::iterator iter = fParReg.begin();
      while (iter!=fParReg.end()) {
         ParamRegList::iterator curr(iter++);
         if (curr->ismaster && (curr->tgtnode==tgtnode) &&
             (curr->remname.compare(tgtname)==0)) {
                DOUT1(("Unsubscribe parameter %s", curr->par->GetName()));
                fParReg.erase(curr);
                break;
             }
      }

   } else
      return Manager::ExecuteCommand(cmd);

   return cmd_true;
}

void dabc::StandaloneManager::ParameterEvent(Parameter* par, int eventid)
{
   dabc::Manager::ParameterEvent(par, eventid);

//   if (event==1)
//      DOUT1(("Parameter: %s Value: %5.1f", par->GetName(), par->GetDouble()));

   // we are not interesting in parameter creation event
   if (eventid==parCreated) return;

   // when parameter is destroyed, simple remove it from the lists
   if (eventid==parDestroy) {
      LockGuard guard(GetMutex());

      ParamRegList::iterator iter = fParReg.begin();
      while (iter!=fParReg.end()) {
         ParamRegList::iterator curr(iter++);
         if (curr->par==par) fParReg.erase(curr);
      }

      // !!!!!!!!!! TODO in case of master parameter, deactivate slave record
      // and in case of slave parameter, deactivate master record

      return;
   }

//   std::string value;

   LockGuard guard(GetMutex());

   ParamRegList::iterator iter = fParReg.begin();
   while (iter!=fParReg.end()) {
      if (iter->par == par)
         SubscribedParChanged(*iter);
      iter++;
   }
}

void dabc::StandaloneManager::SubscribedParChanged(ParamReg& reg)
{
   // check if this is master parameter and it is active
   if (!reg.active || !reg.ismaster) return;

   std::string value;

   // here send to remote node command to update slave parameter value
   if (!reg.par->GetValue(value)) {
      EOUT(("Cannot convert parameter %s to string", reg.par->GetName()));
      reg.active = false;
      return;
   }

   DOUT3(( "Subscribed parameter changed %s tgt:%d %s",
      reg.par->GetName(), reg.tgtnode, reg.remname.c_str()));

   Command* cmd = new CmdSetParameter(reg.par->GetName(), value.c_str());

   if (!Submit(RemoteCmd(cmd, reg.tgtnode, reg.remname.c_str())))
      EOUT(("Cannot send parameter change"));

   DOUT3(("SubscribedParChanged completed"));
}


bool dabc::StandaloneManager::Subscribe(Parameter* par, int remnode, const char* remname)
{
   Unsubscribe(par);

   ParamReg reg(par);

   reg.ismaster = false;
   reg.srcnode = remnode;
   reg.tgtnode = NodeId();
   reg.remname = remname;
   reg.active = false;
   par->GetValue(reg.defvalue);

   {
      LockGuard guard(GetMutex());
      fParReg.push_back(reg);
   }

   CheckSubscriptionList();

   return true;
}

bool dabc::StandaloneManager::Unsubscribe(Parameter* par)
{
   CommandsQueue cmds;

   {
      LockGuard guard(GetMutex());

      ParamRegList::iterator iter = fParReg.begin();
      while (iter!=fParReg.end()) {
         ParamRegList::iterator curr(iter++);
         if ((curr->par==par) && !curr->ismaster) {

            if (curr->active) {
               // send unsubscribe to the node

               std::string fullname;
               par->MakeFullName(fullname, this);

               Command* cmd = new Command("UnsubscribeParam");
               cmd->SetStr("TgtName", fullname);
               cmd->SetInt("TgtNode", NodeId());
               cmds.Push(RemoteCmd(cmd, curr->srcnode, 0));
            }
         }
      }
   }

   while (cmds.Size()>0) Submit(cmds.Pop());

   return true;
}

void dabc::StandaloneManager::CheckSubscriptionList()
{
   LockGuard guard(GetMutex());

   DOUT3(("CheckSubscriptionList sz = %u", fParReg.size()));

   ParamRegList::iterator iter = fParReg.begin();
   while (iter!=fParReg.end()) {
      ParamRegList::iterator curr(iter++);

      if (curr->ismaster && !curr->active) {
         EOUT(("Should not happen - par:%s record must be deleted when node %d disconnected", curr->par->GetName(), curr->tgtnode));
         fParReg.erase(curr);
         continue;
      }

      if (curr->ismaster) {
         // this is case of master parameter

         if (!IsNodeActive(curr->tgtnode)) {
            if (curr->active) {
               DOUT3(( "Master record deactivated %s and removed", curr->remname.c_str()));
               curr->active = false;
               fParReg.erase(curr);
            }
         } else {
            if (!curr->active) {
               curr->active = true;
               EOUT(("Should not happen - par:%s record must be deleted when node %d disconnected", curr->par->GetName(), curr->tgtnode));

               // send parameter update to remote node
               ParameterEvent(curr->par, 1);
            }

         }
      } else
      if (curr->tgtnode == NodeId()) {
         // this is case of slave parameter

         if (!IsNodeActive(curr->srcnode)) {
            if (curr->active) {
               DOUT3(( "Slave record deactivated %s", curr->remname.c_str()));
               curr->active = false;
               curr->par->InvokeChange(curr->defvalue.c_str());
            }

         } else {
            if (!curr->active) {
               DOUT3(( "Slave record activated node:%d %s", curr->srcnode, curr->remname.c_str()));
               curr->active = true;

               std::string fullname;
               curr->par->MakeFullName(fullname, this);

               // send subscribe to the node
               Command* cmd = new Command("SubscribeParam");
               cmd->SetStr("SrcName", curr->remname);
               cmd->SetStr("TgtName", fullname);
               cmd->SetInt("TgtNode", NodeId());
               Submit(RemoteCmd(cmd, curr->srcnode, 0));
            }
         }
      } else {
         EOUT(("Strange entry in subscription list"));
      }
   }
}

int dabc::StandaloneManager::NumNodes()
{
   return fClusterNames.size();
}

const char* dabc::StandaloneManager::GetNodeName(int num)
{
   if ((unsigned) num >= fClusterNames.size()) return 0;

   return fClusterNames[num].c_str();
}

bool dabc::StandaloneManager::IsNodeActive(int num)
{
   if ((unsigned) num >= fClusterNames.size()) return false;

   if (fClusterNames[num].size()==0) return false;

   return fClusterActive[num];
}

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

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/Timer.h"
#include "dabc/Command.h"
#include "dabc/StandaloneManager.h"
#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/threads.h"
#include "dabc/Application.h"
#include "dabc/SocketDevice.h"
#include "dabc/statistic.h"
#include "dabc/TimeSyncModule.h"
#include "dabc/Factory.h"
#include "dabc/Configuration.h"


#include <map>
#include <math.h>
#include <queue>

int TestBufferSize = 8*1024;
int TestSendQueueSize = 5;
int TestRecvQueueSize = 10;
bool TestUseAckn = false;

const int FirstNode = 0;

//#include <iostream>

class Test1SendModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle*   fPool;
      bool                fCanSend;
      unsigned            fPortCnt;
      bool                fChaotic;

   public:
      Test1SendModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         int nports = GetCfgInt("NumPorts", 3, cmd);
         int buffsize = GetCfgInt("BufferSize", 16*1024, cmd);
         fChaotic = GetCfgBool("Chaotic", true, cmd);

         fPool = CreatePoolHandle("SendPool", buffsize, 5);

         dabc::RateParameter* rate = CreateRateParameter("OutRate", false, 3.);
         rate->SetLimits(0.,100.);
         rate->SetUnits("MB/s");
         //rate->SetDebugOutput(true);

         for (int n=0;n<nports;n++) {
            dabc::Port* port = CreateOutput(FORMAT(("Output%d", n)), fPool, TestSendQueueSize);
            port->SetOutRateMeter(rate);
         }

         fCanSend = false;
         fPortCnt = 0;

         DOUT1(("new TSendModule %s nports = %d buf = %d done", GetName(), NumOutputs(), buffsize));
      }

      int ExecuteCommand(dabc::Command* cmd)
      {
         if (cmd->IsName("EnableSending")) {
            fCanSend = cmd->GetBool("Enable", true);
            if (fCanSend) StartSending();
            fPortCnt = 0;
            return cmd_true;
         }

         return ModuleAsync::ExecuteCommand(cmd);
      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         if (!fCanSend) return;

         if (fChaotic) {
            dabc::Buffer* buf = fPool->TakeBuffer();
            port->Send(buf);
            return;
         }

         int cnt = 1000;

         while (Output(fPortCnt)->CanSend()) {
            dabc::Buffer* buf = fPool->TakeBuffer();
            Output(fPortCnt)->Send(buf);
            fPortCnt = (fPortCnt+1) % NumOutputs();
            if (cnt-- == 0) break;
         }
      }

      void StartSending()
      {
         for(int n=0;n<TestSendQueueSize;n++)
            for(unsigned nout=0;nout<NumOutputs();nout++)
               if (Output(nout)->CanSend()) {
                  dabc::Buffer* buf = fPool->TakeBuffer();
                  if (buf==0) { EOUT(("AAAAAAAAA")); }
                  Output(nout)->Send(buf);
               }
      }

      void AfterModuleStop()
      {
         fCanSend = false;
         DOUT0(("TSendModule finish Rate %s", GetParStr("OutRate").c_str()));
      }
};


class Test1RecvModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle*    fPool;
      int                  fSleepTime;

   public:
      Test1RecvModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         // we will use queue (second true) in the signal to detect order of signal fire
         int nports = GetCfgInt("NumPorts", 3, cmd);
         int buffsize = GetCfgInt("BufferSize", 16*1024, cmd);
         fSleepTime = GetCfgInt("SleepTime", 0, cmd);

         // 0 here means that we need at least 1 buffer more for module and force by that pool request scheme
         // 1 is just exactly that we need to work without requests
         fPool = CreatePoolHandle("RecvPool", buffsize, 5);

         dabc::RateParameter* rate = CreateRateParameter("InpRate", false, 3.);
         rate->SetLimits(0.,100.);
         rate->SetUnits("MB/s");
         // rate->SetDebugOutput(true);

         for (int n=0;n<nports;n++) {
            dabc::Port* port = CreateInput(FORMAT(("Input%d", n)), fPool, TestRecvQueueSize);
            port->SetInpRateMeter(rate);
         }

         DOUT1(("new TRecvModule %s nports:%d buf:%d", GetName(), nports, buffsize));

         fSleepTime = 0;
      }

      int ExecuteCommand(dabc::Command* cmd)
      {
         if (cmd->IsName("ChangeSleepTime")) {
            fSleepTime = cmd->GetInt("SleepTime", 0);
         } else
            return ModuleAsync::ExecuteCommand(cmd);

         return cmd_true;
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer* ref = port->Recv();
         if (ref==0) { EOUT(("AAAAAAA")); }

         dabc::Buffer::Release(ref);

         if (fSleepTime>0) {
//            dabc::TimeStamp_t tm1 = TimeStamp();
            dabc::MicroSleep(fSleepTime);
//            dabc::TimeStamp_t tm2 = TimeStamp();
         }
      }

      void AfterModuleStop()
      {
         DOUT0(("TRecvModule finish Rate %s", GetParStr("InpRate").c_str()));
      }
};


class NetTestFactory : public dabc::Factory  {
   public:

      NetTestFactory(const char* name) : dabc::Factory(name) {}

      virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
      {
         if (strcmp(classname,"Test1SendModule")==0)
            return new Test1SendModule(modulename, cmd);
         else
         if (strcmp(classname,"Test1RecvModule")==0)
            return new Test1RecvModule(modulename, cmd);
//         else
//         if (strcmp(classname,"Test1WorkerModule")==0)
//            return new Test1WorkerModule(modulename, cmd);
//         else
//         if (strcmp(classname,"Test2SendModule")==0)
//            return new Test2SendModule(modulename, cmd);
//         else
//         if (strcmp(classname,"Test2RecvModule")==0)
//            return new Test2RecvModule(modulename, cmd);
//         else
//         if (strcmp(classname,"Test2WorkerModule")==0)
//            return new Test2WorkerModule(modulename, cmd);

         return 0;
      }
};

NetTestFactory nettest("net-test");



bool StartAll()
{
   dabc::CommandClient cli;

   for (int node=0;node<dabc::mgr()->NumNodes();node++)
      dabc::mgr()->SubmitRemote(cli, new dabc::CmdStartAllModules(), node);

   bool res = cli.WaitCommands(3);
   DOUT0(("StartAll() res = %s", DBOOL(res)));

   return res;
}

bool EnableSending(bool on = true)
{
   dabc::CommandClient cli;

   for (int node=0;node<dabc::mgr()->NumNodes();node++) {
      dabc::Command* cmd = new dabc::Command("EnableSending");
      cmd->SetBool("Enable", on);
      dabc::mgr()->SubmitRemote(cli, cmd, node, "Sender");
   }

   return cli.WaitCommands(3);
}

bool StopAll()
{
   dabc::CommandClient cli;

   for (int node=0;node<dabc::mgr()->NumNodes();node++)
      dabc::mgr()->SubmitRemote(cli, new dabc::CmdStopAllModules(), node);

   bool res = cli.WaitCommands(3);
   DOUT0(("StopAll() res = %s", DBOOL(res)));

   return res;
}

extern "C" void RunAllToAll()
{
   if (dabc::mgr()->NodeId()!=0) return;
   int numnodes = dabc::mgr()->NumNodes();

   std::string devclass = dabc::mgr()->cfg()->GetUserPar("NetDevice", dabc::typeSocketDevice);

   TestBufferSize = dabc::mgr()->cfg()->GetUserParInt(dabc::xmlBufferSize, TestBufferSize);
   TestSendQueueSize = dabc::mgr()->cfg()->GetUserParInt(dabc::xmlOutputQueueSize, TestSendQueueSize);
   TestRecvQueueSize = dabc::mgr()->cfg()->GetUserParInt(dabc::xmlInputQueueSize, TestRecvQueueSize);
   TestUseAckn = dabc::mgr()->cfg()->GetUserParInt(dabc::xmlUseAcknowledge, TestUseAckn ? 1 : 0) > 0;

   DOUT0(("Run All-to-All test numnodes = %d buffer size = %d", numnodes, TestBufferSize));

   bool res = false;

   dabc::CommandClient cli;

   for (int node=0;node<numnodes;node++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("Test1RecvModule","Receiver");
      cmd->SetInt("NumPorts", numnodes-1);
      cmd->SetInt("BufferSize", TestBufferSize);
      dabc::mgr()->SubmitRemote(cli, cmd, node);
   }

   for (int node=0;node<numnodes;node++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("Test1SendModule","Sender");
      cmd->SetInt("NumPorts", numnodes-1);
      cmd->SetInt("BufferSize", TestBufferSize);
      dabc::mgr()->SubmitRemote(cli, cmd, node);
   }

   res = cli.WaitCommands(5);

   DOUT0(("CreateAllModules() res = %s", DBOOL(res)));

   const char* devname = "Test1Dev";

   for (int node = 0; node < numnodes; node++)
      dabc::mgr()->SubmitRemote(cli, new dabc::CmdCreateDevice(devclass.c_str(), devname), node);

   if (!cli.WaitCommands(5)) {
      EOUT(("Cannot create devices of class %s", devclass.c_str()));
      return;
   }

   for (int nsender = 0; nsender < numnodes; nsender++) {
      for (int nreceiver = 0; nreceiver < numnodes; nreceiver++) {
          if (nsender==nreceiver) continue;

          std::string port1name, port2name;

          dabc::formats(port1name, "%s$Sender/Output%d", dabc::mgr()->GetNodeName(nsender), (nreceiver>nsender ? nreceiver-1 : nreceiver));
          dabc::formats(port2name, "%s$Receiver/Input%d", dabc::mgr()->GetNodeName(nreceiver), (nsender>nreceiver ? nsender-1 : nsender));

          dabc::Command* cmd =
             new dabc::CmdConnectPorts(port1name.c_str(),
                                       port2name.c_str(),
                                       devname, "TransportThrd");
          cmd->SetBool(dabc::xmlUseAcknowledge, TestUseAckn);

          dabc::mgr()->SubmitCl(cli, cmd);
      }
   }
   res = cli.WaitCommands(5);
   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));

   StartAll();

   EnableSending(true);

   dabc::ShowLongSleep("Waiting", 10);

   StopAll();
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/* Adresse für multicast IP */
const char* mcast_host = "224.0.0.15";
const int mcast_port = 7234;

int setup_multicast_socket () {
  int loop = 1;
  int socket_descriptor;
  struct sockaddr_in sin;
  struct ip_mreq command;

  memset (&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl (INADDR_ANY);
  sin.sin_port = htons (mcast_port);
  if((socket_descriptor=socket (PF_INET, SOCK_DGRAM, 0)) == -1) {
     EOUT(("socket()"));
     return -1;
  }
  /* Mehreren Prozessen erlauben den selben Port zu nutzen */
  loop = 1;
  if (setsockopt ( socket_descriptor,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &loop, sizeof (loop)) < 0) {
     EOUT(("setsockopt:SO_REUSEADDR"));
     return -1;
  }
  if(bind( socket_descriptor,
           (struct sockaddr *)&sin,
           sizeof(sin)) < 0) {
     EOUT(("bind"));
     return -1;
  }
  /* Broadcast auf dieser Maschine zulassen */
  loop = 1;
  if (setsockopt ( socket_descriptor,
                   IPPROTO_IP,
                   IP_MULTICAST_LOOP,
                   &loop, sizeof (loop)) < 0) {
     EOUT(("setsockopt:IP_MULTICAST_LOOP"));
     return -1;
  }

  /* Join the broadcast group: */
  command.imr_multiaddr.s_addr = inet_addr (mcast_host);
  command.imr_interface.s_addr = htonl (INADDR_ANY);
  if (command.imr_multiaddr.s_addr == -1) {
     EOUT(("%s ist keine Multicast-Adresse", mcast_host));
     return -1;
  }
  if (setsockopt ( socket_descriptor,
                   IPPROTO_IP,
                   IP_ADD_MEMBERSHIP,
                   &command, sizeof (command)) < 0) {
    EOUT(("setsockopt:IP_ADD_MEMBERSHIP"));
    return -1;
  }
  return socket_descriptor;
}


extern "C" void RunMulticastTest()
{
   if (dabc::mgr()->NodeId()==0) {
      // this is sender
      int socket_descriptor;
      struct sockaddr_in address;

      socket_descriptor = socket (AF_INET, SOCK_DGRAM, 0);
      if (socket_descriptor == -1) {
         EOUT(("socket()"));
         return;
      }
      memset (&address, 0, sizeof (address));
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = inet_addr (mcast_host);
      address.sin_port = htons (mcast_port);

//      if(bind(socket_descriptor,
//               (struct sockaddr *)&address,
//                 sizeof(address)) < 0) {
//         EOUT(("bind"));
//         return;
//      }
//
//      int loop = 1;
//      if (setsockopt(socket_descriptor,
//                     IPPROTO_IP,
//                     IP_MULTICAST_LOOP,
//                     &loop, sizeof (loop)) < 0) {
//         EOUT(("setsockopt:IP_MULTICAST_LOOP"));
//         return;
//      }


      char msg[1000];

      int cnt = 0;

      /* Broadcasting beginnen */
      while (cnt++ < 100) {
        sprintf(msg, "Broadcast test #%d", cnt);

        if (sendto( socket_descriptor,
                    msg, strlen(msg)+1,
                    0,
                   (struct sockaddr *) &address,
                   sizeof (address)) < 0) {
             EOUT(("sendto()"));
             break;
          }

//        if (send(socket_descriptor, msg, strlen(msg)+1, 0) <0) {
//           EOUT(("send()"));
//           break;
//        }

        dabc::MicroSleep(100000);
      }

      close(socket_descriptor);

      DOUT0(("Server is finished..."));

   } else {
      // all other are receivers

      int iter = 0;
      socklen_t sin_len;
      char message[256];
      int socket;
      struct sockaddr_in sin;
      struct hostent *server_host_name;
      struct ip_mreq command;

      if ((server_host_name = gethostbyname (mcast_host)) == 0) {
         EOUT(("gethostbyname"));
         return;
      }
      socket = setup_multicast_socket ();
      /* Broadcast-Nachrichten empfangen */
      while (iter++ < 50) {
//         sin_len = sizeof (sin);
//         if (recvfrom ( socket, message, 256, 0,
//                       (struct sockaddr *) &sin, &sin_len) == -1) {
//            EOUT(("recvfrom"));
//        }

         if (recv(socket, message, 256, 0) < 0) {
             EOUT(("recv"));
             break;
         }

        DOUT0(("Antwort #%-2d vom Server: %s", iter, message));
        dabc::MicroSleep(1000);
      }

      command.imr_multiaddr.s_addr = inet_addr (mcast_host);
      command.imr_interface.s_addr = htonl (INADDR_ANY);

      /* Multicast Socket aus der Gruppe entfernen */
      if (setsockopt ( socket,
                       IPPROTO_IP,
                       IP_DROP_MEMBERSHIP,
                       &command, sizeof (command)) < 0 ) {
         EOUT(("setsockopt:IP_DROP_MEMBERSHIP"));
      }
      close (socket);
   }

}




//class Test1WorkerModule : public dabc::ModuleSync {
//   protected:
//
//      int fCounter;
//
//   public:
//      Test1WorkerModule(const char* name, dabc::Command* cmd) :
//         dabc::ModuleSync(name, cmd)
//      {
//         fCounter = 0;
//         SetTmoutExcept(true);
//      }
//
//      Test1WorkerModule(const char* name) :
//         dabc::ModuleSync(name)
//      {
//         fCounter = 0;
//         SetTmoutExcept(true);
//      }
//
//      int ExecuteCommand(dabc::Command* cmd)
//      {
//         if (cmd->IsName("ResetCounter")) {
//            fCounter = 0;
//         } else
//            return ModuleSync::ExecuteCommand(cmd);
//
//         return cmd_true;
//      }
//
//      virtual void MainLoop()
//      {
//         DOUT1(("TWorkerModule mainloop"));
//
//         while (ModuleWorking(100)) {
//            fCounter++;
//
//            // lets say, about 10 milisecond
//            for (int k=0;k<10000/110;k++) {
//              // this peace of code takes about 110 microsec
//               double sum = 0;
//               for (int n=0;n<10000;n++)
//                 sum+=sqrt(n*1.);
//            }
//         }
//         DOUT1(("TWorkerModule %s finished cnt=%7d tm %4.1f", GetName(), fCounter, fCounter/100.));
//      }
//};
//
//
//void CreateAllModules(dabc::StandaloneManager &m, int numworkers = 0)
//{
//   dabc::CommandClient cli;
//
//   for (int node=FirstNode;node<m.NumNodes();node++) {
//      dabc::Command* cmd =
//         new dabc::CmdCreateModule("Test1RecvModule","Receiver");
//      cmd->SetInt("NumPorts", m.NumNodes()-1-FirstNode);
//      cmd->SetInt("BufferSize", TestBufferSize);
//      m.SubmitRemote(cli, cmd, node);
//   }
//
//   for (int node=FirstNode;node<m.NumNodes();node++) {
//      dabc::Command* cmd =
//         new dabc::CmdCreateModule("Test1SendModule","Sender");
//      cmd->SetInt("NumPorts", m.NumNodes()-1-FirstNode);
//      cmd->SetInt("BufferSize", TestBufferSize);
//      m.SubmitRemote(cli, cmd, node);
//   }
//
//   for (int nw=0;nw<numworkers;nw++)
//      for (int node=FirstNode;node<m.NumNodes();node++) {
//         dabc::Command* cmd =
//            new dabc::CmdCreateModule("Test1WorkerModule", FORMAT(("Worker%d",nw)));
//         m.SubmitRemote(cli, cmd, node);
//      }
//
//   bool res = cli.WaitCommands(5);
//
//   DOUT1(("CreateAllModules() res = %s", DBOOL(res)));
//}
//
//void ConnectModules(dabc::StandaloneManager &m, int deviceid = 1)
//{
//   dabc::CommandClient cli;
//
//   const char* devname = "Test1Dev";
//
//   const char* deviceclass = dabc::typeSocketDevice;
//   if (deviceid==2) deviceclass = "verbs::Device";
//
//   for (int node = FirstNode; node < m.NumNodes(); node++)
//      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);
//
//   if (!cli.WaitCommands(5)) {
//      EOUT(("Cannot create devices of class %s", deviceclass));
//      exit(1);
//   }
//
//   for (int nsender = FirstNode; nsender < m.NumNodes(); nsender++) {
//      for (int nreceiver = FirstNode; nreceiver < m.NumNodes(); nreceiver++) {
//          if (nsender==nreceiver) continue;
//
//          std::string port1name, port2name;
//
//          dabc::formats(port1name, "%s$Sender/Output%d", m.GetNodeName(nsender), (nreceiver>nsender ? nreceiver-1 : nreceiver) - FirstNode);
//          dabc::formats(port2name, "%s$Receiver/Input%d", m.GetNodeName(nreceiver), (nsender>nreceiver ? nsender-1 : nsender) - FirstNode);
//
//          dabc::Command* cmd =
//             new dabc::CmdConnectPorts(port1name.c_str(),
//                                          port2name.c_str(),
//                                          devname, "TransportThrd");
//          cmd->SetBool(dabc::xmlUseAcknowledge, TestUseAckn);
//
//          m.SubmitCl(cli, cmd);
////          break;
//      }
////      break;
//   }
//   bool res = cli.WaitCommands(5);
//   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));
//}
//
//void SetPriorities(dabc::StandaloneManager &m, int prio = 0)
//{
//   dabc::CommandClient cli;
//
//   for (int node=FirstNode;node<m.NumNodes();node++) {
//      dabc::Command* cmd = new dabc::Command("SetPriority");
//      cmd->SetInt("Priority", prio);
//      m.SubmitRemote(cli, cmd, node, "Receiver");
//   }
//   for (int node=FirstNode;node<m.NumNodes();node++) {
//      dabc::Command* cmd = new dabc::Command("SetPriority");
//      cmd->SetInt("Priority", prio);
//      m.SubmitRemote(cli, cmd, node, "Sender");
//   }
//
//   bool res = cli.WaitCommands(1);
//   DOUT1(("SetPriorities res = %s", DBOOL(res)));
//}
//
//void ChangeSleepTime(dabc::StandaloneManager &m, int tm=0, int selectnode = -1)
//{
//   dabc::CommandClient cli;
//
//   for (int node=FirstNode;node<m.NumNodes();node++) {
//      if ((selectnode>=0) && (node!=selectnode)) continue;
//      dabc::Command* cmd = new dabc::Command("ChangeSleepTime");
//      cmd->SetInt("SleepTime", tm);
//      m.SubmitRemote(cli, cmd, node, "Receiver");
//   }
//   cli.WaitCommands(1);
//}
//
//void StopAll(dabc::StandaloneManager &m)
//{
//   dabc::CommandClient cli;
//
//   for (int node=FirstNode;node<m.NumNodes();node++)
//      m.SubmitRemote(cli, new dabc::CmdStopAllModules(), node);
//   bool res = cli.WaitCommands(5);
//   DOUT1(("StopAll res = %s", DBOOL(res)));
//}
//
//void CleanupAll(dabc::StandaloneManager &m)
//{
//   dabc::CommandClient cli;
//
//   for (int node=FirstNode;node<m.NumNodes();node++)
//      m.SubmitRemote(cli, new dabc::CmdCleanupManager(), node);
//   bool res = cli.WaitCommands(5);
//   DOUT1(("CleanupAll res = %s", DBOOL(res)));
//}
//
//
//void TestWorker(dabc::StandaloneManager &mgr)
//{
//   dabc::SetDebugLevel(3);
//
//   Test1WorkerModule* m = new Test1WorkerModule("Combiner");
//
//   mgr.MakeThreadForModule(m, "Thrd1");
//
//   m->Start();
//
//   DOUT1(("Start called"));
//
//   dabc::ShowLongSleep("Working", 3);
//   DOUT1(("Work finished"));
//
//   m->Stop();
//
//   DOUT1(("Stop called"));
//}
//
//
///*
//
//int main(int numc, char* args[])
//{
//   int nodeid = 0;
//   int numnodes = 4;
//   int devices = 11; // only sockets
//   const char* controllerID = "file.txt";
//   if (numc>1) nodeid = atoi(args[1]);
//   if (numc>2) numnodes = atoi(args[2]);
//   if (numc>3) devices = atoi(args[3]);
//   if (numc>4) controllerID = args[4];
//
////   if (numc>5) dabc::SocketDevice::SetLocalHost(args[5]);
//
//   dabc::SetDebugLevel(1);
//
//   dabc::StandaloneManager manager(0, nodeid, numnodes, false);
//
////   TestWorker(manager);
////   return 0;
//
//
//   manager.ConnectCmdChannel(numnodes, devices / 10, controllerID);
//
//   new Test1Plugin();
//
//   DOUT1(("READY"));
//
//   if (nodeid==0) {
//       dabc::SetDebugLevel(1);
//
////       CheckIntQueue();
//
//       sleep(1);
//
//       int numworkers = 0;
//
//       CreateAllModules(manager, numworkers);
//       ConnectModules(manager, devices % 10);
//       sleep(1);
//
//       StartAll(manager);
//       sleep(1);
//
////       dabc::SetDebugLevel(5);
//
//       EnableSending(manager, true);
////       CheckLocking();
//
//       dabc::ShowLongSleep("Main loop", 5);
//
//       EnableSending(manager, false);
//
////       dabc::SetDebugLevel(1);
//
//       sleep(1);
//       StopAll(manager);
//
////       manager.SetPriorities(60);
////       sleep(1);
//
////       manager.ChangeSleepTime(50000, 2);
////       sleep(1);
//
////       EnableSending(manager);
//
////       for (int tm=10;tm<=500;tm+=10) {
////          manager.ChangeSleepTime(tm);
////          sleep(5);
////       }
//
////       manager.ChangeSleepTime(0);
////       sleep(1);
//
//
////       sleep(1); // need for master node to be able smoothly stop threads
//
////       DeleteAll(manager);
//
//       CleanupAll(manager);
//       sleep(2);
//
//
//       DOUT1(("\n\n\n =============== START AGAIN ============"));
//
//       // after cleanup try same again
//
//       CreateAllModules(manager, numworkers);
//       ConnectModules(manager, devices % 10);
//       sleep(1);
//
//       StartAll(manager);
//       sleep(1);
//
//       EnableSending(manager, true);
////       CheckLocking();
//
//       dabc::ShowLongSleep("Main loop", 5);
//
//       EnableSending(manager, false);
//       sleep(1);
//       StopAll(manager);
//       CleanupAll(manager);
//   }
//
//   return 0;
//}
//
//*/
//
//class Test2SendModule : public dabc::ModuleAsync {
//   protected:
//      dabc::PoolHandle* fPool;
//      bool                fCanSend;
//      dabc::Ratemeter     fSendRate;
//      int                 fPortCnt;
//
//   public:
//      Test2SendModule(const char* name, dabc::Command* cmd) :
//         dabc::ModuleAsync(name, cmd)
//      {
//         int nports = cmd->GetInt("NumPorts", 3);
//         int buffsize = cmd->GetInt("BufferSize", 16*1024);
//
//         fPool = CreatePoolHandle("SendPool", buffsize, 500);
//
//         for (int n=0;n<nports;n++)
//            CreateOutput(FORMAT(("Output%d", n)), fPool, TestSendQueueSize);
//
//         // CreateTimer("StillAlive", 1.);
//
//         fCanSend = false;
//         fPortCnt = 0;
//
//         DOUT3(( "new TSendModule %s nports = %d buf = %d done", GetName(), NumOutputs(), buffsize));
//      }
//
//      int ExecuteCommand(dabc::Command* cmd)
//      {
//         if (cmd->IsName("EnableSending")) {
//            fCanSend = cmd->GetBool("Enable", true);
//
//            if (fCanSend) {
//               DOUT1(("Can start sending"));
//               for(int n=0;n<TestSendQueueSize;n++)
//                  for(unsigned nport=0;nport<NumOutputs();nport++) {
//                     dabc::Buffer* ref = fPool->TakeBuffer();
////                     DOUT1(("Buf %p size = %d", ref, ref->GetTotalSize()));
//                     Output(nport)->Send(ref);
//                  }
//            }
//            fPortCnt = 0;
//         } else
//            return ModuleAsync::ExecuteCommand(cmd);
//
//         return cmd_true;
//      }
//
//      void ProcessOutputEvent(dabc::Port* port)
//      {
//         if (!fCanSend || (NumOutputs()==0)) return;
//
//         bool fullchaotic = false;
//
//         if (!fullchaotic && (Output(fPortCnt)!=port)) return;
//
//         bool tryagain = true;
//
//         do {
//            tryagain = false;
//            dabc::Buffer* ref = fPool->TakeBuffer();
//            fSendRate.Packet(ref->GetDataSize());
//            port->Send(ref);
//
//            if (!fullchaotic) {
//               fPortCnt = (fPortCnt+1) % NumOutputs();
//               port = Output(fPortCnt);
//               tryagain = port->CanSend();
//            }
//
//         } while (tryagain);
//      }
//
//      virtual void ProcessTimerEvent(dabc::Timer* timer)
//      {
//         DOUT1(("Get timer event %s", timer->GetName()));
//      }
//
//
//      void AfterModuleStop()
//      {
//         DOUT1(("TSendModule finish Rate %5.1f numoper:%7ld", fSendRate.GetRate(), fSendRate.GetNumOper()));
//      }
//};
//
//
//class Test2RecvModule : public dabc::ModuleAsync {
//   protected:
//      dabc::PoolHandle* fPool;
//      int fSleepTime;
//      dabc::Ratemeter fRecvRate;
//      dabc::Average fRealSleepTime;
//
//   public:
//      Test2RecvModule(const char* name, dabc::Command* cmd) :
//         dabc::ModuleAsync(name, cmd)
//      {
//         // we will use queue (second true) in the signal to detect order of signal fire
//         int nports = cmd->GetInt("NumPorts", 3);
//         int buffsize = cmd->GetInt("BufferSize", 16*1024);
//
//         DOUT3(( "new TRecvModule %s %d %d", GetName(), nports, buffsize));
//
//         fPool = CreatePoolHandle("RecvPool", buffsize, 5);
//
//         for (int n=0;n<nports;n++)
//            CreateInput(FORMAT(("Input%d", n)), fPool, TestRecvQueueSize);
//
//         fSleepTime = 0;
//      }
//
//      int ExecuteCommand(dabc::Command* cmd)
//      {
//         if (cmd->IsName("ChangeSleepTime")) {
//            int old = fSleepTime;
//            fSleepTime = cmd->GetInt("SleepTime", 0);
//            DOUT1(("Sleep:%5d (%5.1f, %5.1f) Rate: %5.1f     new:%5d", old, fRealSleepTime.Mean(), fRealSleepTime.Max(), fRecvRate.GetRate(), fSleepTime));
//            fRealSleepTime.Reset();
//            fRecvRate.Reset();
//         } else
//            return ModuleAsync::ExecuteCommand(cmd);
//
//         return cmd_true;
//      }
//
//      void ProcessInputEvent(dabc::Port* port)
//      {
//         dabc::Buffer* ref = port->Recv();
//
//         if (ref==0) {
//            EOUT(("Cannot recv buffer from port %s", port->GetName()));
//            return;
//         }
//
////         DOUT1(("Recieve packet %p %d ", ref, ref->GetTotalSize()));
//
//         fRecvRate.Packet(ref->GetTotalSize());
//         dabc::Buffer::Release(ref);
//
//         if (fSleepTime>0) {
//            dabc::TimeStamp_t tm1 = TimeStamp();
//            dabc::MicroSleep(fSleepTime);
//            dabc::TimeStamp_t tm2 = TimeStamp();
//            fRealSleepTime.Fill(dabc::TimeDistance(tm1, tm2));
//         }
//      }
//
//      void AfterModuleStop()
//      {
//         DOUT1(("TRecvModule finish Rate %5.1f numoper:%7ld", fRecvRate.GetRate(), fRecvRate.GetNumOper()));
//      }
//};
//
//class Test2WorkerModule : public dabc::ModuleSync {
//   protected:
//
//      int fCounter;
//
//   public:
//      Test2WorkerModule(const char* name, dabc::Command* cmd) :
//         dabc::ModuleSync(name, cmd)
//      {
//         fCounter = 0;
//      }
//
//      int ExecuteCommand(dabc::Command* cmd)
//      {
//         if (cmd->IsName("ResetCounter")) {
//            fCounter = 0;
//         } else
//            return ModuleSync::ExecuteCommand(cmd);
//
//         return cmd_true;
//      }
//
//      virtual void MainLoop()
//      {
//         DOUT1(("TWorkerModule mainloop"));
//
//         while (ModuleWorking()) {
//            fCounter++;
//
//            // lets say, about 10 milisecond
//            for (int k=0;k<10000/110;k++) {
//              // this peace of code takes about 110 microsec
//               double sum = 0;
//               for (int n=0;n<10000;n++)
//                 sum+=sqrt(n*1.);
//            }
//         }
//         DOUT1(("TWorkerModule %s finished cnt=%7d tm %4.1f", GetName(), fCounter, fCounter/100.));
//      }
//};
//
//void CreateAllModules2(dabc::StandaloneManager &m, int buffersize, int numworkers = 0)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node<m.NumNodes();node++) {
//      dabc::Command* cmd =
//         new dabc::CmdCreateModule("Test2RecvModule","Receiver");
//      cmd->SetInt("NumPorts", m.NumNodes()-1);
//      cmd->SetInt("BufferSize", buffersize);
//      m.SubmitRemote(cli, cmd, node);
//   }
//
//   for (int node=0;node<m.NumNodes();node++) {
//      dabc::Command* cmd =
//         new dabc::CmdCreateModule("Test2SendModule","Sender");
//      cmd->SetInt("NumPorts", m.NumNodes()-1);
//      cmd->SetInt("BufferSize", buffersize);
//      m.SubmitRemote(cli, cmd, node);
//   }
//
//   for (int nw=0;nw<numworkers;nw++)
//      for (int node=0;node<m.NumNodes();node++) {
//         dabc::Command* cmd =
//            new dabc::CmdCreateModule("Test2WorkerModule", FORMAT(("Worker%d",nw)));
//         m.SubmitRemote(cli, cmd, node);
//      }
//
//   bool res = cli.WaitCommands(5);
//
//   DOUT1(("CreateAllModules() res = %s", DBOOL(res)));
//}
//
//void ConnectModules2(dabc::StandaloneManager &m, int deviceid = 1)
//{
//   dabc::CommandClient cli;
//
//   const char* devname = "Test2Dev";
//   const char* deviceclass = dabc::typeSocketDevice;
//   if (deviceid==2) deviceclass = "verbs::Device";
//
//   for (int node = 0; node < m.NumNodes(); node++)
//      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);
//
//   if (!cli.WaitCommands(5)) {
//      EOUT(("Cannot create devices of class %s", deviceclass));
//      exit(1);
//   }
//
//   for (int nsender = 0; nsender < m.NumNodes(); nsender++) {
//      for (int nreceiver = 0; nreceiver < m.NumNodes(); nreceiver++) {
//          if (nsender==nreceiver) continue;
//
//          std::string port1name, port2name;
//
//          dabc::formats(port1name, "%s$Sender/Output%d", m.GetNodeName(nsender), nreceiver>nsender ? nreceiver-1 : nreceiver);
//          dabc::formats(port2name, "%s$Receiver/Input%d", m.GetNodeName(nreceiver), nsender>nreceiver ? nsender-1 : nsender);
//
//          dabc::Command* cmd =
//             new dabc::CmdConnectPorts(port1name.c_str(),
//                                          port2name.c_str(),
//                                          devname);
//
//          cmd->SetBool(dabc::xmlUseAcknowledge, TestUseAckn);
//
//          m.SubmitCl(cli, cmd);
//      }
//   }
//   bool res = cli.WaitCommands(5);
//   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));
//}
//
//void SetPriorities2(dabc::StandaloneManager &m, int prio = 0)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node<m.NumNodes();node++) {
//      dabc::Command* cmd = new dabc::Command("SetPriority");
//      cmd->SetInt("Priority",prio);
//      m.SubmitRemote(cli, cmd, node, "Receiver");
//   }
//   for (int node=0;node<m.NumNodes();node++) {
//      dabc::Command* cmd = new dabc::Command("SetPriority");
//      cmd->SetInt("Priority",prio);
//      m.SubmitRemote(cli, cmd, node, "Sender");
//   }
//
//   bool res = cli.WaitCommands(1);
//   DOUT1(("SetPriorities res = %s", DBOOL(res)));
//}
//
//void StartAll2(dabc::StandaloneManager &m, int numworkers = 0)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node<m.NumNodes();node++)
//      m.SubmitRemote(cli, new dabc::CmdStartAllModules(), node);
//
//   bool res = cli.WaitCommands(1);
//   DOUT1(("StartAll() res = %s", DBOOL(res)));
//}
//
//void EnableSending2(dabc::StandaloneManager &m, bool on = true, int maxnode = -1)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node< (maxnode>=0 ? maxnode : m.NumNodes()); node++) {
//      dabc::Command* cmd = new dabc::Command("EnableSending");
//      cmd->SetBool("Enable", on);
//      m.SubmitRemote(cli, cmd, node, "Sender");
//   }
//   cli.WaitCommands(1);
//}
//
//void ChangeSleepTime2(dabc::StandaloneManager &m, int tm=0, int selectnode = -1)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node<m.NumNodes();node++) {
//      if ((selectnode>=0) && (node!=selectnode)) continue;
//      dabc::Command* cmd = new dabc::Command("ChangeSleepTime");
//      cmd->SetInt("SleepTime", tm);
//      m.SubmitRemote(cli, cmd, node, "Receiver");
//   }
//   cli.WaitCommands(1);
//}
//
//void StopAll2(dabc::StandaloneManager &m)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node<m.NumNodes();node++)
//      m.SubmitRemote(cli, new dabc::CmdStopAllModules(), node);
//   bool res = cli.WaitCommands(5);
//   DOUT1(("StopAll res = %s", DBOOL(res)));
//}
//
//void CleanupAll2(dabc::StandaloneManager &m)
//{
//   dabc::CommandClient cli;
//
//   for (int node=0;node<m.NumNodes();node++)
//      m.SubmitRemote(cli, new dabc::CmdCleanupManager(), node);
//   bool res = cli.WaitCommands(5);
//   DOUT1(("CleanupAll res = %s", DBOOL(res)));
//}
//
//void RunStandaloneTest(dabc::StandaloneManager &m, int nmodules = 1)
//{
//   DOUT1(("STANDALONE TEST N = %d", nmodules));
//
//   dabc::CommandClient cli;
//
//   const char* thrdname = 0; // "CommonThread";
//
//   for (int nm=0;nm<nmodules;nm++) {
//      dabc::Command* cmd =
//         new dabc::CmdCreateModule("TRecvModule", FORMAT(("Receiver%d",nm)), thrdname);
//      cmd->SetInt("NumPorts", nmodules);
//      cmd->SetInt("BufferSize", TestBufferSize);
//      m.SubmitCl(cli, cmd);
//   }
//
//   for (int nm=0;nm<nmodules;nm++) {
//      dabc::Command* cmd =
//         new dabc::CmdCreateModule("TSendModule",FORMAT(("Sender%d",nm)), thrdname);
//      cmd->SetInt("NumPorts", nmodules);
//      cmd->SetInt("BufferSize", TestBufferSize);
//      m.SubmitCl(cli, cmd);
//   }
//
//   for (int nm1=0;nm1<nmodules;nm1++)
//      for (int nm2=0;nm2<nmodules;nm2++)
//         m.SubmitCl(cli,
//            new dabc::CmdConnectPorts(
//                  FORMAT(("Sender%d/Output%d", nm1, nm2)),
//                  FORMAT(("Receiver%d/Input%d", nm2, nm1))));
//   cli.WaitCommands(1);
//
//   m.SubmitCl(cli, new dabc::CmdStartAllModules());
//   cli.WaitCommands(1);
//
//   for (int nm=0;nm<nmodules;nm++) {
//      dabc::Command* cmd = new dabc::Command("EnableSending");
//      cmd->SetBool("Enable", true);
//      m.SubmitLocal(cli, cmd, FORMAT(("Sender%d",nm)));
//   }
//   cli.WaitCommands(1);
//
//   sleep(3);
//
//   for (int nm=0;nm<nmodules;nm++) {
//      dabc::Command* cmd = new dabc::Command("EnableSending");
//      cmd->SetBool("Enable", false);
//      m.SubmitLocal(cli, cmd, FORMAT(("Sender%d",nm)));
//   }
//   cli.WaitCommands(1);
//
//   m.SubmitCl(cli, new dabc::CmdStopAllModules());
//   cli.WaitCommands(1);
//
//   m.SubmitCl(cli, new dabc::CmdCleanupManager());
//   cli.WaitCommands(5);
//}
//
//void AllToAllLoop2(dabc::StandaloneManager &manager, int device, int buffersize, int numworkers)
//{
//   long arena1 = dabc::GetProcVirtMem();
//   DOUT1(("AllToAllLoop Start with: %ld",arena1));
//
//   dabc::CpuStatistic cpu;
//
//   CreateAllModules2(manager, buffersize, numworkers);
//
//   ConnectModules2(manager, device);
//
//   sleep(1);
//
//   // dabc::Iterator::PrintHieararchy(&manager);
//   StartAll2(manager);
//   sleep(1);
//   EnableSending2(manager, true);
//   cpu.Reset();
//   sleep(10);
//   cpu.Measure();
//   EnableSending2(manager, false);
//   sleep(1);
//   DOUT1(("AllToAllLoop Works with: %ld  CPU: %5.1f", dabc::GetProcVirtMem(), cpu.CPUutil()*100.));
//   StopAll2(manager);
//
//   CleanupAll2(manager);
//   sleep(2);
//
//   long arena2 = dabc::GetProcVirtMem();
//   DOUT1(("CleanupAll  DIFF = %ld", arena2-arena1));
//
//}
//
//void OneToAllLoop2(dabc::StandaloneManager &m, int deviceid)
//{
//   dabc::CommandClient cli;
//
//   long arena1 = dabc::GetProcVirtMem();
//   DOUT1(("OneToAllLoop Start with: %ld",arena1));
//
//   dabc::Command* cmd = 0;
//
//   for (int node=1;node<m.NumNodes();node++) {
//      cmd = new dabc::CmdCreateModule("Test2RecvModule","Receiver");
//      cmd->SetInt("NumPorts", 1);
//      cmd->SetInt("BufferSize", TestBufferSize);
//      m.SubmitRemote(cli, cmd, node);
//   }
//
//   cmd = new dabc::CmdCreateModule("Test2SendModule","Sender");
//   cmd->SetInt("NumPorts", m.NumNodes()-1);
//   cmd->SetInt("BufferSize", TestBufferSize);
//   m.SubmitLocal(cli, cmd);
//
//   bool res = cli.WaitCommands(5);
//
//   DOUT1(("CreateAllModules() res = %s", DBOOL(res)));
//
//   const char* devname = "Test2Dev";
//   const char* deviceclass = dabc::typeSocketDevice;
//   if (deviceid==2) deviceclass = "verbs::Device";
//
//   for (int node = 0; node < m.NumNodes(); node++)
//      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);
//
//   if (!cli.WaitCommands(5)) {
//      EOUT(("Cannot create devices of class %s", deviceclass));
//      exit(1);
//   }
//
//   for (int nreceiver = 1; nreceiver < m.NumNodes(); nreceiver++) {
//      std::string port1name, port2name;
//
//      dabc::formats(port1name, "%s$Sender/Output%d", m.GetNodeName(0), nreceiver-1);
//      dabc::formats(port2name, "%s$Receiver/Input0", m.GetNodeName(nreceiver));
//
//      cmd = new dabc::CmdConnectPorts(port1name.c_str(),
//                                         port2name.c_str(),
//                                         devname);
//
//      m.SubmitCl(cli, cmd);
//   }
//   res = cli.WaitCommands(5);
//   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));
//   sleep(1);
//
//   StartAll2(m);
//   sleep(1);
//   EnableSending2(m, true, 1);
//   sleep(5);
//   EnableSending2(m, false, 1);
//   sleep(1);
//   StopAll2(m);
//
//   CleanupAll2(m);
//   sleep(2);
//
//   long arena2 = dabc::GetProcVirtMem();
//   DOUT1(("CleanupAll  DIFF = %ld", arena2-arena1));
//}
//
///*
//
//void TimeSyncLoop2(dabc::StandaloneManager &m, int deviceid)
//{
//   dabc::CommandClient cli;
//
//   const char* devname = "TSyncDev";
//   const char* deviceclass = dabc::typeSocketDevice;
//   const char* thrdclass = dabc::typeSocketThread;
//   if (deviceid==2) {
//       deviceclass = "verbs::Device";
//       thrdclass = "VerbsThread";
//   }
//
//   for (int node = 0; node < m.NumNodes(); node++) {
//      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);
//      m.SubmitRemote(cli, new dabc::CmdCreateThread("TSyncThrd", thrdclass, devname), node);
//   }
//
//   if (!cli.WaitCommands(5)) {
//      EOUT(("Cannot create devices of class %s", deviceclass));
//      exit(1);
//   }
//
//   dabc::Command* cmd = 0;
//
//   for (int node=0;node<m.NumNodes();node++) {
//      cmd = new dabc::CmdCreateModule("dabc::TimeSyncModule","TSync", "TSyncThrd");
//
//      if (node==0)
//         cmd->SetInt("NumSlaves", m.NumNodes()-1);
//      else
//         cmd->SetBool("MasterConn", true);
//      m.SubmitRemote(cli, cmd, node);
//   }
//
//   bool res = cli.WaitCommands(5);
//
//   DOUT1(("Create TimeSync modules() res = %s", DBOOL(res)));
//
//   for (int nslave = 1; nslave < m.NumNodes(); nslave++) {
//      std::string port1name, port2name;
//
//      dabc::formats(port1name, "%s$TSync/Slave%d", m.GetNodeName(0), nslave-1);
//      dabc::formats(port2name, "%s$TSync/Master", m.GetNodeName(nslave));
//
//      cmd = new dabc::CmdConnectPorts(port1name.c_str(),
//                                         port2name.c_str(),
//                                         devname, "TSyncThrd");
//
//      m.SubmitCl(cli, cmd);
//   }
//   res = cli.WaitCommands(5);
//   DOUT1(("Connect TimeSync Modules() res = %s", DBOOL(res)));
//
//   StartAll(m);
//   sleep(1);
//
//   cmd = new dabc::CommandDoTimeSync(false, 100, true, false);
//   m.SubmitLocal(cli, cmd, "TSync");
//   res = cli.WaitCommands(5);
//
//   dabc::ShowLongSleep("First pause", 5);
//
//   cmd = new dabc::CommandDoTimeSync(false, 100, true, true);
//   m.SubmitLocal(cli, cmd, "TSync");
//   res = cli.WaitCommands(5);
//
//   dabc::ShowLongSleep("Second pause", 10);
//
//   cmd = new dabc::CommandDoTimeSync(false, 100, false, false);
//   m.SubmitLocal(cli, cmd, "TSync");
//   res = cli.WaitCommands(5);
//
//   sleep(1);
//   StopAll2(m);
//
//   CleanupAll2(m);
//   sleep(2);
//}
//
//*/
//
///*
//
//int main(int numc, char* args[])
//{
//   dabc::SetDebugLevel(1);
//
//
//   int nodeid = 0;
//   int numnodes = 4;
//   int devices = 11; // only sockets
//   const char* controllerID = "file.txt";
//   if (numc>1) nodeid = atoi(args[1]);
//   if (numc>2) numnodes = atoi(args[2]);
//   if (numc>3) devices = atoi(args[3]);
//   if (numc>4) controllerID = args[4];
//
//   dabc::SetDebugLevel(1);
//
//   dabc::StandaloneManager manager(0, nodeid, numnodes, false);
//   new Test2Plugin();
//
//   if (numc<2) {
//      RunStandaloneTest(manager, 5);
//
//      return 0;
//   }
//
//   manager.ConnectCmdChannel(numnodes, devices / 10, controllerID);
//
//   DOUT1(("READY"));
//
//   if (nodeid==0) {
////       dabc::SetDebugLevel(3);
//
//       sleep(1);
//
//       int bufsize = 4*1024;
//
//       for (int cnt=0;cnt<4;cnt++) {
//
////          CheckNewDelete();
//
//          DOUT1(("\n\n\n =============== START AGAIN %d ============", cnt));
//
//          DOUT1(("BufSize %d Basic %ld Transports %ld",
//             bufsize,
//             dabc::Basic::NumInstances(),
//             dabc::Transport::NumTransports()));
//
////          AllToAllLoop(manager, devices % 10, bufsize, 0);
//
////          OneToAllLoop(manager, devices % 10);
//
//          TimeSyncLoop(manager, devices % 10);
//          bufsize *= 2;
//       }
//
//       // after cleanup try same again
////       AllToAllLoop(manager, devices % 10, 0);
//
//   }
//
//   return 0;
//}
//
//*/
//

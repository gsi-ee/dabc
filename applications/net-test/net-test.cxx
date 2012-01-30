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
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/threads.h"
#include "dabc/Application.h"
#include "dabc/SocketDevice.h"
#include "dabc/statistic.h"
#include "dabc/Factory.h"
#include "dabc/Configuration.h"
#include "dabc/CommandsSet.h"
#include "dabc/Url.h"

int TestBufferSize = 8*1024;
int TestSendQueueSize = 5;
int TestRecvQueueSize = 10;
bool TestUseAckn = false;

class NetTestSenderModule : public dabc::ModuleAsync {
   protected:
      bool                fCanSend;
      unsigned            fPortCnt;
      bool                fChaotic;

   public:
      NetTestSenderModule(const char* name, dabc::Command cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         int nports = Cfg("NumPorts",cmd).AsInt(3);
         int buffsize = Cfg("BufferSize",cmd).AsInt(16*1024);
         fChaotic = Cfg("Chaotic", cmd).AsBool(true);

         CreatePoolHandle("SendPool");

         dabc::Parameter rate = CreatePar("OutRate");
         rate.SetRatemeter(false, 3.);
         rate.SetLimits(0.,100.);
         rate.SetUnits("MB");
         //rate.SetDebugOutput(true);

         for (int n=0;n<nports;n++) {
            dabc::Port* port = CreateOutput(FORMAT(("Output%d", n)), Pool(), TestSendQueueSize);
            port->SetOutRateMeter(rate);
         }

         fCanSend = Cfg("CanSend",cmd).AsBool(false);
         fPortCnt = 0;

         DOUT1(("new TSendModule %s nports = %d buf = %d done", GetName(), NumOutputs(), buffsize));
      }

      ~NetTestSenderModule()
      {
         DOUT3(("~NetTestSenderModule() %s", GetName()));
      }

      int ExecuteCommand(dabc::Command cmd)
      {
         if (cmd.IsName("EnableSending")) {
            fCanSend = cmd.GetBool("Enable", true);
            if (fCanSend) StartSending();
            fPortCnt = 0;
            return dabc::cmd_true;
         }

         return ModuleAsync::ExecuteCommand(cmd);
      }

      void ProcessOutputEvent(dabc::Port* port)
      {
//         DOUT0(("Module %s Process output event port %s cansend %s", GetName(), port->GetName(), DBOOL(fCanSend)));

         if (!fCanSend) return;

         if (fChaotic) {
            dabc::Buffer buf = Pool()->TakeBuffer();
  //          DOUT0(("Module %s Send buffer %p", GetName(), buf));
            port->Send(buf);
            return;
         }

         int cnt = 1000;

         while (Output(fPortCnt)->CanSend()) {
            dabc::Buffer buf = Pool()->TakeBuffer();
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
                  dabc::Buffer buf = Pool()->TakeBuffer();
                  if (buf.null()) { EOUT(("no buffers in memory pool")); }
                  Output(nout)->Send(buf);
               }
      }

      void BeforeModuleStart()
      {
         DOUT2(("SenderModule starting"));
      }


      void AfterModuleStop()
      {
         DOUT0(("SenderModule finish Rate %s", Par("OutRate").AsStr()));
      }
};


class NetTestReceiverModule : public dabc::ModuleAsync {
   protected:
      int                  fSleepTime;

   public:
      NetTestReceiverModule(const char* name, dabc::Command cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         // we will use queue (second true) in the signal to detect order of signal fire
         int nports = Cfg("NumPorts",cmd).AsInt(3);
         int buffsize = Cfg("BufferSize",cmd).AsInt(16*1024);
         fSleepTime = Cfg("SleepTime",cmd).AsInt(0);

         CreatePoolHandle("RecvPool");

         dabc::Parameter par = CreatePar("InpRate");
         par.SetRatemeter(false, 3.);
         par.SetLimits(0., 100.);
         par.SetUnits("MB");
         par.SetDebugOutput(true);

         for (int n=0;n<nports;n++) {
            dabc::Port* port = CreateInput(FORMAT(("Input%d", n)), Pool(), TestRecvQueueSize);
            port->SetInpRateMeter(par);
         }

         DOUT1(("new TRecvModule %s nports:%d buf:%d", GetName(), nports, buffsize));

         fSleepTime = 0;
      }

      virtual ~NetTestReceiverModule()
      {
          DOUT3(("Calling ~NetTestReceiverModule"));
      }

      int ExecuteCommand(dabc::Command cmd)
      {
         if (cmd.IsName("ChangeSleepTime")) {
            fSleepTime = cmd.GetInt("SleepTime", 0);
         } else
            return ModuleAsync::ExecuteCommand(cmd);

         return dabc::cmd_true;
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer ref = port->Recv();

//         DOUT0(("******************************************* Module %s Port %s Recv buffer %p", GetName(), port->GetName(), ref));

//         exit(076);

         if (ref.null()) { EOUT(("AAAAAAA type = %u", ref.GetTypeId())); }

         ref.Release();

         if (fSleepTime>0) {
//            dabc::TimeStamp tm1 = dabc::Now();
            WorkerSleep(fSleepTime);
//            dabc::TimeStamp tm2 = dabc::Now();
         }
      }

      void BeforeModuleStart()
      {
         DOUT2(("ReceiverModule starting"));
      }

      void AfterModuleStop()
      {
         DOUT0(("ReceiverModule finish Rate %s", Par("InpRate").AsStr()));
      }
};

class NetTestMcastModule : public dabc::ModuleAsync {
   protected:
      bool fReceiver;
      int  fCounter;

   public:
      NetTestMcastModule(const char* name, dabc::Command cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         fReceiver = Cfg("IsReceiver",cmd).AsBool(true);

         CreatePoolHandle("MPool");

         fCounter = 0;

         CreatePar("DataRate").SetRatemeter(false, 3.);

         if (fReceiver)
            CreateInput("Input", Pool())->SetInpRateMeter(Par("DataRate"));
         else {
            CreateOutput("Output", Pool())->SetOutRateMeter(Par("DataRate"));
//            CreateTimer("Timer1", 0.1);
         }
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer buf = port->Recv();
         if (buf.null()) { EOUT(("AAAAAAA")); return; }

//         DOUT0(("Process input event %d", buf->GetDataSize()));

//         DOUT0(("Counter %3d Size %u Msg %s", fCounter++, buf->GetDataSize(), buf->GetDataLocation()));
         buf.Release();
      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         dabc::Buffer buf = Pool()->TakeBuffer();
         if (buf.null()) { EOUT(("AAAAAAA")); return; }

         buf.SetTotalSize(1000);
//         DOUT0(("Process output event %d", buf->GetDataSize()));
         port->Send(buf);
      }

      void ProcessTimerEvent(dabc::Timer* timer)
      {
         if (!Output()->CanSend()) return;
         dabc::Buffer buf = Pool()->TakeBuffer(1024);
         if (buf.null()) return;

         sprintf((char*)buf.GetPointer().ptr(),"Message %3d from sender", fCounter++);

         buf.SetTotalSize(strlen((char*)buf.GetPointer().ptr())+1);

         DOUT0(("Sending %3d Size %u Msg %s", fCounter, buf.GetTotalSize(), buf.GetPointer().ptr()));

         Output()->Send(buf);
      }

      void AfterModuleStop()
      {
         DOUT0(("Mcast module rate %f", Par("DataRate").AsDouble()));
      }
};

class NetTestApplication : public dabc::Application {
   public:

      enum EKinds { kindAllToAll, kindMulticast };

      NetTestApplication() : dabc::Application("NetTestApp")
      {
         DOUT0(("Create net test application"));


         CreatePar("Kind").DfltStr("all-to-all");

         CreatePar("NetDevice").DfltStr(dabc::typeSocketDevice);

         CreatePar(dabc::xmlMcastAddr).DfltStr("224.0.0.15");
         CreatePar(dabc::xmlMcastPort).DfltInt(7234);

         CreatePar(dabc::xmlBufferSize).DfltInt(1024);
         CreatePar(dabc::xmlOutputQueueSize).DfltInt(4);
         CreatePar(dabc::xmlInputQueueSize).DfltInt(8);
         CreatePar(dabc::xmlUseAcknowledge).DfltBool(false);

         DOUT0(("Test application was build"));
      }

      int Kind()
      {
         if (Par("Kind").AsStdStr() == "all-to-all") return kindAllToAll;
         if (Par("Kind").AsStdStr() == "multicast") return kindMulticast;
         return kindAllToAll;
      }

      virtual bool CreateAppModules()
      {
         std::string devclass = Par("NetDevice").AsStdStr();

         if (Kind() == kindAllToAll) {

            if (!dabc::mgr.CreateDevice(devclass.c_str(), "NetDev")) return false;

            // no need, will be done when modules are created
            // dabc::mgr.CreateMemoryPool("SendPool");

            // no need, will be done when modules are created
            // dabc::mgr.CreateMemoryPool("RecvPool");

            dabc::CmdCreateModule cmd1("NetTestReceiverModule", "Receiver");
            cmd1.SetInt("NumPorts", dabc::mgr()->NumNodes());
            if (!dabc::mgr.Execute(cmd1)) return false;

            dabc::CmdCreateModule cmd2("NetTestSenderModule", "Sender");
            cmd2.SetInt("NumPorts", dabc::mgr()->NumNodes());
            cmd2.SetBool("CanSend", true);
            if (!dabc::mgr.Execute(cmd2)) return false;

            for (unsigned nsender = 0; nsender < NumNodes(); nsender++) {
               for (unsigned nreceiver = 0; nreceiver < NumNodes(); nreceiver++) {
                  if (nsender == nreceiver) continue;

                   std::string port1 = dabc::Url::ComposePortName(nsender, "Sender/Output", nreceiver);

                   std::string port2 = dabc::Url::ComposePortName(nreceiver, "Receiver/Input", nsender);

                   dabc::mgr.Connect(port1, port2);
               }
            }

            DOUT0(("Create all-to-all modules done"));

            return true;


         } else
         if (Kind() == kindMulticast) {
            bool isrecv = dabc::mgr()->NodeId() > 0;

            DOUT1(("Create device %s", devclass.c_str()));

            if (!dabc::mgr.CreateDevice(devclass.c_str(), "MDev")) return false;

            dabc::CmdCreateModule cmd3("NetTestMcastModule", "MM");
            cmd3.SetBool("IsReceiver", isrecv);
            if (!dabc::mgr.Execute(cmd3)) return false;

            if (!dabc::mgr.CreateTransport(isrecv ? "MM/Input" : "MM/Output", "MDev")) return false;

            DOUT1(("Create multicast modules done"));

            return true;
         }

         return false;
      }

      virtual bool BeforeAppModulesStarted()
      {
         return true;
      }

};



class NetTestFactory : public dabc::Factory  {
   public:

      NetTestFactory(const char* name) : dabc::Factory(name) {}

      virtual dabc::Application* CreateApplication(const char* classname, dabc::Command cmd)
      {
         if (strcmp(classname, "NetTestApp")==0)
            return new NetTestApplication();
         return dabc::Factory::CreateApplication(classname, cmd);
      }


      virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
      {
         if (strcmp(classname,"NetTestSenderModule")==0)
            return new NetTestSenderModule(modulename, cmd);
         else
         if (strcmp(classname,"NetTestReceiverModule")==0)
            return new NetTestReceiverModule(modulename, cmd);
         else
         if (strcmp(classname,"NetTestMcastModule")==0)
            return new NetTestMcastModule(modulename, cmd);
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



bool StartStopAll(bool isstart)
{
   dabc::CommandsSet cli(dabc::mgr()->thread());

   for (int node=0;node<dabc::mgr()->NumNodes();node++) {
      dabc::Command cmd;
      if (isstart) cmd = dabc::CmdStartModule("*");
              else cmd = dabc::CmdStopModule("*");
      cmd.SetReceiver(node);
      cli.Add(cmd, dabc::mgr());
   }

   int res = cli.ExecuteSet(3);
   DOUT0(("%s all res = %s", (isstart ? "Start" : "Stop"), DBOOL(res)));

   return res;
}

bool EnableSending(bool on = true)
{
   dabc::CommandsSet cli(dabc::mgr()->thread());

   for (int node=0;node<dabc::mgr()->NumNodes();node++) {
      dabc::Command cmd("EnableSending");
      cmd.SetBool("Enable", on);
      cmd.SetReceiver(node, "Sender");

      cli.Add(cmd, dabc::mgr());
   }

   return cli.ExecuteSet(3) == dabc::cmd_true;
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

   dabc::CommandsSet cli(dabc::mgr()->thread());

   for (int node=0;node<numnodes;node++) {
      dabc::CmdCreateModule cmd("NetTestReceiverModule","Receiver");
      cmd.SetInt("NumPorts", numnodes-1);
      cmd.SetInt("BufferSize", TestBufferSize);
      cmd.SetReceiver(node);

      cli.Add(cmd, dabc::mgr());
   }

   for (int node=0;node<numnodes;node++) {
      dabc::CmdCreateModule cmd("NetTestSenderModule","Sender");
      cmd.SetInt("NumPorts", numnodes-1);
      cmd.SetInt("BufferSize", TestBufferSize);
      cmd.SetReceiver(node);

      cli.Add(cmd, dabc::mgr());
   }

   res = cli.ExecuteSet(5);

   DOUT0(("CreateAllModules() res = %s", DBOOL(res)));

   cli.Cleanup();

   const char* devname = "Test1Dev";

   for (int node = 0; node < numnodes; node++) {
      dabc::CmdCreateDevice cmd(devclass.c_str(), devname);
      cmd.SetReceiver(node);
      cli.Add(cmd, dabc::mgr());
   }

   if (!cli.ExecuteSet(5)) {
      EOUT(("Cannot create devices of class %s", devclass.c_str()));
      return;
   }

   cli.Cleanup();

   for (int nsender = 0; nsender < numnodes; nsender++) {
      for (int nreceiver = 0; nreceiver < numnodes; nreceiver++) {
          if (nsender==nreceiver) continue;

          std::string port1 = dabc::Url::ComposePortName(nsender, "Sender/Output", (nreceiver>nsender ? nreceiver-1 : nreceiver));

          std::string port2 = dabc::Url::ComposePortName(nreceiver, "Receiver/Input", (nsender>nreceiver ? nsender-1 : nsender));

          dabc::ConnectionRequest req = dabc::mgr.Connect(port1, port2);

          req.SetUseAckn(TestUseAckn);
          req.SetConnDevice(devname);
          req.SetConnThread("TransportThrd");
      }
   }

   res = dabc::mgr.ActivateConnections(10.);
   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));

   StartStopAll(true);

   EnableSending(true);

   dabc::mgr()->Sleep(10, "Waiting");

   StartStopAll(false);
}

extern "C" void RunMulticastTest()
{
   std::string devclass = dabc::mgr()->cfg()->GetUserPar("Device", dabc::typeSocketDevice);

   std::string mcast_host = dabc::mgr()->cfg()->GetUserPar(dabc::xmlMcastAddr, "224.0.0.15");
   int mcast_port = dabc::mgr()->cfg()->GetUserParInt(dabc::xmlMcastPort, 7234);
   bool isrecv = dabc::mgr()->NodeId() > 0;

   DOUT1(("Create device %s", devclass.c_str()));

   if (!dabc::mgr.CreateDevice(devclass.c_str(), "MDev")) return;

   dabc::CmdCreateModule cmd("NetTestMcastModule", "MM");
   cmd.SetBool("IsReceiver", isrecv);
   dabc::mgr.Execute(cmd);

   dabc::CmdCreateTransport cmd2(isrecv ? "MM/Input" : "MM/Output", "MDev");
   cmd2.SetStr(dabc::xmlMcastAddr, mcast_host);
   cmd2.SetInt(dabc::xmlMcastPort, mcast_port);
   dabc::mgr.Execute(cmd2);

   DOUT1(("Create transport for addr %s", mcast_host.c_str()));

   dabc::mgr.StartAllModules();

   dabc::mgr()->Sleep(5);

   dabc::mgr.StopAllModules();

   DOUT0(("Multicast test done"));
}

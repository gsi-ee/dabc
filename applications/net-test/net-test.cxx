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
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/threads.h"
#include "dabc/Application.h"
#include "dabc/SocketDevice.h"
#include "dabc/statistic.h"
#include "dabc/Factory.h"
#include "dabc/Configuration.h"
#include "dabc/CommandsSet.h"
#include "dabc/Url.h"
#include "dabc/Pointer.h"

class NetTestSenderModule : public dabc::ModuleAsync {
   protected:
      std::string         fKind;
      unsigned            fSendCnt;
      unsigned            fIgnoreNode;

   public:
      NetTestSenderModule(const std::string& name, dabc::Command cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         fKind = Cfg("Kind", cmd).AsStdStr();

         fSendCnt =  dabc::mgr.NodeId();

         fIgnoreNode = dabc::mgr.NodeId();

         if (IsCmdTest())
            CreatePar("CmdExeTime").SetAverage(false, 2);

         DOUT2("new NetTestSenderModule %s numout = %d ignore %u done", GetName(), NumOutputs(), fIgnoreNode);
//         for(unsigned n=0;n<NumOutputs();n++)
//            DOUT1("   Output %s capacity %u", OutputName(n).c_str(), OutputQueueCapacity(n));
      }

      ~NetTestSenderModule()
      {
         DOUT0("#### ~NetTestSenderModule()");
      }

      bool IsCmdTest() const { return fKind == "cmd-test"; }

      bool IsChaoticTest() const { return fKind == "chaotic"; }

      bool IsRegularTest() const { return fKind == "regular"; }

      int ExecuteCommand(dabc::Command cmd)
      {
         return ModuleAsync::ExecuteCommand(cmd);
      }

      virtual bool ProcessSend(unsigned nout)
      {
//         DOUT1("Process output event");

         if (IsChaoticTest()) {

            if (nout==fIgnoreNode) return false;

            dabc::Buffer buf = TakeBuffer();
            if (buf.null()) return false;
            Send(nout, buf);
            return true;
         }

         if (IsRegularTest()) {

            if (fSendCnt!=nout) return false;

            dabc::Buffer buf = TakeBuffer();
            if (buf.null()) return false;
            Send(nout, buf);

            do {
               fSendCnt = (fSendCnt+1) % dabc::mgr.NumNodes();
            } while (fSendCnt == fIgnoreNode);

            ActivateOutput(fSendCnt);

            return false;
         }

         return false;
      }

      void BeforeModuleStart()
      {
         DOUT0("NetTestSenderModule starting");

         if (IsCmdTest()) {
            DOUT1("Start command test");
            SendNextCommand();
         }
      }

      void SendNextCommand()
      {
         dabc::Command cmd("Test");
         fSendCnt = (fSendCnt + 1) % dabc::mgr.NumNodes();
         cmd.SetReceiver(dabc::Url::ComposeItemName(fSendCnt, "Receiver"));
         cmd.SetTimeout(1);

         dabc::mgr.Submit(Assign(cmd));
      }

      virtual bool ReplyCommand(dabc::Command cmd)
      {
         double tmout = cmd.TimeTillTimeout();
         if (fSendCnt != (unsigned) dabc::mgr.NodeId())
            Par("CmdExeTime").SetDouble((1.- tmout)*1000.);

         //DOUT0("********************** Get command reply res = %d! ************************* ", cmd.GetResult());
         //dabc::mgr.StopApplication();

         if (cmd.GetResult() == dabc::cmd_true) SendNextCommand();
         return true;
      }

      void AfterModuleStop()
      {
         DOUT2("SenderModule finish");
      }
};


class NetTestReceiverModule : public dabc::ModuleAsync {
   protected:
      int                  fSleepTime;

   public:
      NetTestReceiverModule(const std::string& name, dabc::Command cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         // we will use queue (second true) in the signal to detect order of signal fire
         fSleepTime = Cfg("SleepTime",cmd).AsInt(0);
      }

      virtual ~NetTestReceiverModule()
      {
          DOUT0("#### ~NetTestReceiverModule");
      }

      int ExecuteCommand(dabc::Command cmd)
      {
         if (cmd.IsName("ChangeSleepTime")) {
            fSleepTime = cmd.GetInt("SleepTime", 0);
         } else
         if (cmd.IsName("Test")) {
            // DOUT0("---------------- Executing test command ---------------");
            return dabc::cmd_true;
         }

         return ModuleAsync::ExecuteCommand(cmd);
      }

      virtual bool ProcessRecv(unsigned port)
      {
         dabc::Buffer buf = Recv(port);

//         DOUT0("Module %s Port %u Recv buffer %u", GetName(), port, buf.GetTotalSize());

//         exit(076);

         if (buf.null()) { EOUT("AAAAAAA type = %u", buf.GetTypeId()); }

         buf.Release();

//         if (fSleepTime>0) {
//            WorkerSleep(fSleepTime);
//         }

         return true;
      }

      void BeforeModuleStart()
      {
         DOUT2("ReceiverModule starting");
      }

      void AfterModuleStop()
      {
         DOUT2("ReceiverModule finish");
      }
};

class NetTestSpecialModule : public dabc::ModuleAsync {
   protected:
      bool fReceiver;
      int fCnt;

   public:
      NetTestSpecialModule(const std::string& name, dabc::Command cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         fReceiver = dabc::mgr.NodeId() < dabc::mgr.NumNodes() - 1;

         EnsurePorts(fReceiver ? 1 : 0, fReceiver ? 0 : 1);

         fCnt = 0;
      }

      bool ProcessRecv(unsigned port)
      {
         dabc::Buffer buf = Recv(port);
         if (buf.null()) { EOUT("no buffer is received"); return false; }
         buf.Release();
         fCnt++;
         return true;
      }

      bool ProcessSend(unsigned port)
      {
         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) { EOUT("no free buffer"); return false; }
         // buf.SetTotalSize(1000);
         Send(port, buf);
         fCnt++;
         return true;
      }

      void AfterModuleStop()
      {
         DOUT1("Total num buffers %d", fCnt);
      }
};

class NetTestApplication : public dabc::Application {
   public:

      enum EKinds { kindAllToAll, kindMulticast };

      NetTestApplication() : dabc::Application("NetTestApp")
      {
         DOUT0("Create net test application");


         CreatePar("Kind").DfltStr("all-to-all");

         CreatePar("NetDevice").DfltStr(dabc::typeSocketDevice);

         CreatePar(dabc::xmlMcastAddr).DfltStr("224.0.0.15");
         CreatePar(dabc::xmlMcastPort).DfltInt(7234);

         CreatePar(dabc::xmlBufferSize).DfltInt(1024);
         CreatePar(dabc::xmlOutputQueueSize).DfltInt(4);
         CreatePar(dabc::xmlInputQueueSize).DfltInt(8);
         CreatePar(dabc::xmlUseAcknowledge).DfltBool(false);

         DOUT0("Test application was build");
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

            if (!dabc::mgr.CreateDevice(devclass, "NetDev")) return false;

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

            DOUT0("Create all-to-all modules done");

            return true;


         } else
         if (Kind() == kindMulticast) {
            bool isrecv = dabc::mgr()->NodeId() > 0;

            DOUT1("Create device %s", devclass.c_str());

            if (!dabc::mgr.CreateDevice(devclass, "MDev")) return false;

            dabc::CmdCreateModule cmd3("NetTestSpecialModule", "MM");
            cmd3.SetBool("IsReceiver", isrecv);
            if (!dabc::mgr.Execute(cmd3)) return false;

            if (!dabc::mgr.CreateTransport(isrecv ? "MM/Input" : "MM/Output", "MDev")) return false;

            DOUT1("Create multicast modules done");

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

      NetTestFactory(const std::string& name) : dabc::Factory(name) {}

      virtual dabc::Application* CreateApplication(const std::string& classname, dabc::Command cmd)
      {
         if (classname == "NetTestApp")
            return new NetTestApplication();
         return dabc::Factory::CreateApplication(classname, cmd);
      }


      virtual dabc::Module* CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
      {
         if (classname == "NetTestSenderModule")
            return new NetTestSenderModule(modulename, cmd);

         if (classname == "NetTestReceiverModule")
            return new NetTestReceiverModule(modulename, cmd);

         if (classname == "NetTestSpecialModule")
            return new NetTestSpecialModule(modulename, cmd);

         return 0;
      }
};

dabc::FactoryPlugin nettest(new NetTestFactory("net-test"));


extern "C" void RunMulticastTest()
{
   std::string devclass = dabc::mgr()->cfg()->GetUserPar("Device", dabc::typeSocketDevice);

   std::string mcast_host = dabc::mgr()->cfg()->GetUserPar(dabc::xmlMcastAddr, "224.0.0.15");
   int mcast_port = dabc::mgr()->cfg()->GetUserParInt(dabc::xmlMcastPort, 7234);
   bool isrecv = dabc::mgr()->NodeId() > 0;

   DOUT1("Create device %s", devclass.c_str());

   if (!dabc::mgr.CreateDevice(devclass, "MDev")) return;

   dabc::CmdCreateModule cmd("NetTestSpecialModule", "MM");
   cmd.SetBool("IsReceiver", isrecv);
   dabc::mgr.Execute(cmd);

   dabc::CmdCreateTransport cmd2(isrecv ? "MM/Input" : "MM/Output", "MDev");
   cmd2.SetStr(dabc::xmlMcastAddr, mcast_host);
   cmd2.SetInt(dabc::xmlMcastPort, mcast_port);
   dabc::mgr.Execute(cmd2);

   DOUT1("Create transport for addr %s", mcast_host.c_str());

   dabc::mgr.StartAllModules();

   dabc::mgr.Sleep(5);

   dabc::mgr.StopAllModules();

   DOUT0("Multicast test done");
}

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

// TODO: make example with IB MCast
// one could use global ID like
//           <McastAddr value="FF:12:A0:1C:FE:80:00:00:00:00:00:00:33:44:55:66"/>
//           <McastPort value="112233"/>


#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/ModuleAsync.h"
#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Url.h"
#include "dabc/Configuration.h"

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
         cmd.SetReceiver(dabc::mgr.ComposeAddress(dabc::mgr.GetNodeAddress(fSendCnt), "Receiver"));
         cmd.SetTimeout(1);

         dabc::mgr.Submit(Assign(cmd));
      }

      virtual bool ReplyCommand(dabc::Command cmd)
      {
         double tmout = cmd.TimeTillTimeout();
         if (fSendCnt != (unsigned) dabc::mgr.NodeId())
            Par("CmdExeTime").SetValue((1.- tmout)*1000.);

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



class NetTestFactory : public dabc::Factory  {
   public:

      NetTestFactory(const std::string& name) : dabc::Factory(name) {}

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

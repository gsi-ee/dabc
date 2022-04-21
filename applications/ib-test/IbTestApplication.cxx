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
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/threads.h"
#include "dabc/Application.h"
#include "dabc/SocketDevice.h"
#include "dabc/statistic.h"
#include "dabc/Factory.h"
#include "dabc/Configuration.h"
#include "dabc/Url.h"

#include "IbTestWorkerModule.h"

class IbTestApplication : public dabc::Application {
   public:

      IbTestApplication() : dabc::Application("IbTestApp")
      {
         DOUT2("Create ib-test application");

         CreatePar("NetDevice").Dflt(dabc::typeSocketDevice);

         CreatePar("TestKind").Dflt("OnlyConnect");
         CreatePar("TestReliable").Dflt(true);
         CreatePar("TestTimeSyncShort").Dflt(false);

         CreatePar("TestOutputQueue").Dflt(5);
         CreatePar("TestInputQueue").Dflt(10);
         CreatePar("TestBufferSize").Dflt(128*1024);
         CreatePar("TestPoolSize").Dflt(250);
         CreatePar("TestRate").Dflt(1000);
         CreatePar("TestTime").Dflt(10);
         CreatePar("TestPattern").Dflt(0);
         CreatePar("TestNumLids").Dflt(1);
         CreatePar("TestSchedule");
         CreatePar("TestTimeout").Dflt(20);
         CreatePar("TestRead").Dflt(true);
         CreatePar("TestWrite").Dflt(true);

         DOUT2("ib-test application was build");
      }

      virtual ~IbTestApplication()
      {
      }

      virtual bool CreateAppModules()
      {
         std::string devclass = Par("NetDevice").Value().AsStr();

         if (!dabc::mgr.CreateDevice(devclass, "NetDev")) return false;

         int connect_packet_size = 1024 + dabc::mgr.NumNodes() * Par("TestNumLids").Value().AsInt() * sizeof(IbTestConnRec);
         int bufsize = 16*1024;
         while (bufsize < connect_packet_size) bufsize*=2;

         dabc::mgr.CreateMemoryPool("SendPool", bufsize, dabc::mgr.NumNodes() * 4);

         dabc::CmdCreateModule cmd("IbTestWorkerModule", IBTEST_WORKERNAME, "IbTestThrd");
         cmd.SetInt("NodeNumber", dabc::mgr()->NodeId());
         cmd.SetInt("NumNodes", dabc::mgr.NumNodes());
         cmd.SetInt("NumPorts", (dabc::mgr()->NodeId()==0) ? dabc::mgr.NumNodes()-1 : 1);

         if (!dabc::mgr.Execute(cmd)) return false;

         for (int node = 1; node < dabc::mgr.NumNodes(); node++) {
            std::string port1 = dabc::Url::ComposePortName(0, dabc::format("%s/Output", IBTEST_WORKERNAME), node-1);

            std::string port2 = dabc::Url::ComposePortName(node, dabc::format("%s/Input", IBTEST_WORKERNAME), 0);

            dabc::mgr.Connect(port1, port2).SetOptional(dabc::mgr()->NodeId()==0);
         }

         return true;
      }

      virtual bool BeforeAppModulesStarted()
      {
         DOUT0("Num threads in ib-test = %d %d", dabc::mgr.NumThreads(), dabc::Thread::NumThreadInstances());

         return true;
      }

      /** use 20 seconds timeout, probably should be extended */
      virtual int SMCommandTimeout() const
      {
         // all slaves should be able to connect inside 10 seconds, master timeout could be configurable
         if (dabc::mgr.NodeId()>0) return 10;

         return Par("TestTimeout").Value().AsInt(20);
      }

};



class IbTestFactory : public dabc::Factory  {
   public:

      IbTestFactory(const std::string &name) : dabc::Factory(name) {}

      dabc::Application *CreateApplication(const std::string &classname, dabc::Command cmd) override
      {
         if (classname == "IbTestApp")
            return new IbTestApplication();

         return dabc::Factory::CreateApplication(classname, cmd);
      }


      dabc::Module *CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd) override
      {
         if (classname == "IbTestWorkerModule")
            return new IbTestWorkerModule(modulename, cmd);

         return nullptr;
      }
};

dabc::FactoryPlugin ibtest(new IbTestFactory("ib-test"));


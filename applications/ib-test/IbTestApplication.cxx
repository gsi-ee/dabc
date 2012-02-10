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
#include "dabc/Url.h"

#include "IbTestWorkerModule.h"

class IbTestApplication : public dabc::Application {
   public:

      IbTestApplication() : dabc::Application("IbTestApp")
      {
         DOUT2(("Create ib-test application"));

         CreatePar("NetDevice").DfltStr(dabc::typeSocketDevice);

         CreatePar("TestKind").DfltStr("OnlyConnect");
         CreatePar("TestReliable").DfltBool(true);
         CreatePar("TestTimeSyncShort").DfltBool(false);

         CreatePar("TestOutputQueue").DfltInt(5);
         CreatePar("TestInputQueue").DfltInt(10);
         CreatePar("TestBufferSize").DfltInt(128*1024);
         CreatePar("TestPoolSize").DfltInt(250);
         CreatePar("TestRate").DfltInt(1000);
         CreatePar("TestTime").DfltInt(10);
         CreatePar("TestPattern").DfltInt(0);
         CreatePar("TestNumLids").DfltInt(1);
         CreatePar("TestSchedule");
         CreatePar("TestTimeout").DfltDouble(20);
         CreatePar("TestRead").DfltBool(true);
         CreatePar("TestWrite").DfltBool(true);

         DOUT2(("ib-test application was build"));
      }

      virtual ~IbTestApplication()
      {
      }

      virtual bool CreateAppModules()
      {
         std::string devclass = Par("NetDevice").AsStdStr();
         
         if (!dabc::mgr.CreateDevice(devclass.c_str(), "NetDev")) return false;

         int connect_packet_size = 1024 + NumNodes() * Par("TestNumLids").AsInt() * sizeof(IbTestConnRec);
         int bufsize = 16*1024;
         while (bufsize < connect_packet_size) bufsize*=2;

         dabc::mgr.CreateMemoryPool("SendPool", bufsize, NumNodes() * 4, 2);

         dabc::CmdCreateModule cmd("IbTestWorkerModule", IBTEST_WORKERNAME, "IbTestThrd");
         cmd.Field("NodeNumber").SetInt(dabc::mgr()->NodeId());
         cmd.Field("NumNodes").SetInt(NumNodes());
         cmd.Field("NumPorts").SetInt((dabc::mgr()->NodeId()==0) ? NumNodes()-1 : 1);

         if (!dabc::mgr.Execute(cmd)) return false;

         for (unsigned node = 1; node < NumNodes(); node++) {
            std::string port1 = dabc::Url::ComposePortName(0, FORMAT(("%s/Port", IBTEST_WORKERNAME)), node-1);

            std::string port2 = dabc::Url::ComposePortName(node, FORMAT(("%s/Port", IBTEST_WORKERNAME)), 0);

            dabc::mgr.Connect(port1, port2).SetOptional(dabc::mgr()->NodeId()==0);
         }
         
         return true;
      }

      virtual bool BeforeAppModulesStarted()
      {
         DOUT0(("Num threads in ib-test = %d %d", dabc::mgr()->GetThreadsFolder(true).NumChilds(), dabc::Thread::NumThreadInstances()));

         return true;
      }

      /** use 20 seconds timeout, probably should be extended */
      virtual int SMCommandTimeout() const
      {
         // all slaves should be able to connect inside 10 seconds, master timeout could be configurable
         if (dabc::mgr()->NodeId()>0) return 10.;

         return Par("TestTimeout").AsInt(20);
      }

};



class IbTestFactory : public dabc::Factory  {
   public:

      IbTestFactory(const char* name) : dabc::Factory(name) {}

      virtual dabc::Application* CreateApplication(const char* classname, dabc::Command cmd)
      {
         if (strcmp(classname, "IbTestApp")==0)
            return new IbTestApplication();
         return dabc::Factory::CreateApplication(classname, cmd);
      }


      virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
      {
         if (strcmp(classname,"IbTestWorkerModule")==0)
            return new IbTestWorkerModule(modulename, cmd);


         return 0;
      }
};

IbTestFactory nettest("ib-test");


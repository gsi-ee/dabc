#include "bnet/Application.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Url.h"

#include "bnet/defines.h"
#include "bnet/TransportModule.h"


//#include "dabc/timing.h"
//#include "dabc/Port.h"
//#include "dabc/Timer.h"
//#include "dabc/Command.h"
//#include "dabc/MemoryPool.h"
//#include "dabc/PoolHandle.h"
//#include "dabc/threads.h"
//#include "dabc/Application.h"
//#include "dabc/SocketDevice.h"
//#include "dabc/statistic.h"
//#include "dabc/Factory.h"
//#include "dabc/Configuration.h"

bnet::Application::Application() :
   dabc::Application("bnet::Application")
{
   DOUT2(("Create bnet::Application"));

   CreatePar("NetDevice").DfltStr(dabc::typeSocketDevice);

   CreatePar(dabc::xmlBufferSize).DfltInt(65536);
   CreatePar(dabc::xmlNumBuffers).DfltInt(1000);

   CreatePar("SpecialThread").DfltBool(false);


   CreatePar("TestKind").DfltStr("OnlyConnect");
   CreatePar("TestReliable").DfltBool(true);

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

bnet::Application::~Application()
{
}

bool bnet::Application::CreateAppModules()
{
   std::string devclass = Par("NetDevice").AsStdStr();

   if (!dabc::mgr.CreateDevice(devclass.c_str(), "NetDev")) return false;

   // int connect_packet_size = 1024 + NumNodes() * Par("TestNumLids").AsInt() * sizeof(VerbsConnRec);
   int connect_packet_size = 50000;
   int bufsize = 16*1024;
   while (bufsize < connect_packet_size) bufsize*=2;
   dabc::mgr.CreateMemoryPool("BnetCtrlPool", bufsize, NumNodes() * 4, 2);

   bufsize = Cfg(dabc::xmlBufferSize).AsInt(65536);
   int numbuf = Cfg(dabc::xmlNumBuffers).AsInt(1024);
   dabc::mgr.CreateMemoryPool("BnetDataPool", bufsize, numbuf, 2);

   dabc::CmdCreateModule cmd("bnet::TransportModule", IBTEST_WORKERNAME, "BnetModule");
   cmd.Field("NodeNumber").SetInt(dabc::mgr.NodeId());
   cmd.Field("NumNodes").SetInt(dabc::mgr.NumNodes());
   cmd.Field("NumPorts").SetInt((dabc::mgr.NodeId()==0) ? dabc::mgr.NumNodes()-1 : 1);

   if (!dabc::mgr.Execute(cmd)) return false;

   for (unsigned node = 1; node < NumNodes(); node++) {
      std::string port1 = dabc::Url::ComposePortName(0, FORMAT(("%s/Port", IBTEST_WORKERNAME)), node-1);

      std::string port2 = dabc::Url::ComposePortName(node, FORMAT(("%s/Port", IBTEST_WORKERNAME)), 0);

      dabc::mgr.Connect(port1, port2).SetOptional(dabc::mgr.NodeId()==0);
   }

   return true;
}

bool bnet::Application::BeforeAppModulesStarted()
{
   DOUT0(("Num threads in ib-test = %d %d", dabc::mgr()->GetThreadsFolder(true).NumChilds(), dabc::Thread::NumThreadInstances()));

   return true;
}

/** use 20 seconds timeout, probably should be extended */
int bnet::Application::SMCommandTimeout() const
{
   // all slaves should be able to connect inside 10 seconds, master timeout could be configurable
   if (dabc::mgr()->NodeId()>0) return 10.;

   return Par("TestTimeout").AsInt(20);
}




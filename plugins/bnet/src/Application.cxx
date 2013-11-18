#include "bnet/Application.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Url.h"

#include "bnet/defines.h"
#include "bnet/TransportModule.h"

bnet::Application::Application() :
   dabc::Application("bnet::Application")
{
   DOUT2("Create bnet::Application");

   CreatePar("NetDevice").Dflt(dabc::typeSocketDevice);

   CreatePar(dabc::xmlBufferSize).Dflt(65536);
   CreatePar(dabc::xmlNumBuffers).Dflt(1000);

   CreatePar("SpecialThread").Dflt(false);


   CreatePar("TestKind").Dflt("OnlyConnect");
   CreatePar("TestReliable").Dflt(true);

   CreatePar("TestOutputQueue").Dflt(5);
   CreatePar("TestInputQueue").Dflt(10);
   CreatePar("TestEventQueue").Dflt(100);
   CreatePar(names::EventLifeTime()).Dflt(2.);
   CreatePar("TestBufferSize").Dflt(128*1024);
   CreatePar("TestPoolSize").Dflt(250);
   CreatePar("TestRate").Dflt(1000);
   CreatePar("TestTime").Dflt(10);
   CreatePar("TestPattern").Dflt(0);
   CreatePar("TestNumLids").Dflt(1);
   CreatePar("TestSchedule");
   CreatePar("TestTimeout").Dflt(20);
   CreatePar(names::ControlKind()).Dflt(0);
   CreatePar("TestStartTurns").Dflt(10);
   CreatePar("TestRead").Dflt(true);
   CreatePar("TestWrite").Dflt(true);

   DOUT2("ib-test application was build");
}

bnet::Application::~Application()
{
}

bool bnet::Application::CreateAppModules()
{
   std::string devclass = Par("NetDevice").Value().AsStr();

   if (!dabc::mgr.CreateDevice(devclass, "NetDev")) return false;

   // int connect_packet_size = 1024 + NumNodes() * Par("TestNumLids").AsInt() * sizeof(VerbsConnRec);
   int connect_packet_size = 50000;
   int bufsize = 16*1024;
   while (bufsize < connect_packet_size) bufsize*=2;
   dabc::mgr.CreateMemoryPool("BnetCtrlPool", bufsize, NumNodes() * 4);

   bufsize = Cfg(dabc::xmlBufferSize).AsInt(65536);
   int numbuf = Cfg(dabc::xmlNumBuffers).AsInt(1024);
   dabc::mgr.CreateMemoryPool("BnetDataPool", bufsize, numbuf);

   dabc::CmdCreateModule cmd("bnet::TransportModule", names::WorkerModule(), "BnetModuleThrd");
   cmd.SetInt("NodeNumber", dabc::mgr.NodeId());
   cmd.SetInt("NumNodes", dabc::mgr.NumNodes());
   cmd.SetInt("NumPorts" , (dabc::mgr.NodeId()==0) ? dabc::mgr.NumNodes()-1 : 1);
   if (!dabc::mgr.Execute(cmd)) return false;

   // create event generator only for nodes there it is really necessary
   if ((Cfg(names::ControlKind()).AsInt()==0) || (dabc::mgr.NodeId()>0)) {
      dabc::mgr.CreateModule("bnet::GeneratorModule", "BnetGener", "BnetGenerThrd");
      dabc::mgr.Connect("BnetGener/Output", dabc::format("%s/DataInput", names::WorkerModule()));
   }

   for (unsigned node = 1; node < NumNodes(); node++) {
      std::string port1 = dabc::Url::ComposePortName(0, dabc::format("%s/Port", names::WorkerModule()), node-1);

      std::string port2 = dabc::Url::ComposePortName(node, dabc::format("%s/Port", names::WorkerModule()), 0);

      dabc::mgr.Connect(port1, port2).SetOptional(dabc::mgr.NodeId()==0);
   }

   return true;
}

bool bnet::Application::BeforeAppModulesStarted()
{
   DOUT0("Num threads in ib-test = %d %d", dabc::mgr.NumThreads(), dabc::Thread::NumThreadInstances());

   return true;
}

/** use 20 seconds timeout, probably should be extended */
int bnet::Application::SMCommandTimeout() const
{
   // all slaves should be able to connect inside 10 seconds, master timeout could be configurable
   if (dabc::mgr()->NodeId()>0) return 10;

   return Par("TestTimeout").Value().AsInt(20);
}




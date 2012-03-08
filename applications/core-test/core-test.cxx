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

#include "dabc/FileIO.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/BuffersQueue.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"
#include "dabc/statistic.h"
#include "dabc/Timer.h"
#include "dabc/MemoryPool.h"
#include "dabc/CommandsSet.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"
#include "dabc/Application.h"


#define BUFFERSIZE 1024
#define QUEUESIZE 5

long global_cnt = 0;
bool global_cnt_fill = false;

class TestModuleAsync : public dabc::ModuleAsync {
   protected:
      int                 fKind; // 0 - first in chain, 1 - repeater, 2 - last in chain,

   public:

      TestModuleAsync(const char* name, int kind = -1) :
         dabc::ModuleAsync(name),
         fKind(kind)
      {
         CreatePoolHandle("Pool");

//         dabc::SetDebugLevel(3);
//         DOUT0(("--------------- KIND start ---------------------------"));

         if (fKind<0) fKind = Cfg("Kind").AsInt(0);

//         DOUT0(("--------------- KIND FINISH ---------------------------"));
//         dabc::SetDebugLevel(0);

         DOUT3(("Create module:%s kind = %d Class %s", GetName(), fKind, ClassName()));

         if (fKind>0) CreateInput("Input", Pool(), QUEUESIZE);

         if (fKind<2) CreateOutput("Output", Pool(), QUEUESIZE);
      }

      virtual ~TestModuleAsync()
      {
         DOUT3(("Async module %s destroyed", GetName()));
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         if (fKind==1)
            if (!Input()->CanRecv() || !Output()->CanSend()) return;

         dabc::Buffer buf = Input()->Recv();

         if (buf.null()) { EOUT(("CCCCCCCCCC")); exit(1); }

         if (fKind==2) {
            buf.Release();
            if (global_cnt_fill) global_cnt++;
         } else
            Output()->Send(buf);

      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         if (!Output()->CanSend()) return;

         if (fKind==0) {
            dabc::Buffer buf = Pool()->TakeBuffer(BUFFERSIZE);
            if (buf.null()) { EOUT(("AAAAAAAAAAAA")); exit(1); }
            Output()->Send(buf);
         } else
         if ((fKind==1) && Input()->CanRecv()) {
            dabc::Buffer buf = Input()->Recv();
            if (buf.null()) { EOUT(("BBBBBBBBBBBB")); exit(1); }
            Output()->Send(buf);
         }
      }

      virtual void AfterModuleStop()
      {
         DOUT3(("TestModuleAsync %s stops, cnt = %ld", GetName(), global_cnt));
      }
};


class TestModuleSync : public dabc::ModuleSync {
   protected:
      int                 fKind; // 0 - first in chain, 1 - repeater, 2 - last in chain,

   public:
      TestModuleSync(const char* name, int kind = -1) :
         dabc::ModuleSync(name),
         fKind(kind)
      {
         CreatePoolHandle("Pool");

         if (fKind<0) fKind = Cfg("Kind").AsInt(0);

         if (fKind>0) CreateInput("Input", Pool(), QUEUESIZE);

         if (fKind<2) CreateOutput("Output", Pool(), QUEUESIZE);
      }

      virtual ~TestModuleSync()
      {
         DOUT0(("Sync module %s destroyed", GetName()));
      }

      virtual void MainLoop()
      {
         switch (fKind) {
            case 0: GeneratorLoop(); break;
            case 1: RepeaterLoop(); break;
            case 2: ReceiverLoop(); break;
            default: EOUT(("Error"));
         }
      }

      void GeneratorLoop()
      {
         while (ModuleWorking()) {
            dabc::Buffer buf = TakeBuffer(Pool(), BUFFERSIZE);
            Send(Output(), buf);
         }
      }

      void RepeaterLoop()
      {
         while (ModuleWorking())
            Send(Output(), Recv(Input()));
      }

      void ReceiverLoop()
      {
         while (ModuleWorking()) {
            dabc::Buffer buf = Recv(Input());
            buf.Release();
            if (global_cnt_fill) global_cnt++;
         }
      }

      virtual void AfterModuleStop()
      {
         DOUT0(("Module %s did stop", GetName()));
      }
};

class CoreTestApplication : public dabc::Application {
   public:

      CoreTestApplication() : dabc::Application("CoreTestApp")
      {
         DOUT0(("Create net test application"));

         CreatePar("TestKind").DfltStr("sync");

         CreatePar(dabc::xmlBufferSize).DfltInt(1024);

         DOUT0(("Test application was build bufsize = %d", Par(dabc::xmlBufferSize).AsUInt()));
      }

      bool IsSync()
      {
         return Par("TestKind").AsStdStr() == "sync";
      }

      virtual bool CreateAppModules()
      {
         dabc::mgr.CreateMemoryPool("Pool");

         dabc::mgr.CreateModule("TestModuleAsync", "Sender");

         dabc::mgr.CreateModule("TestModuleAsync", "Receiver");

         dabc::mgr.Connect("Sender/Output", "Receiver/Input");

         global_cnt_fill = true;

         DOUT0(("Create all modules done"));

         return true;
      }

};

class CoreTestFactory : public dabc::Factory  {
   public:

      CoreTestFactory(const char* name) : dabc::Factory(name) {}

      virtual dabc::Application* CreateApplication(const char* classname, dabc::Command cmd)
      {
         if (strcmp(classname, "CoreTestApp")==0)
            return new CoreTestApplication();
         return dabc::Factory::CreateApplication(classname, cmd);
      }

      virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
      {
         if (strcmp(classname,"TestModuleAsync")==0)
            return new TestModuleAsync(modulename);
         else
         if (strcmp(classname,"TestModuleSync")==0)
            return new TestModuleSync(modulename);

         return 0;
      }
};

CoreTestFactory coretest("core-test");



void TestChain(bool isM, int number, int testkind = 0, double test_tm = 2.)
{
   DOUT0(("==============================================="));
   DOUT0(("Test chain of %d %s modules, using %d threads",
            number, (isM ? "Sync" : "Async"), (testkind==0 ? number : testkind)));

   dabc::mgr.CreateMemoryPool("Pool", BUFFERSIZE, number*QUEUESIZE*2, 2);

   for (int n=0;n<number;n++) {
      int kind = 1;
      if (n==0) kind = 0; else
      if (n==(number-1)) kind = 2;

      dabc::Module* m = 0;

      if (isM) {
         m = new TestModuleSync(FORMAT(("Module%d",n)), kind);
         dabc::mgr()->MakeThreadFor(m);
      } else {
         m = new TestModuleAsync(FORMAT(("Module%d",n)), kind);

         switch (testkind) {
            case 1:
               dabc::mgr()->MakeThreadFor(m, "MainThread");
               break;
            case 2:
              if (n<number/2) dabc::mgr()->MakeThreadFor(m, "MainThread0");
                         else dabc::mgr()->MakeThreadFor(m, "MainThread1");
              break;
            default:
              dabc::mgr()->MakeThreadFor(m);
         }
      }
   }

   DOUT0(("Create module done"));

   bool connectres = true;

   dabc::CpuStatistic cpu;

   dabc::TimeStamp tm1, tm2;

   for (int n=1; n<number; n++)
      if (!dabc::mgr()->ConnectPorts( FORMAT(("Module%d/Output", n-1)),
                         FORMAT(("Module%d/Input", n)))) connectres = false;

   DOUT0(("Connect ports done %s", DBOOL(connectres)));

//   dabc::Iterator::PrintHieararchy(dabc::mgr);

   if (!connectres)
     EOUT(("Ports are not connect"));
   else {

      global_cnt = 0;

      dabc::SetDebugLevel(0);

      dabc::mgr.StartAllModules();

      DOUT0(("Start all modules"));
      cpu.Reset();

      tm1 = dabc::Now();
      global_cnt_fill = true;

      dabc::mgr()->Sleep(test_tm, "Main loop");

      tm2 = dabc::Now();
      global_cnt_fill = false;

      cpu.Measure();



//      DOUT0(("Current Thread %p name %s manager thread %p", dabc::mgr()->CurrentThread()(), dabc::mgr()->CurrentThrd().GetName(), dabc::mgr()->thread()()));

//      dabc::SetDebugLevel(5);
//      dabc::SetFileLevel(5);

       dabc::mgr.StopAllModules();

       DOUT0(("Stop all modules"));
   }

//   dabc::SetDebugLevel(1);
//   dabc::lgr()->SetFileLevel(5);

   dabc::mgr.CleanupApplication();

   dabc::mgr()->Sleep(0.1);

   dabc::Object::InspectGarbageCollector();


   if (global_cnt<=0) global_cnt = 1;

   DOUT0(("IsM = %s Kind = %d Time = %5.3f Cnt = %ld Per buffer = %5.3f ms CPU = %5.1f",DBOOL(isM), testkind,
      tm2-tm1, global_cnt, (tm2-tm1)/global_cnt*1e3, cpu.CPUutil()*100.));

   DOUT0(("Did manager cleanup"));
}

class TimeoutTestModuleAsync : public dabc::ModuleAsync {
   protected:
      long                fCounter1;
      long                fCounter2;
      bool                fWithRate;

   public:
      TimeoutTestModuleAsync(const char* name, bool withrate = false) :
         dabc::ModuleAsync(name),
         fCounter1(0),
         fCounter2(0)
      {
         CreateTimer("Timer1", 0.01, true);
         CreateTimer("Timer2", 0.01, false);

         if (withrate)
            CreatePar("Rate").SetRatemeter(false).SetDebugOutput(1).SetUnits("call");
      }

      virtual void ProcessTimerEvent(dabc::Timer* timer)
      {
         if (timer->IsName("Timer1")) {
            fCounter1++;
            Par("Rate").SetDouble(1.);
         } else
            fCounter2++;
      }

      virtual void AfterModuleStop()
      {
         DOUT0(("Module %s Timer1 %ld Timer2 %ld", GetName(), fCounter1, fCounter2));
      }

};

void TestTimers(int number)
{
   DOUT0(("==============================================="));
   DOUT0(("Test timers with %d modules, running in the same thread", number));

   for (int n=0;n<number;n++) {
      dabc::Module* m = new TimeoutTestModuleAsync(FORMAT(("Module%d",n)), (number==1));

      dabc::mgr()->MakeThreadFor(m, "MainThread");
   }

   dabc::CpuStatistic cpu;

   dabc::mgr.StartAllModules();

   dabc::TimeStamp tm1 = dabc::Now();

   cpu.Reset();

   dabc::mgr()->Sleep(5, "Main loop");

   cpu.Measure();

   dabc::mgr.StopAllModules();

   dabc::TimeStamp tm2 = dabc::Now();

   DOUT1(("Overall time = %3.2f CPU = %5.1f", tm2-tm1, cpu.CPUutil()*100.));

   dabc::mgr.CleanupApplication();

   DOUT3(("Did manager cleanup"));

   dabc::mgr()->Sleep(0.1);

   dabc::Object::InspectGarbageCollector();

}

void TestMemoryPool()
{
   unsigned bufsize = 2048;
   unsigned numbuf = 2048;

   int numtests = 1000000;

   dabc::MemoryPool mem_pool("Test", true);

   mem_pool.Allocate(bufsize, numbuf, 2);

   DOUT1(("Manager thread = %s", dabc::mgr()->ThreadName().c_str()));
   mem_pool.AssignToThread(dabc::mgr()->thread());
   DOUT1(("Pool thread = %s", mem_pool.ThreadName().c_str()));

   dabc::TimeStamp tm1, tm2;


   tm1 = dabc::Now();
   for (int n=0;n<numtests;n++) {
      dabc::Buffer buf = mem_pool.TakeBuffer(bufsize);
      buf.Release();
   }
   tm2 = dabc::Now();
   DOUT1(("Simple get without headers %5.3f, per buffer %4.0f nano", tm2-tm1, (tm2-tm1)/numtests*1e9));

   tm1 = dabc::Now();
   for (int n=0;n<numtests;n++) {
      dabc::Buffer buf = mem_pool.TakeBuffer(bufsize);
      buf.Release();
   }
   tm2 = dabc::Now();
   DOUT1(("Simple get with headers %5.3f, per buffer %4.0f nano", tm2-tm1, (tm2-tm1)/numtests*1e9));


   tm1 = dabc::Now();

//   double sum1, sum2;

   for (int n=0;n<1000000;n++) {
      dabc::Buffer buf = mem_pool.TakeBuffer(bufsize*4);
      buf.Release();
   }
   tm2 = dabc::Now();
   DOUT1(("Simple get of large buffers %5.3f", tm2-tm1));
//   DOUT1(("Take %5.1f Release %5.1f", sum1, sum2));

//   delete mem_pool;
//   return;


   #define BlockSize 100
   dabc::Buffer block[BlockSize];

   tm1 = dabc::Now();
   for (int k=0;k<1000000/BlockSize;k++) {
      for (int n=0;n<BlockSize;n++)
         block[n] = mem_pool.TakeBuffer(bufsize);
      for (int n=0;n<BlockSize;n++)
         block[n].Release();
   }
   tm2 = dabc::Now();
   DOUT1(("Block get of simple buffers %5.3f", tm2-tm1));


   #define BigBlockSize 10000
   dabc::Buffer bigblock[BigBlockSize];

//   mem_pool->AllocateMemory(bufsize, BigBlockSize, 0);
//   mem_pool->AllocateMemory(bufsize * 16, BigBlockSize, 0);

   tm1 = dabc::Now();
   for (int k=0;k<1000000/BigBlockSize;k++) {
      int failcnt = 0;
      for (int n=0;n<BigBlockSize;n++) {
         bigblock[n] = mem_pool.TakeBuffer(bufsize);
         if (bigblock[n].null()) failcnt++;
      }

      if (failcnt>0) EOUT(("Failed requests = %d", failcnt));

      for (int n=0;n<BigBlockSize;n++)
         bigblock[n].Release();

   }
   tm2 = dabc::Now();
   DOUT1(("Large block get of simple buffers %5.3f", tm2-tm1));

   sleep(2);

   DOUT1(("Did sleep"));
}


struct MyRec {
   int i;
   bool b;
   MyRec()
   {
      DOUT0(("Default constructor %p", this));
   }
   MyRec(const MyRec& src) : i(src.i), b(src.b)
   {
      DOUT0(("Copy constructor from %p to %p", &src, this));
   }
   ~MyRec()
   {
      DOUT0(("Destructor %p", this));
   }

   MyRec& operator=(const MyRec& src)
   {
      DOUT0(("operator= from %p to %p", &src, this));
      i = src.i;
      b = src.b;
      return *this;
   }

   void reset()
   {
      DOUT0(("reset of %p", this));

   }

};

void TestQueue()
{
   DOUT0(("Create queue"));
   dabc::Queue<MyRec, true> q(4);

   DOUT0(("Create record"));
   MyRec r;

   DOUT0(("Push record"));
   q.Push(r);

   DOUT0(("Pop record"));
   r = q.Pop();

   DOUT0(("Push 3 records"));
   for (int n=0;n<3;n++) q.Push(r);

   DOUT0(("Pop 3 records"));
   for (int n=0;n<3;n++) q.Pop();

   DOUT0(("Push 2 records"));
   for (int n=0;n<2;n++) q.Push(r);

   DOUT0(("Reset queue"));
   q.Reset();

   DOUT0(("Destroy queue"));
}

void TestRecordsQueue()
{
   DOUT0(("Create complex queue"));
   dabc::RecordsQueue<MyRec> q(4);

   DOUT0(("Create record"));
   MyRec r;

   DOUT0(("Push record"));
   q.Push(r);

   DOUT0(("Pop record"));
   r = q.Front();
   q.PopOnly();

   DOUT0(("Push 3 records"));
   for (int n=0;n<3;n++) q.Push(r);

   DOUT0(("Pop 3 records"));
   for (int n=0;n<3;n++) q.PopOnly();

   DOUT0(("Push 2 records"));
   for (int n=0;n<2;n++) q.Push(r);

   DOUT0(("Reset queue"));
   q.Reset();

   DOUT0(("Destroy queue"));
}


#include <list>

void TestList()
{
   DOUT0(("Create list"));
   std::list<MyRec> l;

   DOUT0(("Create record"));
   MyRec r;

   DOUT0(("Push record"));
   l.push_back(r);

   DOUT0(("Pop record"));
   r = l.front();
   l.pop_front();

   DOUT0(("Push 3 records"));
   for (int n=0;n<3;n++) l.push_back(r);

   DOUT0(("Pop 3 records"));
   for (int n=0;n<3;n++) l.pop_front();

   DOUT0(("Push 2 records"));
   for (int n=0;n<2;n++) l.push_back(r);

   DOUT0(("Reset list"));
   l.clear();

   DOUT0(("Destroy list"));

}

#include <vector>

void TestVector()
{
   DOUT0(("Create vector"));
   std::vector<MyRec> l;
   l.resize(4);

   DOUT0(("Create record"));
   MyRec r;

   DOUT0(("Push record"));
   l.push_back(r);

   DOUT0(("Pop record"));
   r = l.back();
   l.pop_back();

   DOUT0(("Push 3 records"));
   for (int n=0;n<3;n++) l.push_back(r);

   DOUT0(("Pop 3 records"));
   for (int n=0;n<3;n++) l.pop_back();

   DOUT0(("Push 2 records"));
   for (int n=0;n<2;n++) l.push_back(r);

   DOUT0(("Reset vector"));
   l.clear();

   DOUT0(("Destroy vector"));

}

class TestModuleCmd : public dabc::ModuleAsync {
   protected:
      int                 fNext; // -1 reply, or number of next in chain
      int                 fCount;
      bool                fTimeout;
      bool                fSameCmd;  //!< should we submit same command or create new one
      dabc::Command       fTmCmd;

   public:

      enum { MaxLoop = 10 };

      TestModuleCmd(const char* name, int next, bool timeout = false, bool samecmd = true) :
         dabc::ModuleAsync(name),
         fNext(next),
         fCount(0),
         fTimeout(timeout),
         fSameCmd(samecmd),
         fTmCmd()
      {
      }

      virtual int ExecuteCommand(dabc::Command cmd)
      {
         DOUT2(("Module %s Executes command %s cnt:%d", GetName(), cmd.GetName(), fCount));

         if ((fNext<0) || (++fCount>MaxLoop)) {

            if (fTimeout && !fTmCmd.null()) {
               EOUT(("Module %s waiting for timeout - reject cmd %s", GetName(), cmd.GetName()));
               return dabc::cmd_false;
            }

            if (fTimeout && fTmCmd.null()) {
               fTmCmd = cmd;
               ActivateTimeout(5);
               DOUT0(("Module %s activate timeout", GetName()));
               return dabc::cmd_postponed;
            }

            return dabc::cmd_true;
         }

         std::string nextname = dabc::format("Module%d", fNext);
         dabc::ModuleRef next = dabc::mgr.FindModule(nextname.c_str());
         if (next.null()) return dabc::cmd_false;

         if (fTimeout) {
            next.Submit(Assign(cmd));
            return dabc::cmd_postponed;
         }

         if (!fSameCmd) cmd = dabc::Command(FORMAT(("%s_cmd%d", GetName(), fCount)));

         int res = ExecuteIn(next(), cmd);

         DOUT4(("Module %s ExecuteIn res = %d", GetName(), res));

         return res>0 ? res+1 : dabc::cmd_false;
      }

      virtual double ProcessTimeout(double last_diff)
      {
         DOUT2((" ++++++++++++ Module %s process timeout diff %5.3f ++++++++++++++", GetName(), last_diff));

         fTmCmd.ReplyTrue();

         return -1;
      }

      virtual bool ReplyCommand(dabc::Command cmd)
      {
         int res = cmd.GetResult();
         DOUT2(("Module %s Get reply res = %d", GetName(), res));
         cmd.Reply(res+1);
         return false;
      }
};


void TestCmdChain(int number, bool timeout = false, bool samecmd = true)
{
   DOUT0(("==============================================="));
   DOUT0(("Test cmd chain of %d modules timeout:%s samecommand:%s", number, DBOOL(timeout), DBOOL(samecmd)));

   dabc::Module* m0 = 0;

   for (int n=0;n<number;n++) {
      dabc::Module* m = new TestModuleCmd(FORMAT(("Module%d",n)), n==number-1 ? 0 : n+1, timeout, samecmd);

      dabc::mgr()->MakeThreadFor(m, n % 2 ? "MainThread0" : "MainThread1");

      if (n==0) m0 = m;
   }

   dabc::mgr.StartAllModules();

   DOUT0(("==============================================="));
   DOUT0(("Inject command m0 = %p %s", m0, m0->GetName()));

   dabc::Command cmd("MyCommand");
   cmd.SetTimeout(timeout ? 10. : 1.);
   m0->Execute(cmd);

   int res = cmd.GetResult();
   int expected = number* TestModuleCmd::MaxLoop + dabc::cmd_true;

   DOUT0(("Execution result = %d (expected = %d)", res, expected));

   if (res != expected)
      EOUT(("Test fails, expected result is %d", expected));

//   dabc::mgr.StopAllModules();

//   for (int n=0;n<number;n++)
//      dabc::mgr.DeleteModule(FORMAT(("Module%d",n)));

   dabc::mgr.CleanupApplication();

   dabc::mgr()->Sleep(0.1);

   dabc::Object::InspectGarbageCollector();

   DOUT0(("Did manager cleanup"));
}

void TestCmdSet(int number, bool sync, int numtest = 1, bool errtmout = false)
{
   DOUT0(("==============================================="));
   DOUT0(("Test cmd set with %d modules", number));

   for (int n=0;n<number;n++) {
      dabc::Module* m = new TestModuleCmd(FORMAT(("SetModule%d",n)), -1, errtmout);

      dabc::mgr()->MakeThreadFor(m, (n % 2) ? "SetThread0" : "SetThread1");
   }

   dabc::mgr.StartAllModules();

   for (int ntry=0;ntry<numtest;ntry++) {

      DOUT0(("================ NTRY = %d ===============================", ntry));
      DOUT0(("Create set"));

      dabc::CommandsSet* set = new dabc::CommandsSet(dabc::mgr()->thread());

      set->SetParallelExe(true);

      for (int n=0;n<number;n++) {
         dabc::Command cmd("MyCommand");
         cmd.SetReceiver(FORMAT(("SetModule%d",n)));
         set->Add(cmd);
      }

      DOUT0(("Inject set"));

//      if (ntry == numtest-1) dabc::SetDebugLevel(5);

      if (sync) {
         int res = set->ExecuteSet(2.);
         dabc::Object::Destroy(set);
         DOUT0(("SET result = %d", res));
      } else {
         set->SubmitSet(0, 2.);
         dabc::mgr()->Sleep(3.);
         DOUT0(("SET result = unknown"));
      }
   }


   DOUT0(("Do stop modules"));

   dabc::mgr.StopAllModules();

   DOUT0(("Did stop modules"));

//   for (int n=0;n<number;n++)
//      dabc::mgr.DeleteModule(FORMAT(("SetModule%d",n)));

   dabc::mgr.CleanupApplication();

   DOUT0(("Did manager cleanup"));
}

void TestGuard(dabc::Mutex* m, int& b)
{
   dabc::IntGuard block1(b);

   printf("PLACE0: Mutex locked %s b = %d\n", DBOOL(m->IsLocked()),b);

   dabc::LockGuard guard(m);

   printf("PLACE1: Mutex locked %s b = %d\n", DBOOL(m->IsLocked()),b);

   dabc::IntGuard block2(b);

   printf("PLACE2: Mutex locked %s b = %d\n", DBOOL(m->IsLocked()),b);

   dabc::UnlockGuard unlock(m);

   printf("PLACE3: Mutex locked %s b = %d\n", DBOOL(m->IsLocked()),b);

   // here will be mutex locked, fObjectBlock counter decremented and finally mutex unlocked again
}

extern "C" void RunCoreTest()
{
/*
   dabc::Mutex* m = new dabc::Mutex;
   int b = 0;
   TestGuard(m, b);
   printf("PLACE4: Mutex locked %s b = %d\n", DBOOL(m->IsLocked()),b);
   exit(1);
*/


   //   TestMemoryPool();

//   dabc::SetDebugLevel(1);

//   TestRecordsQueue();
//   TestQueue();
//   TestList();
//   TestVector();
//   return;


   TestChain(true, 10);

   TestChain(false, 10, 0);

   TestChain(false, 10, 2);
   TestChain(false, 10, 1);

   TestTimers(1);
   TestTimers(3);
   TestTimers(10);
}


extern "C" void RunTimersTest()
{
   TestTimers(1);
   TestTimers(3);
   TestTimers(10);
}




extern "C" void RunCmdTest()
{
/*

   dabc::Command cmd1("MyCmd");
   cmd1.SetBool("abc", false);
   cmd1.SetInt("kdf", 122);
   cmd1.SetStr("bad", "bad & string");
   cmd1.SetStr("bad par", "with bad \"string's");
   cmd1.SetInt("_lmc", 1222);
   cmd1.SetInt("#hidden", 1222);

   std::string s1,s2;
   cmd1.SaveToString(s1, false);

   DOUT1(("CMD1: \n%s", s1.c_str()));


   dabc::Command cmd2;
   cmd2.ReadFromString(s1);
   cmd2.SaveToString(s2, false);

   DOUT1(("CMD2: \n%s", s2.c_str()));

   DOUT1(("Same = %s", DBOOL(s1==s2)));

   return;
*/



//   TestCmdSet(5, false, 3, true);
//   return;


   TestCmdChain(20, true); // with timeout

   TestCmdChain(20, false); // without timeout and same command

   TestCmdChain(20, false, false); // without timeout and every time new command


   return;


   TestCmdSet(5, true);

   TestCmdSet(10, false);

   TestCmdSet(15, true);

}


extern "C" void RunTimeTest()
{
   dabc::TimeStamp ttt;


   timespec tm;
   clock_gettime(CLOCK_MONOTONIC, &tm);

   dabc::TimeStamp tm1 = dabc::Now();

   long long int time0 = tm.tv_sec*1000000000LL + tm.tv_nsec;

   long long int time1 = 0;

   const int nrepeat = 1000000;

   for (int n=0;n<nrepeat;n++) {
//     abc = dabc::Now();
      clock_gettime(CLOCK_MONOTONIC, &tm);
   }
   time1 = tm.tv_sec*1000000000LL + tm.tv_nsec - time0;

   dabc::TimeStamp tm2 = dabc::Now();

   dabc::TimeStamp tm3 = dabc::Now();

   dabc::TimeStamp tm4 = ttt.Now();


//   DOUT0(("First %f last %f now %f", arr1[0], arr1[nrepeat-1], oldtm2));

//   printf("First %f Last %f\n", oldtm1, oldtm2);


   dabc::TimeStamp tm5, tm6;

   tm5 = dabc::Now();

   for (int n=0;n<nrepeat;n++) {
      tm6.GetNow();
      // tm6.fValue = dabc::TimeStamp::gFast ? (tm6.GetFastClock() - dabc::TimeStamp::gFastClockZero) * dabc::TimeStamp::gFastClockMult : 0;
//      arr2[n] = tm6();
   }

//   tm6.GetNow();

//   DOUT0(("First %f last %f now %f", arr2[0], arr2[nrepeat-1], tm6()));


   double dist1 = tm2 - tm1;
   double dist2 = time1*1e-9;
   double dist3 = tm4 - tm3;
   double dist3old = 0.;
   double dist4 = tm6 - tm5;

   DOUT0(("Slow Time = sec %d nsec = %ld", tm.tv_sec, tm.tv_nsec));
   DOUT0(("Interval: slow %6.5f fast %6.5f ", dist1, dist2));
   if ((dist1/dist2 > 1.05) || (dist1/dist2 < 0.95))
      EOUT(("Too big difference between slow and fast time measurement"));

   DOUT0(("Speed (microsec per measurement): slow %4.3f fast %4.3f %4.3f new %4.3f", dist2/nrepeat*1e6, dist3/nrepeat*1e6, dist3old/nrepeat*1e6, dist4/nrepeat*1e6));
}



extern "C" void RunAllTests()
{
   RunCoreTest();

   RunCmdTest();

   RunTimeTest();
}

extern "C" void RunHeavyTest()
{
   for (int n=10;n<200;n+=10)
      TestChain(true, n, 0, 1.);
}


extern "C" void RunPoolTest()
{
/**   {
      DOUT0(("Test standalone buffer"));
      dabc::Buffer buf1 = dabc::Buffer::CreateBuffer(4096);

      DOUT0(("Full size = %u", buf1.GetTotalSize()));

      dabc::Buffer buf2 = buf1;

      DOUT0(("Full size = %u %u", buf1.GetTotalSize(), buf2.GetTotalSize()));

      dabc::Buffer buf3 = buf1;

      DOUT0(("Full size = %u %u %u", buf1.GetTotalSize(), buf2.GetTotalSize(), buf3.GetTotalSize()));
   }
*/

   dabc::MemoryPool pool1("pool1", false);

   {

      pool1.Allocate(0x1000, 1000, 2);


      dabc::Buffer buf1;
      buf1 = pool1.TakeBuffer(0x4000);

      DOUT0(("buf1 = %p", &buf1));

      DOUT0(("Full size = %u numsegm %u", buf1.GetTotalSize(), buf1.NumSegments()));

      dabc::Buffer buf2 = buf1.Duplicate();

      DOUT0(("Full size1 = %u size2 = %u", buf1.GetTotalSize(), buf2.GetTotalSize()));

      unsigned cnt(0);
      dabc::Buffer hdr;
      dabc::Pointer ptr = buf2.GetPointer();

      while (!(hdr = buf2.GetNextPart(ptr, 16)).null()) cnt++;

      DOUT0(("Get %u number of small parts, expected %u", cnt, 0x4000/16));

      ptr = buf2.GetPointer();

      hdr = buf2.GetNextPart(ptr, 16);
      hdr.CopyFromStr("First second");

      dabc::Buffer hdr2 = buf2.GetNextPart(ptr, 16);

      hdr2.CopyFromStr(" in between");
      hdr2.SetTotalSize(11);

      hdr.Insert(5, hdr2);

      DOUT0(("Result of insertion is: %s", hdr.AsStdString().c_str()));

      const char* mystr = "This is example of external buffer";

      buf1 = dabc::Buffer::CreateBuffer(mystr, strlen(mystr), false);

      DOUT0(("Ext buffer: %s", buf1.AsStdString().c_str()));

      DOUT0((" ============================= create queue ======================= "));

      dabc::BuffersQueue queue(50);
      for (int n=0;n<50;n++)
         queue.Push(pool1.TakeBuffer(0x4000));

      DOUT0(("1.Size = %u", queue.ItemRef(5).GetTotalSize()));

      buf1 = queue.ItemRef(5);

      DOUT0(("2.Size = %u", queue.ItemRef(5).GetTotalSize()));

      buf1 = queue.ItemRef(5).Duplicate();

      DOUT0(("3.Size = %u", queue.ItemRef(5).GetTotalSize()));

      buf1 = queue.ItemRef(5);

      DOUT0(("4.Size = %u", queue.ItemRef(5).GetTotalSize()));

      queue.Cleanup();

      DOUT0((" ============================= cleanup queue ======================= "));

   }

   pool1.Release();

   DOUT0(("Pool test done"));
}

// ===================================== CPP test =================================

/** Prototype of stack buffer - all fields are in stack, need to deep copy all of them by assign operator */
class SBuffer {
   protected:
      int fField;
   public:
      SBuffer() : fField(0) { DOUT0(("SBuffer Default constructor")); }

      SBuffer(const SBuffer& src) : fField(src.fField) { DOUT0(("SBuffer Copy constructor")); }

      virtual ~SBuffer() { DOUT0(("SBuffer Destructor")); }

      SBuffer& operator=(const SBuffer& src)
      {
         DOUT0(("operator=(const SBuffer& src)"));
         fField = src.fField; return *this;
      }

      SBuffer& operator=(SBuffer& src)
      {
         DOUT0(("operator=(SBuffer& src)"));
         fField = src.fField;
         src.fField = 0;
         return *this;
      }

      SBuffer& operator<<(SBuffer& src)
      {
         DOUT0(("operator<<(SBuffer& src)"));
         fField = src.fField;
         src.fField = 0;
         return *this;
      }

      SBuffer& operator<<(const SBuffer& src)
      {
         DOUT0(("operator<<(const SBuffer& src)"));
         fField = src.fField;
         return *this;
      }
};

struct DBufferRec {
   int fRefCnt;
   int fField1;
   int fField2;
   int fField3;
   void incref() { fRefCnt++; }
   bool decref() { return --fRefCnt<=0; }
};

class DBuffer {
   protected:
      DBufferRec* fRec;

      void close()
      {
         if (fRec && fRec->decref()) free(fRec);
         fRec = 0;
      }

   public:
      DBuffer() : fRec(0) { DOUT0(("DBuffer Default constructor")); }

      DBuffer(unsigned extrasz) : fRec(0)
      {
         DOUT0(("DBuffer normal constructor"));
         allocate(extrasz);
      }

      DBuffer(const DBuffer& src) : fRec(src.fRec)
      {
         DOUT0(("DBuffer Copy constructor"));
         if (fRec) fRec->incref();
      }

      virtual ~DBuffer()
      {
         DOUT0(("DBuffer Destructor"));
         close();
      }

      void allocate(unsigned extrasz = 0)
      {
         close();
         fRec = (DBufferRec*) malloc(sizeof(DBufferRec) + extrasz);
         fRec->fRefCnt = 1;
      }

      DBuffer& operator=(const DBuffer& src)
      {
         close();
         DOUT0(("operator=(const DBuffer& src)"));
         fRec = src.fRec;
         if (fRec) fRec->incref();
         return *this;
      }

      DBuffer& operator<<(DBuffer& src)
      {
         DOUT0(("operator<<(DBuffer& src)"));
         close();
         fRec = src.fRec;
         src.fRec = 0;
         return *this;
      }

      DBuffer& operator<<(const DBuffer& src)
      {
         DOUT0(("operator<<(const DBuffer& src)"));
         return operator=(src);
      }
};


SBuffer SFunc1()
{
   DOUT0(("Enter SFunc1"));
   SBuffer res;
   DOUT0(("Return from SFunc1"));
   return res;
}

SBuffer SFunc2()
{
   DOUT0(("Enter and return from SFunc2"));
   return SFunc1();
}

DBuffer DFunc1()
{
   DOUT0(("Enter DFunc1"));
   DBuffer res(16);
   DOUT0(("Return from DFunc1"));
   return res;
}

DBuffer DFunc2()
{
   DOUT0(("Enter and return from DFunc2"));
   return DFunc1();
}



extern "C" void RunCPPTest()
{
   {
   DOUT0(("***** Calling SBuffer res = SFunc1() ****** "));

   SBuffer res = SFunc1();

   DOUT0(("***** Calling SBuffer res2; res2 = SFunc1() ****** "));

   SBuffer res2; res2 = SFunc1();

   DOUT0(("***** Calling SBuffer res3 = SFunc2() ****** "));

   SBuffer res3 = SFunc2();

   DOUT0(("***** Calling SBuffer res4; res4 = SFunc2() ****** "));

   SBuffer res4; res4 = SFunc2();

   DOUT0(("***** Calling res4 = res2; ****** "));

   res4 = res2;

   DOUT0(("***** Calling res << res3; ****** "));

   res << res3;

   DOUT0(("***** Calling res << SFunc1(); ****** "));

   res << SFunc1();

   DOUT0(("***** Leaving SBuffer part ****** "));

   }

   {
      DOUT0(("***** Calling DBuffer res = DFunc1() ****** "));

      DBuffer res = DFunc1();

      DOUT0(("***** Calling DBuffer res2; res2 = DFunc1() ****** "));

      DBuffer res2; res2 = DFunc1();

      DOUT0(("***** Leaving DBuffer part ****** "));


   }
}

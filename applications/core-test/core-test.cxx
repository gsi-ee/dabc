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
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"
#include "dabc/statistic.h"
#include "dabc/Timer.h"
#include "dabc/WorkingThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/CommandsSet.h"

#define BUFFERSIZE 1024
#define QUEUESIZE 5

long int fGlobalCnt = 0;

class TestModuleAsync : public dabc::ModuleAsync {
   protected:
      int                 fKind; // 0 - first in chain, 1 - repeater, 2 - last in chain,
      long                fCounter;

   public:
      TestModuleAsync(const char* name, int kind) :
         dabc::ModuleAsync(name),
         fKind(kind)
      {
         CreatePoolHandle("Pool", BUFFERSIZE, 5);

         if (fKind>0) CreateInput("Input", Pool(), QUEUESIZE);

         if (fKind<2) CreateOutput("Output", Pool(), QUEUESIZE);

         fCounter = 0;
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         if (fKind==1)
            if (!Input()->CanRecv() || !Output()->CanSend()) return;

         dabc::Buffer* buf = Input()->Recv();

         if (buf==0) { EOUT(("CCCCCCCCCC")); exit(1); }

         if (fKind==2) {
            dabc::Buffer::Release(buf);
            fCounter++;
         } else
            Output()->Send(buf);

      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         if (!Output()->CanSend()) return;

         if (fKind==0) {
            dabc::Buffer* buf = Pool()->TakeBuffer(BUFFERSIZE);
            if (buf==0) { EOUT(("AAAAAAAAAAAA")); exit(1); }
            Output()->Send(buf);
         } else
         if ((fKind==1) && Input()->CanRecv()) {
            dabc::Buffer* buf = Input()->Recv();
            if (buf==0) { EOUT(("BBBBBBBBBBBB")); exit(1); }
            Output()->Send(buf);
         }
      }

      virtual void AfterModuleStop()
      {
         DOUT3(("TestModuleAsync %s stops %ld", GetName(), fCounter));
         if (fCounter>0) fGlobalCnt = fCounter;
      }
};


class TestModuleSync : public dabc::ModuleSync {
   protected:
      int                 fKind; // 0 - first in chain, 1 - repeater, 2 - last in chain,
      long                fCounter;

   public:
      TestModuleSync(const char* name, int kind) :
         dabc::ModuleSync(name),
         fKind(kind)
      {
         CreatePoolHandle("Pool", BUFFERSIZE, 5);

         if (fKind>0) CreateInput("Input", Pool(), QUEUESIZE);

         if (fKind<2) CreateOutput("Output", Pool(), QUEUESIZE);

         fCounter = 0;
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
            dabc::Buffer* buf = TakeBuffer(Pool(), BUFFERSIZE);
            Send(Output(), buf);
            fCounter++;
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
            dabc::Buffer* buf = Recv(Input());
            dabc::Buffer::Release(buf);
            fCounter++;
         }
      }

      virtual void AfterModuleStop()
      {
         if (fCounter>0) fGlobalCnt = fCounter;
      }
};

void TestChain(bool isM, int number, int testkind = 0)
{
   DOUT0(("==============================================="));
   DOUT0(("Test chain of %d %s modules, using %d threads",
            number, (isM ? "Sync" : "Async"), (testkind==0 ? number : testkind)));


   for (int n=0;n<number;n++) {
      int kind = 1;
      if (n==0) kind = 0; else
      if (n==(number-1)) kind = 2;

      dabc::Module* m = 0;

      if (isM) {
         m = new TestModuleSync(FORMAT(("Module%d",n)), kind);
         dabc::mgr()->MakeThreadForModule(m);
      } else {
         m = new TestModuleAsync(FORMAT(("Module%d",n)), kind);

         switch (testkind) {
            case 1:
               dabc::mgr()->MakeThreadForModule(m, "MainThread");
               break;
            case 2:
              if (n<number/2) dabc::mgr()->MakeThreadForModule(m, "MainThread0");
                         else dabc::mgr()->MakeThreadForModule(m, "MainThread1");
              break;
            default:
              dabc::mgr()->MakeThreadForModule(m);
         }
      }
   }

   bool connectres = true;

   for (int n=1; n<number; n++)
      if (!dabc::mgr()->ConnectPorts( FORMAT(("Module%d/Output", n-1)),
                         FORMAT(("Module%d/Input", n)))) connectres = false;

   if (!connectres)
     EOUT(("Ports are not connect"));
   else {
      dabc::CpuStatistic cpu;

      fGlobalCnt = 0;

      dabc::SetDebugLevel(1);

      dabc::mgr()->StartAllModules();
      dabc::TimeStamp_t tm1 = TimeStamp();

      dabc::SetDebugLevel(1);

      cpu.Reset();

      dabc::ShowLongSleep("Main loop", 5);

      cpu.Measure();

//      dabc::SetDebugLevel(5);

      dabc::mgr()->StopAllModules();

      dabc::TimeStamp_t tm2 = TimeStamp();

      if (fGlobalCnt<=0) fGlobalCnt = 1;

      DOUT1(("IsM = %s Kind = %d Time = %5.3f Cnt = %ld Per buffer = %5.3f ms CPU = %5.1f",DBOOL(isM), testkind,
         dabc::TimeDistance(tm1,tm2), fGlobalCnt, dabc::TimeDistance(tm1,tm2)/fGlobalCnt*1e3, cpu.CPUutil()*100.));
   }

   dabc::mgr()->CleanupManager();

   DOUT3(("Did manager cleanup"));
}

class TimeoutTestModuleAsync : public dabc::ModuleAsync {
   protected:
      long                fCounter1;
      long                fCounter2;

   public:
      TimeoutTestModuleAsync(const char* name) :
         dabc::ModuleAsync(name),
         fCounter1(0),
         fCounter2(0)
      {
         CreateTimer("Timer1", 0.001, true);
         CreateTimer("Timer2", 0.001, false);
      }

      virtual void ProcessTimerEvent(dabc::Timer* timer)
      {
         if (timer->IsName("Timer1"))
            fCounter1++;
         else
            fCounter2++;
      }

      virtual void AfterModuleStop()
      {
         DOUT1(("Module %s Timer1 %ld Timer2 %ld", GetName(), fCounter1, fCounter2));
      }

};

void TestTimers(int number)
{
   DOUT0(("==============================================="));
   DOUT0(("Test timers with %d modules, running in the same thread", number));

   for (int n=0;n<number;n++) {
      dabc::Module* m = new TimeoutTestModuleAsync(FORMAT(("Module%d",n)));

      dabc::mgr()->MakeThreadForModule(m, "MainThread");
   }

   dabc::CpuStatistic cpu;

   dabc::mgr()->StartAllModules();

   dabc::TimeStamp_t tm1 = TimeStamp();

   cpu.Reset();

   dabc::ShowLongSleep("Main loop", 5);

   cpu.Measure();

   dabc::mgr()->StopAllModules();

   dabc::TimeStamp_t tm2 = TimeStamp();

   DOUT1(("Overall time = %3.2f CPU = %5.1f", dabc::TimeDistance(tm1,tm2), cpu.CPUutil()*100.));

   dabc::mgr()->CleanupManager();

   DOUT3(("Did manager cleanup"));
}

void TestMemoryPool()
{
   unsigned bufsize = 2048;
   unsigned numbuf = 2048;
   unsigned headersize = 20;

   int numtests = 1000000;

   dabc::MemoryPool* mem_pool = new dabc::MemoryPool(dabc::mgr(), "Test");

   mem_pool->UseMutex();
   mem_pool->SetMemoryLimit(0); // endless
   mem_pool->SetCleanupTimeout(0.1);

   mem_pool->AllocateMemory(bufsize, numbuf, numbuf / 2);
   mem_pool->AllocateMemory(bufsize * 16, numbuf, numbuf / 2);
   mem_pool->AllocateReferences(headersize, numbuf, numbuf); // can increase

   DOUT1(("Manager thread = %p", dabc::mgr()->ProcessorThread()));
   mem_pool->AssignProcessorToThread(dabc::mgr()->ProcessorThread());
   DOUT1(("Pool thread = %p", mem_pool->ProcessorThread()));

   dabc::TimeStamp_t tm1, tm2;


   tm1 = TimeStamp();
   for (int n=0;n<numtests;n++) {
      dabc::Buffer* buf = mem_pool->TakeBuffer(bufsize, 0);
      dabc::Buffer::Release(buf);
   }
   tm2 = TimeStamp();
   DOUT1(("Simple get without headers %5.3f, per buffer %4.0f nano", dabc::TimeDistance(tm1,tm2), dabc::TimeDistance(tm1,tm2)/numtests*1e9));

   tm1 = TimeStamp();
   for (int n=0;n<numtests;n++) {
      dabc::Buffer* buf = mem_pool->TakeBuffer(bufsize, headersize);
      dabc::Buffer::Release(buf);
   }
   tm2 = TimeStamp();
   DOUT1(("Simple get with headers %5.3f, per buffer %4.0f nano", dabc::TimeDistance(tm1,tm2), dabc::TimeDistance(tm1,tm2)/numtests*1e9));


   tm1 = TimeStamp();

//   double sum1, sum2;

   for (int n=0;n<1000000;n++) {
//      dabc::TimeStamp_t t1 = TimeStamp();
      dabc::Buffer* buf = mem_pool->TakeBuffer(bufsize*16, headersize);
//      dabc::TimeStamp_t t2 = TimeStamp();
      dabc::Buffer::Release(buf);
//      dabc::TimeStamp_t t3 = TimeStamp();
//      sum1 += (t2-t1);
//      sum2 += (t3-t2);
   }
   tm2 = TimeStamp();
   DOUT1(("Simple get of large buffers %5.3f", dabc::TimeDistance(tm1,tm2)));
//   DOUT1(("Take %5.1f Release %5.1f", sum1, sum2));

//   delete mem_pool;
//   return;


   #define BlockSize 100
   dabc::Buffer* block[BlockSize];

   tm1 = TimeStamp();
   for (int k=0;k<1000000/BlockSize;k++) {
      for (int n=0;n<BlockSize;n++)
         block[n] = mem_pool->TakeBuffer(bufsize, headersize);
      for (int n=0;n<BlockSize;n++)
         dabc::Buffer::Release(block[n]);
   }
   tm2 = TimeStamp();
   DOUT1(("Block get of simple buffers %5.3f", dabc::TimeDistance(tm1,tm2)));


   #define BigBlockSize 10000
   dabc::Buffer* bigblock[BigBlockSize];

//   mem_pool->AllocateMemory(bufsize, BigBlockSize, 0);
//   mem_pool->AllocateMemory(bufsize * 16, BigBlockSize, 0);

   tm1 = TimeStamp();
   for (int k=0;k<1000000/BigBlockSize;k++) {
      int failcnt = 0;
      for (int n=0;n<BigBlockSize;n++) {
         bigblock[n] = mem_pool->TakeBuffer(bufsize, headersize);
         if (bigblock[n]==0) failcnt++;
      }

      if (failcnt>0) EOUT(("Failed requests = %d", failcnt));

      for (int n=0;n<BigBlockSize;n++)
         dabc::Buffer::Release(bigblock[n]);

   }
   tm2 = TimeStamp();
   DOUT1(("Large block get of simple buffers %5.3f", dabc::TimeDistance(tm1,tm2)));

   sleep(2);

   DOUT1(("Did sleep"));

   delete mem_pool;
}


extern "C" void RunCoreTest()
{
//   TestMemoryPool();

   TestChain(true, 10);

   TestChain(false, 10, 0);
   TestChain(false, 10, 2);
   TestChain(false, 10, 1);

   TestTimers(1);
   TestTimers(3);
   TestTimers(10);
}



class TestModuleCmd : public dabc::ModuleAsync {
   protected:
      int                 fNext; // -1 reply, or number of next in chain
      int                 fCount;

   public:
      TestModuleCmd(const char* name, int next) :
         dabc::ModuleAsync(name),
         fNext(next),
         fCount(0)
      {
      }

      virtual int ExecuteCommand(dabc::Command* cmd)
      {
         DOUT0(("Module %s execute command %s Exequeue %u", GetName(), cmd->GetName(), fProcessorExeCommands.Size()));

         if (fNext<0) return dabc::cmd_true;

         if (fCount++>10) return dabc::cmd_true;

         std::string nextname = dabc::format("Module%d", fNext);
         dabc::Module* next = dabc::mgr()->FindModule(nextname.c_str());
         if (next==0) return dabc::cmd_false;

         int res = ExecuteIn(next, "MyCmd");

         return res>0 ? res+1 : 0;
      }
};


void TestCmdChain(int number)
{
   DOUT0(("==============================================="));
   DOUT0(("Test cmd chain of %d modules", number));

   dabc::Module* m0 = 0;

   for (int n=0;n<number;n++) {
      dabc::Module* m = new TestModuleCmd(FORMAT(("Module%d",n)), n==number-1 ? 0 : n+1);

      dabc::mgr()->MakeThreadForModule(m, n<number/2 ? "MainThread0" : "MainThread1");

      if (n==0) m0 = m;
   }

   dabc::mgr()->StartAllModules();

   DOUT0(("==============================================="));
   DOUT0(("Inject command m0 = %p %s", m0, m0->GetName()));

   int res = m0->Execute("MyCommand",2);

   DOUT0(("Execution result = %d", res));

   dabc::mgr()->StopAllModules();

   for (int n=0;n<number;n++)
      dabc::mgr()->DeleteModule(FORMAT(("Module%d",n)));

   dabc::mgr()->CleanupManager();

   DOUT0(("Did manager cleanup"));
}

void TestCmdSet(int number, bool sync)
{
   DOUT0(("==============================================="));
   DOUT0(("Test cmd set with %d modules", number));

   for (int n=0;n<number;n++) {
      dabc::Module* m = new TestModuleCmd(FORMAT(("SetModule%d",n)), -1);

      dabc::mgr()->MakeThreadForModule(m, (n % 2) ? "SetThread0" : "SetThread1");
   }

   dabc::mgr()->StartAllModules();

   DOUT0(("==============================================="));
   DOUT0(("Create set"));

   dabc::CommandsSet* set = new dabc::CommandsSet;

   for (int n=0;n<number;n++) {
      dabc::Command* cmd = new dabc::Command("MyCommand");
      SetCmdReceiver(cmd, FORMAT(("SetModule%d",n)));
      set->Add(cmd);
   }

   DOUT0(("Inject set"));

   if (sync) {
      int res = set->ExecuteSet(2.);
      delete set;
      DOUT0(("SET result = %d", res));
   } else {
      set->SubmitSet(0, 2.);
      dabc::mgr()->Sleep(1.);
      DOUT0(("Sleep DONE"));
   }

   dabc::mgr()->StopAllModules();

   for (int n=0;n<number;n++)
      dabc::mgr()->DeleteModule(FORMAT(("SetModule%d",n)));

   dabc::mgr()->CleanupManager();

   DOUT0(("Did manager cleanup"));
}


extern "C" void RunCmdTest()
{
//   TestMemoryPool();

   TestCmdChain(10);

   TestCmdChain(10);

   TestCmdSet(5, true);

   TestCmdSet(10, false);

   TestCmdSet(15, true);

}


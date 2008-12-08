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

#include <queue>

#define BUFFERSIZE 1024
#define QUEUESIZE 5

long int fGlobalCnt = 0;

class TestModuleAsync : public dabc::ModuleAsync {
   protected:
      int                 fKind; // 0 - first in chain, 1 - repeater, 2 - last in chain,
      dabc::PoolHandle* fPool;
      dabc::Port*         fInput;
      dabc::Port*         fOutput;
      long                fCounter;


   public:
      TestModuleAsync(const char* name, int kind) :
         dabc::ModuleAsync(name),
         fKind(kind),
         fPool(0),
         fInput(0),
         fOutput(0)
      {
         fPool = CreatePool("Pool", 5, BUFFERSIZE);

         if (fKind>0)
            fInput = CreateInput("Input", fPool, QUEUESIZE);

         if (fKind<2)
            fOutput = CreateOutput("Output", fPool, QUEUESIZE);

         fCounter = 0;
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer* buf = 0;

         if (fKind==1)
            if (fInput->InputBlocked() || fOutput->OutputBlocked()) return;

         fInput->Recv(buf);

         if (buf==0) { EOUT(("CCCCCCCCCC")); exit(1); }

         if (fKind==2) {
            dabc::Buffer::Release(buf);
            fCounter++;
         } else
            fOutput->Send(buf);

      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         if (fKind==0) {
            dabc::Buffer* buf = fPool->TakeBuffer(BUFFERSIZE, false);
            if (buf==0) { EOUT(("AAAAAAAAAAAA")); exit(1); }
            fOutput->Send(buf);
         } else
         if ((fKind==1) && !fInput->InputBlocked()) {
            dabc::Buffer* buf = 0;
            fInput->Recv(buf);
            if (buf==0) { EOUT(("BBBBBBBBBBBB")); exit(1); }
            fOutput->Send(buf);
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
      dabc::PoolHandle* fPool;
      dabc::Port*         fInput;
      dabc::Port*         fOutput;
      long                fCounter;

   public:
      TestModuleSync(const char* name, int kind) :
         dabc::ModuleSync(name),
         fKind(kind),
         fPool(0),
         fInput(0),
         fOutput(0)
      {
         fPool = CreatePool("Pool", 5, BUFFERSIZE);

         if (fKind>0)
            fInput = CreateInput("Input", fPool, QUEUESIZE);

         if (fKind<2)
            fOutput = CreateOutput("Output", fPool, QUEUESIZE);

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
         while (TestWorking()) {
            dabc::Buffer* buf = TakeBuffer(fPool, BUFFERSIZE);
            Send(fOutput, buf);
            fCounter++;
         }
      }

      void RepeaterLoop()
      {
         while (TestWorking())
            Send(fOutput, Recv(fInput));
//            Send(fOutput, RecvFromAny());
      }

      void ReceiverLoop()
      {
         while (TestWorking()) {
            dabc::Buffer* buf = Recv(fInput);
//            dabc::Buffer* buf = RecvFromAny();
            dabc::Buffer::Release(buf);
            fCounter++;
         }
      }

      virtual void AfterModuleStop()
      {
         if (fCounter>0) fGlobalCnt = fCounter;
      }
};

void TestChain(dabc::Manager* mgr, bool isM, int number, int testkind = 0)
{
   for (int n=0;n<number;n++) {
      int kind = 1;
      if (n==0) kind = 0; else
      if (n==(number-1)) kind = 2;

      dabc::Module* m = 0;

      if (isM) {
         m = new TestModuleSync(FORMAT(("Module%d",n)), kind);
         mgr->MakeThreadForModule(m);
      } else {
         m = new TestModuleAsync(FORMAT(("Module%d",n)), kind);

         switch (testkind) {
            case 1:
              if (n<number/2) mgr->MakeThreadForModule(m, "MainThread0");
                         else mgr->MakeThreadForModule(m, "MainThread1");
              break;
            case 2:
               mgr->MakeThreadForModule(m);
               break;
            default:
               mgr->MakeThreadForModule(m, "MainThread");
               break;
         }
      }
   }

   mgr->CreateMemoryPools();
   DOUT3(("Creare memory pools"));

   bool connectres = true;

   for (int n=1; n<number; n++) {
      if (!mgr->ConnectPorts( FORMAT(("Module%d/Output", n-1)),
                         FORMAT(("Module%d/Input", n)))) connectres = false;
   }

   if (!connectres)
     EOUT(("Ports are not connect"));
   else {
      DOUT1(("Connect ports done"));
      dabc::CpuStatistic cpu;

      fGlobalCnt = 0;

      dabc::SetDebugLevel(1);

      mgr->StartAllModules();
      dabc::TimeStamp_t tm1 = TimeStamp();

      dabc::SetDebugLevel(1);
      DOUT1(("Start all modules done"));

      cpu.Reset();

      dabc::ShowLongSleep("Main loop", 5);

      cpu.Measure();

//      dabc::SetDebugLevel(5);

      mgr->StopAllModules();

      dabc::TimeStamp_t tm2 = TimeStamp();

      if (fGlobalCnt<=0) fGlobalCnt = 1;

      DOUT1(("IsM = %s Kind = %d Time = %5.3f Cnt = %ld Per buffer = %5.3f ms CPU = %5.1f",DBOOL(isM), testkind,
         dabc::TimeDistance(tm1,tm2), fGlobalCnt, dabc::TimeDistance(tm1,tm2)/fGlobalCnt*1e3, cpu.CPUutil()*100.));
   }

   mgr->CleanupManager();

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
         CreateTimer("Timer1", 0.01, true);
         CreateTimer("Timer2", 0.1, false);
      }

      virtual void ProcessTimerEvent(dabc::Timer* timer)
      {
         if (timer->IsName("Timer1"))
            fCounter1++;
         else
            fCounter2++;
      }
};

void TestTimers(dabc::Manager* mgr, int number)
{
   for (int n=0;n<number;n++) {
      dabc::Module* m = 0;

      m = new TimeoutTestModuleAsync(FORMAT(("Module%d",n)));

      mgr->MakeThreadForModule(m, "MainThread");
   }

   dabc::CpuStatistic cpu;

   mgr->CreateMemoryPools();

   mgr->StartAllModules();

   dabc::TimeStamp_t tm1 = TimeStamp();

   cpu.Reset();

   dabc::ShowLongSleep("Main loop", 5);

   cpu.Measure();

   mgr->StopAllModules();

   dabc::TimeStamp_t tm2 = TimeStamp();

   DOUT1(("Overall time = %3.2f CPU = %5.1f", dabc::TimeDistance(tm1,tm2), cpu.CPUutil()*100.));

   mgr->CleanupManager();

   DOUT3(("Did manager cleanup"));
}

void TestNewThreads(dabc::Manager* mgr)
{
   dabc::WorkingThread* thrd = new dabc::WorkingThread(mgr->GetThreadsFolder(true), "Thread1");

   dabc::WorkingProcessor* tm = 0;

   int nrepeat = 3;

   for (int n=0;n<nrepeat;n++) {
      thrd->Start();
      if (tm==0) {
         tm = new dabc::WorkingProcessor();
         tm->AssignProcessorToThread(thrd);
      }

      tm->ActivateTimeout(0.2);
      thrd->Sync();
      dabc::LongSleep(1);

      if (n == nrepeat - 1) {
         delete tm; tm = 0;
         dabc::MicroSleep(10000);
      }

      thrd->Stop(false);
   }

   delete thrd;
}

#include <map>

void TestMap()
{
   std::map<uint32_t, void*> mm;

   uint32_t numrec = 10;

   for (uint32_t n=1;n<numrec;n++)
      mm[n << 4] = &mm;

   void* res = 0;

   for (uint32_t n=1;n<numrec;n++)
      res = mm[n << 4];

   double min(10000), max(0);

   for(uint32_t n=1;n<numrec;n++) {
      int ntry = 100;

      dabc::TimeStamp_t tm1 = TimeStamp(), tm2;

      for (int k=0;k<ntry;k++)
         res = mm[n << 4];

      tm2 = TimeStamp();

      double v = dabc::TimeDistance(tm1,tm2)*1e6/ntry;

//      DOUT0(("Value %u Time %5.3f", n, v));
      if (v>max) max = v;
      if (v<min) min = v;
   }
   DOUT0(("Minimum %5.3f  Maximum %5.3f", min, max));

   DOUT0(("Outofband value = %u", mm[0]));

   if (res==0);
}

void TestMemoryPool(dabc::Manager& mgr)
{
   unsigned bufsize = 2048;
   unsigned numbuf = 2048;
   unsigned headersize = 20;

   int numtests = 1000000;

   dabc::MemoryPool* mem_pool = new dabc::MemoryPool(mgr.GetPoolsFolder(true), "Test");

   mem_pool->UseMutex();
   mem_pool->SetMemoryLimit(0); // endless
   mem_pool->SetCleanupTimeout(0.1);

   mem_pool->AllocateMemory(bufsize, numbuf, numbuf / 2);
   mem_pool->AllocateMemory(bufsize * 16, numbuf, numbuf / 2);
   mem_pool->AllocateReferences(headersize, numbuf, numbuf); // can increase

   DOUT1(("Manager thread = %p", mgr.ProcessorThread()));
   mem_pool->AssignProcessorToThread(mgr.ProcessorThread());
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

void TestLoop()
{
   #define ArrSize  10000
   char arr[ArrSize];

   for (int n=0;n<ArrSize;n++)
     arr[n] = n % 256;

   dabc::TimeStamp_t tm1, tm2;

   unsigned cnt1;
   uint16_t cnt4;
   uint32_t cnt2;
   uint64_t cnt3;

   tm1 = TimeStamp();

   for (int k=0;k<100000;k++) {
       cnt1=0;
       for (unsigned n=0;n<ArrSize;n++) {
          if (arr[n]!=0) cnt1++;
       }
   }
   tm2 = TimeStamp();
   DOUT1(("Unsigned loop %5.3f cnt1 %u", dabc::TimeDistance(tm1,tm2), cnt1));


   tm1 = TimeStamp();
   for (int k=0;k<100000;k++) {
       cnt4=0;
       for (uint16_t n=0;n<ArrSize;n++) {
          if (arr[n]!=0) cnt4++;
       }
   }
   tm2 = TimeStamp();
   DOUT1(("Uint16_t loop %5.3f cnt4 %u", dabc::TimeDistance(tm1,tm2), cnt4));

   tm1 = TimeStamp();
   for (int k=0;k<100000;k++) {
       cnt2=0;
       for (uint32_t n=0;n<ArrSize;n++) {
          if (arr[n]!=0) cnt2++;
       }
   }
   tm2 = TimeStamp();
   DOUT1(("Uint32_t loop %5.3f cnt2 %u", dabc::TimeDistance(tm1,tm2), cnt2));

   tm1 = TimeStamp();
   for (int k=0;k<100000;k++) {
       cnt3=0;
       for (uint64_t n=0;n<ArrSize;n++) {
          if (arr[n]!=0) cnt3++;
       }
   }
   tm2 = TimeStamp();
   DOUT1(("Uint64_t loop %5.3f cnt3 %u", dabc::TimeDistance(tm1,tm2), cnt3));

}

void TestQueues()
{
   dabc::Queue<int> q1(1024, true);
   std::queue<int> q2;

   dabc::TimeStamp_t t1, t2;

   t1 = TimeStamp();
   for (int n=0;n<10000;n++) {
      for (int k=0;k<1000;k++);
      for (int k=0;k<1000;k++);
   }
   t2 = TimeStamp();

   DOUT1(("NULL Time %5.3f", dabc::TimeDistance(t1, t2)));

   t1 = TimeStamp();

   for (int n=0;n<10000;n++) {
      for (int k=0; k<1000; k++)
        q2.push(k);
      for (int k=0; k<1000; k++) {
         q2.front();
         q2.pop();
      }
   }

   t2 = TimeStamp();

   DOUT1(("STD Size %d Time %5.3f", q2.size(), dabc::TimeDistance(t1, t2)));

   t1 = TimeStamp();

   for (int n=0;n<10000;n++) {
      for (int k=0; k<1000; k++)
        q1.Push(k);
      for (int k=0; k<1000; k++)
         q1.Pop();
   }

   t2 = TimeStamp();


   DOUT1(("DABC Size %d Capacity %d Time %5.3f", q1.Size(), q1.Capacity(), dabc::TimeDistance(t1, t2)));

/*
   q1.Reset();
   q1.Push(0);
   q1.Push(1);
   q1.Push(1);
   q1.Push(2);
   DOUT1(("Size = %d", q1.Size()));
   q1.Remove(1);
   DOUT1(("Size = %d", q1.Size()));

   while (q1.Size()>0)
     DOUT1(("Pop = %d", q1.Pop()));
*/

}

#include <dirent.h>
#include <fnmatch.h>


void TestScanDir(const char* dirname, const char* pattern = 0)
{
   struct dirent **namelist;
   int n;

   n = scandir(dirname, &namelist, 0, 0);
   if (n < 0)
       perror("scandir");
   else {
       while(n--) {
          if ((pattern==0) || (fnmatch(pattern, namelist[n]->d_name, FNM_NOESCAPE)==0))
              printf("%s\n", namelist[n]->d_name);
           free(namelist[n]);
       }
       free(namelist);
   }

}

int main(int numc, char* args[])
{
//   if (numc <= 1) return 0;
//   TestScanDir(args[1], (numc>2) ? args[2] : 0);
//   return 0;

   dabc::Manager mgr("local");

   dabc::SetDebugLevel(1);

//   for(int n=0;n<10;n++)
//     TestChain(&mgr, false, 10, 0);
//   return 0;

//   TestLoop();
//   return 0;

//   TestQueues();
//   return 0;

   TestMemoryPool(mgr);
//   return 0;

//   TestMap();
//   return 0;

//   TestNewThreads(&mgr);
//   return 0;

   TestChain(&mgr, true, 10);

   TestChain(&mgr, false, 10, 2);
   TestChain(&mgr, false, 10, 1);
   TestChain(&mgr, false, 10, 0);

   TestTimers(&mgr, 1);
   TestTimers(&mgr, 3);
   TestTimers(&mgr, 10);

   return 0;
}




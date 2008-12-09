#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/Command.h"
#include "dabc/StandaloneManager.h"
#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/threads.h"
#include "dabc/Application.h"
#include "dabc/SocketDevice.h"
#include "dabc/statistic.h"


#include <map>
#include <math.h>
#include <queue>

const int TestBufferSize = 128*1024;
const int TestSendQueueSize = 5;
const int TestRecvQueueSize = 10;
const int TestUseAkcn = true;

const int FirstNode = 0;

//#include <iostream>

class Test1SendModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool;
      bool                fCanSend;
      dabc::Ratemeter     fSendRate;
      unsigned            fPortCnt;

   public:
      Test1SendModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         int nports = cmd->GetInt("NumPorts", 3);
         int buffsize = cmd->GetInt("BufferSize", 16*1024);

         fPool = CreatePool("SendPool", 5, buffsize);

         for (int n=0;n<nports;n++)
            CreateOutput(FORMAT(("Output%d", n)), fPool, TestSendQueueSize);

         fCanSend = false;
         fPortCnt = 0;

         DOUT1(("new TSendModule %s nports = %d buf = %d req = %d done", GetName(), NumOutputs(), buffsize, fPool->GetRequiredBuffersNumber()));
      }

      int ExecuteCommand(dabc::Command* cmd)
      {
         if (cmd->IsName("EnableSending")) {
            fCanSend = cmd->GetBool("Enable", true);
            if (fCanSend) StartSending();
            fPortCnt = 0;
            return cmd_true;
         }

         return ModuleAsync::ExecuteCommand(cmd);
      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         bool fullchaotic = false;

         if (!fCanSend) return;

         if (!fullchaotic && (Output(fPortCnt) != port)) return;

         bool tryagain = true;

         int cnt=0;

         do {
            if (cnt++> 100) { EOUT(("AAAAAAAAAAA")); }

            tryagain = false;
            dabc::Buffer* ref = fPool->TakeBuffer(0, false);
            if (ref==0) { EOUT(("AAAAAAA")); }

            fSendRate.Packet(ref->GetDataSize());

            port->Send(ref);

            if (!fullchaotic) {
               fPortCnt = (fPortCnt+1) % NumOutputs();
               port = Output(fPortCnt);
               tryagain = !port->OutputBlocked();
            }

         } while (tryagain);
      }

      void StartSending()
      {
         for(int n=0;n<TestSendQueueSize;n++)
            for(unsigned nout=0;nout<NumOutputs();nout++) {
               dabc::Buffer* buf = fPool->TakeBuffer(0, false);

               if (buf==0) { EOUT(("AAAAAAAAA")); }

               Output(nout)->Send(buf);
            }
      }

      void AfterModuleStop()
      {
         fCanSend = false;
         DOUT1(("TSendModule finish Rate %5.1f numoper:%7ld", fSendRate.GetRate(), fSendRate.GetNumOper()));
      }
};


class Test1RecvModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool;
      int fSleepTime;
      dabc::Ratemeter fRecvRate;
      dabc::Average fRealSleepTime;

   public:
      Test1RecvModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         // we will use queue (second true) in the signal to detect order of signal fire
         int nports = cmd->GetInt("NumPorts", 3);
         int buffsize = cmd->GetInt("BufferSize", 16*1024);


         // 0 here means that we need at least 1 buffer more for module and force by that pool request scheme
         // 1 is just exactly that we need to work without requests
         fPool = CreatePool("RecvPool", 0, buffsize);

         for (int n=0;n<nports;n++)
            CreateInput(FORMAT(("Input%d", n)), fPool, TestRecvQueueSize);

         DOUT1(("new TRecvModule %s nports:%d buf:%d req:%d", GetName(), nports, buffsize, fPool->GetRequiredBuffersNumber()));

         fSleepTime = 0;
      }

      int ExecuteCommand(dabc::Command* cmd)
      {
         if (cmd->IsName("ChangeSleepTime")) {
            int old = fSleepTime;
            fSleepTime = cmd->GetInt("SleepTime", 0);
            DOUT1(("Sleep:%5d (%5.1f, %5.1f) Rate: %5.1f     new:%5d", old, fRealSleepTime.Mean(), fRealSleepTime.Max(), fRecvRate.GetRate(), fSleepTime));
            fRealSleepTime.Reset();
            fRecvRate.Reset();
         } else
            return ModuleAsync::ExecuteCommand(cmd);

         return cmd_true;
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer* ref = 0;
         port->Recv(ref);
         if (ref==0) { EOUT(("AAAAAAA")); }

//         DOUT1(("Did recv buffer %p", ref));

         fRecvRate.Packet(ref->GetTotalSize());
         dabc::Buffer::Release(ref);

         if (fSleepTime>0) {
            dabc::TimeStamp_t tm1 = TimeStamp();
            dabc::MicroSleep(fSleepTime);
            dabc::TimeStamp_t tm2 = TimeStamp();
            fRealSleepTime.Fill(dabc::TimeDistance(tm1,tm2));
         }
      }

      void AfterModuleStop()
      {
         DOUT1(("TRecvModule finish Rate %5.1f numoper:%7ld", fRecvRate.GetRate(), fRecvRate.GetNumOper()));
      }
};

class Test1WorkerModule : public dabc::ModuleSync {
   protected:

      int fCounter;

   public:
      Test1WorkerModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleSync(name, cmd)
      {
         fCounter = 0;
         SetTmoutExcept(true);
      }

      Test1WorkerModule(const char* name) :
         dabc::ModuleSync(name)
      {
         fCounter = 0;
         SetTmoutExcept(true);
      }

      int ExecuteCommand(dabc::Command* cmd)
      {
         if (cmd->IsName("ResetCounter")) {
            fCounter = 0;
         } else
            return ModuleSync::ExecuteCommand(cmd);

         return cmd_true;
      }

      virtual void MainLoop()
      {
         DOUT1(("TWorkerModule mainloop"));

         while (TestWorking(100)) {
            fCounter++;

            // lets say, about 10 milisecond
            for (int k=0;k<10000/110;k++) {
              // this peace of code takes about 110 microsec
               double sum = 0;
               for (int n=0;n<10000;n++)
                 sum+=sqrt(n*1.);
            }
         }
         DOUT1(("TWorkerModule %s finished cnt=%7d tm %4.1f", GetName(), fCounter, fCounter/100.));
      }
};


class Test1Plugin: public dabc::Application  {
   public:
      Test1Plugin() : dabc::Application("Test1Plugin") { }

      dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
      {
          if ((classname==0) || (cmd==0)) return 0;

          if (strcmp(classname,"Test1SendModule")==0)
             return new Test1SendModule(modulename, cmd);
          else
          if (strcmp(classname,"Test1RecvModule")==0)
             return new Test1RecvModule(modulename, cmd);
          else
          if (strcmp(classname,"Test1WorkerModule")==0)
             return new Test1WorkerModule(modulename, cmd);

          return 0;
      }
};

void CreateAllModules(dabc::StandaloneManager &m, int numworkers = 0)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("Test1RecvModule","Receiver");
      cmd->SetInt("NumPorts", m.NumNodes()-1-FirstNode);
      cmd->SetInt("BufferSize", TestBufferSize);
      m.SubmitRemote(cli, cmd, node);
   }

   for (int node=FirstNode;node<m.NumNodes();node++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("Test1SendModule","Sender");
      cmd->SetInt("NumPorts", m.NumNodes()-1-FirstNode);
      cmd->SetInt("BufferSize", TestBufferSize);
      m.SubmitRemote(cli, cmd, node);
   }

   for (int nw=0;nw<numworkers;nw++)
      for (int node=FirstNode;node<m.NumNodes();node++) {
         dabc::Command* cmd =
            new dabc::CmdCreateModule("Test1WorkerModule", FORMAT(("Worker%d",nw)));
         m.SubmitRemote(cli, cmd, node);
      }

   DOUT1(("Do CreateMemoryPools "));

   for (int node=FirstNode;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdCreateMemoryPools(), node);

   bool res = cli.WaitCommands(5);

   DOUT1(("CreateAllModules() res = %s", DBOOL(res)));
}

void ConnectModules(dabc::StandaloneManager &m, int deviceid = 1)
{
   dabc::CommandClient cli;

   const char* devname = "Test1Dev";

   const char* deviceclass = dabc::typeSocketDevice;
   if (deviceid==2) deviceclass = "verbs::Device";

   for (int node = FirstNode; node < m.NumNodes(); node++)
      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);

   if (!cli.WaitCommands(5)) {
      EOUT(("Cannot create devices of class %s", deviceclass));
      exit(1);
   }

   for (int nsender = FirstNode; nsender < m.NumNodes(); nsender++) {
      for (int nreceiver = FirstNode; nreceiver < m.NumNodes(); nreceiver++) {
          if (nsender==nreceiver) continue;

          std::string port1name, port2name;

          dabc::formats(port1name, "%s$Sender/Output%d", m.GetNodeName(nsender), (nreceiver>nsender ? nreceiver-1 : nreceiver) - FirstNode);
          dabc::formats(port2name, "%s$Receiver/Input%d", m.GetNodeName(nreceiver), (nsender>nreceiver ? nsender-1 : nsender) - FirstNode);

          dabc::Command* cmd =
             new dabc::CmdConnectPorts(port1name.c_str(),
                                          port2name.c_str(),
                                          devname, "TransportThrd");
          cmd->SetBool(dabc::xmlUseAcknowledge, TestUseAkcn);

          m.SubmitCl(cli, cmd);
//          break;
      }
//      break;
   }
   bool res = cli.WaitCommands(5);
   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));
}

void SetPriorities(dabc::StandaloneManager &m, int prio = 0)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++) {
      dabc::Command* cmd = new dabc::Command("SetPriority");
      cmd->SetInt("Priority", prio);
      m.SubmitRemote(cli, cmd, node, "Receiver");
   }
   for (int node=FirstNode;node<m.NumNodes();node++) {
      dabc::Command* cmd = new dabc::Command("SetPriority");
      cmd->SetInt("Priority", prio);
      m.SubmitRemote(cli, cmd, node, "Sender");
   }

   bool res = cli.WaitCommands(1);
   DOUT1(("SetPriorities res = %s", DBOOL(res)));
}

void StartAll(dabc::StandaloneManager &m, int numworkers = 0)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdStartAllModules(), node);

   bool res = cli.WaitCommands(1);
   DOUT1(("StartAll() res = %s", DBOOL(res)));
}

void EnableSending(dabc::StandaloneManager &m, bool on = true)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++) {
      dabc::Command* cmd = new dabc::Command("EnableSending");
      cmd->SetBool("Enable", on);
      m.SubmitRemote(cli, cmd, node, "Sender");
   }

   cli.WaitCommands(3);
}

void ChangeSleepTime(dabc::StandaloneManager &m, int tm=0, int selectnode = -1)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++) {
      if ((selectnode>=0) && (node!=selectnode)) continue;
      dabc::Command* cmd = new dabc::Command("ChangeSleepTime");
      cmd->SetInt("SleepTime", tm);
      m.SubmitRemote(cli, cmd, node, "Receiver");
   }
   cli.WaitCommands(1);
}

void StopAll(dabc::StandaloneManager &m)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdStopAllModules(), node);
   bool res = cli.WaitCommands(5);
   DOUT1(("StopAll res = %s", DBOOL(res)));
}

void CleanupAll(dabc::StandaloneManager &m)
{
   dabc::CommandClient cli;

   for (int node=FirstNode;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdCleanupManager(), node);
   bool res = cli.WaitCommands(5);
   DOUT1(("CleanupAll res = %s", DBOOL(res)));
}


void TestWorker(dabc::StandaloneManager &mgr)
{
   dabc::SetDebugLevel(3);

   Test1WorkerModule* m = new Test1WorkerModule("Combiner");

   mgr.MakeThreadForModule(m, "Thrd1");

   mgr.CreateMemoryPools();

   m->Start();

   DOUT1(("Start called"));

   dabc::ShowLongSleep("Working", 3);
   DOUT1(("Work finished"));

   m->Stop();

   DOUT1(("Stop called"));
}


void CheckLocking()
{
   dabc::TimeStamp_t tm1, tm2;

   dabc::Mutex mutex;
   double cnt = 0;

   for (int k=0;k<10;k++) {
       tm1 = TimeStamp();
       for (unsigned n=0;n<1000;n++) {
          mutex.Lock();
//          dabc::LockGuard guard(mutex);
          cnt+=1.;
          mutex.Unlock();
       }
       tm2 = TimeStamp();
       DOUT1(("Locking 1000 times need %7.6f", dabc::TimeDistance(tm1, tm2)));
   }
}

void CheckIntQueue()
{
   dabc::TimeStamp_t tm1, tm2;

   std::queue<uint64_t> q;

   tm1 = TimeStamp();
   for (unsigned n=0;n<1000;n++)
      q.push(n);
   tm2 = TimeStamp();
   DOUT1(("Insert 1000 items need  %7.6f", dabc::TimeDistance(tm1, tm2)));

   uint64_t res=0;
   tm1 = TimeStamp();
   for (unsigned n=0;n<1000;n++) {
      res = q.front();
      q.pop();
   }
   tm2 = TimeStamp();
   DOUT1(("Extract 1000 items need %7.6f", dabc::TimeDistance(tm1, tm2)));

   tm1 = TimeStamp();
   for (unsigned n=0;n<1000;n++) {
      q.push(n);
      res = q.front();
      q.pop();
   }
   tm2 = TimeStamp();

   DOUT1(("Insert and Extract 1000 items need %7.6f", dabc::TimeDistance(tm1, tm2)));

   uint64_t arr[1000];

   int cnt = 0;

   tm1 = TimeStamp();
   for (unsigned n=0;n<1000;n++) {
      arr[cnt++] = n;
   }
   tm2 = TimeStamp();
   DOUT1(("Insert 1000 items in array need %7.6f", dabc::TimeDistance(tm1, tm2)));

   cnt = 0;
   tm1 = TimeStamp();
   for (unsigned n=0;n<1000;n++) {
      res = arr[cnt++];
   }
   tm2 = TimeStamp();
   DOUT1(("Extract 1000 items from array need %7.6f", dabc::TimeDistance(tm1, tm2)));
}


int main(int numc, char* args[])
{
   int nodeid = 0;
   int numnodes = 4;
   int devices = 11; // only sockets
   const char* controllerID = "file.txt";
   if (numc>1) nodeid = atoi(args[1]);
   if (numc>2) numnodes = atoi(args[2]);
   if (numc>3) devices = atoi(args[3]);
   if (numc>4) controllerID = args[4];

//   if (numc>5) dabc::SocketDevice::SetLocalHostIP(args[5]);

   dabc::SetDebugLevel(1);

   dabc::StandaloneManager manager(0, nodeid, numnodes, false);

//   TestWorker(manager);
//   return 0;


   manager.ConnectCmdChannel(numnodes, devices / 10, controllerID);

   new Test1Plugin();

   DOUT1(("READY"));

   if (nodeid==0) {
       dabc::SetDebugLevel(1);

//       CheckIntQueue();

       sleep(1);

       int numworkers = 0;

       CreateAllModules(manager, numworkers);
       ConnectModules(manager, devices % 10);
       sleep(1);

       StartAll(manager);
       sleep(1);

//       dabc::SetDebugLevel(5);

       EnableSending(manager, true);
//       CheckLocking();

       dabc::ShowLongSleep("Main loop", 5);

       EnableSending(manager, false);

//       dabc::SetDebugLevel(1);

       sleep(1);
       StopAll(manager);

//       manager.SetPriorities(60);
//       sleep(1);

//       manager.ChangeSleepTime(50000, 2);
//       sleep(1);

//       EnableSending(manager);

//       for (int tm=10;tm<=500;tm+=10) {
//          manager.ChangeSleepTime(tm);
//          sleep(5);
//       }

//       manager.ChangeSleepTime(0);
//       sleep(1);


//       sleep(1); // need for master node to be able smoothly stop threads

//       DeleteAll(manager);

       CleanupAll(manager);
       sleep(2);


       DOUT1(("\n\n\n =============== START AGAIN ============"));

       // after cleanup try same again

       CreateAllModules(manager, numworkers);
       ConnectModules(manager, devices % 10);
       sleep(1);

       StartAll(manager);
       sleep(1);

       EnableSending(manager, true);
//       CheckLocking();

       dabc::ShowLongSleep("Main loop", 5);

       EnableSending(manager, false);
       sleep(1);
       StopAll(manager);
       CleanupAll(manager);
   }

   return 0;
}

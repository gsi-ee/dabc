#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/threads.h"

#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/Command.h"
#include "dabc/Iterator.h"
#include "dabc/StandaloneManager.h"
#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/Timer.h"
#include "dabc/TimeSyncModule.h"
#include "dabc/Application.h"
#include "dabc/statistic.h"


#include <map>
#include <math.h>
#include <queue>
#include <malloc.h>
#include <stdio.h>

const int TestBufferSize = 1024*64;
const int TestSendQueueSize = 5;
const int TestRecvQueueSize = 10;
const bool TestUseAckn = false;

class Test2SendModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool;
      bool                fCanSend;
      dabc::Ratemeter     fSendRate;
      int                 fPortCnt;

   public:
      Test2SendModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         int nports = cmd->GetInt("NumPorts", 3);
         int buffsize = cmd->GetInt("BufferSize", 16*1024);

         fPool = CreatePool("SendPool", buffsize, 500);

         for (int n=0;n<nports;n++)
            CreateOutput(FORMAT(("Output%d", n)), fPool, TestSendQueueSize);

         // CreateTimer("StillAlive", 1.);

         fCanSend = false;
         fPortCnt = 0;

         DOUT3(( "new TSendModule %s nports = %d buf = %d done", GetName(), NumOutputs(), buffsize));
      }

      int ExecuteCommand(dabc::Command* cmd)
      {
         if (cmd->IsName("EnableSending")) {
            fCanSend = cmd->GetBool("Enable", true);

            if (fCanSend) {
               DOUT1(("Can start sending"));
               for(int n=0;n<TestSendQueueSize;n++)
                  for(unsigned nport=0;nport<NumOutputs();nport++) {
                     dabc::Buffer* ref = fPool->TakeBuffer(0, false);
//                     DOUT1(("Buf %p size = %d", ref, ref->GetTotalSize()));
                     Output(nport)->Send(ref);
                  }
            }
            fPortCnt = 0;
         } else
            return ModuleAsync::ExecuteCommand(cmd);

         return cmd_true;
      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         if (!fCanSend || (NumOutputs()==0)) return;

         bool fullchaotic = false;

         if (!fullchaotic && (Output(fPortCnt)!=port)) return;

         bool tryagain = true;

         do {
            tryagain = false;
            dabc::Buffer* ref = fPool->TakeBuffer(0, false);
            fSendRate.Packet(ref->GetDataSize());
            port->Send(ref);

            if (!fullchaotic) {
               fPortCnt = (fPortCnt+1) % NumOutputs();
               port = Output(fPortCnt);
               tryagain = !port->OutputBlocked();
            }

         } while (tryagain);
      }

      virtual void ProcessTimerEvent(dabc::Timer* timer)
      {
         DOUT1(("Get timer event %s", timer->GetName()));
      }


      void AfterModuleStop()
      {
         DOUT1(("TSendModule finish Rate %5.1f numoper:%7ld", fSendRate.GetRate(), fSendRate.GetNumOper()));
      }
};


class Test2RecvModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool;
      int fSleepTime;
      dabc::Ratemeter fRecvRate;
      dabc::Average fRealSleepTime;

   public:
      Test2RecvModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleAsync(name, cmd)
      {
         // we will use queue (second true) in the signal to detect order of signal fire
         int nports = cmd->GetInt("NumPorts", 3);
         int buffsize = cmd->GetInt("BufferSize", 16*1024);

         DOUT3(( "new TRecvModule %s %d %d", GetName(), nports, buffsize));

         fPool = CreatePool("RecvPool", buffsize, 5);

         for (int n=0;n<nports;n++)
            CreateInput(FORMAT(("Input%d", n)), fPool, TestRecvQueueSize);

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

         if (ref==0) {
            EOUT(("Cannot recv buffer from port %s", port->GetName()));
            return;
         }

//         DOUT1(("Recieve packet %p %d ", ref, ref->GetTotalSize()));

         fRecvRate.Packet(ref->GetTotalSize());
         dabc::Buffer::Release(ref);

         if (fSleepTime>0) {
            dabc::TimeStamp_t tm1 = TimeStamp();
            dabc::MicroSleep(fSleepTime);
            dabc::TimeStamp_t tm2 = TimeStamp();
            fRealSleepTime.Fill(dabc::TimeDistance(tm1, tm2));
         }
      }

      void AfterModuleStop()
      {
         DOUT1(("TRecvModule finish Rate %5.1f numoper:%7ld", fRecvRate.GetRate(), fRecvRate.GetNumOper()));
      }
};

class Test2WorkerModule : public dabc::ModuleSync {
   protected:

      int fCounter;

   public:
      Test2WorkerModule(const char* name, dabc::Command* cmd) :
         dabc::ModuleSync(name, cmd)
      {
         fCounter = 0;
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

         while (TestWorking()) {
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


class Test2Plugin: public dabc::Application  {
   public:
      Test2Plugin() : dabc::Application("Test2Plugin") { }

      dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
      {
          if ((classname==0) || (cmd==0)) return 0;

          if (strcmp(classname,"Test2SendModule")==0)
             return new Test2SendModule(modulename, cmd);
          else
          if (strcmp(classname,"Test2RecvModule")==0)
             return new Test2RecvModule(modulename, cmd);
          else
          if (strcmp(classname,"Test2WorkerModule")==0)
             return new Test2WorkerModule(modulename, cmd);

          return 0;
      }
};

void CreateAllModules(dabc::StandaloneManager &m, int buffersize, int numworkers = 0)
{
   dabc::CommandClient cli;

   for (int node=0;node<m.NumNodes();node++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("Test2RecvModule","Receiver");
      cmd->SetInt("NumPorts", m.NumNodes()-1);
      cmd->SetInt("BufferSize", buffersize);
      m.SubmitRemote(cli, cmd, node);
   }

   for (int node=0;node<m.NumNodes();node++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("Test2SendModule","Sender");
      cmd->SetInt("NumPorts", m.NumNodes()-1);
      cmd->SetInt("BufferSize", buffersize);
      m.SubmitRemote(cli, cmd, node);
   }

   for (int nw=0;nw<numworkers;nw++)
      for (int node=0;node<m.NumNodes();node++) {
         dabc::Command* cmd =
            new dabc::CmdCreateModule("Test2WorkerModule", FORMAT(("Worker%d",nw)));
         m.SubmitRemote(cli, cmd, node);
      }

   bool res = cli.WaitCommands(5);

   DOUT1(("CreateAllModules() res = %s", DBOOL(res)));
}

void ConnectModules(dabc::StandaloneManager &m, int deviceid = 1)
{
   dabc::CommandClient cli;

   const char* devname = "Test2Dev";
   const char* deviceclass = dabc::typeSocketDevice;
   if (deviceid==2) deviceclass = "verbs::Device";

   for (int node = 0; node < m.NumNodes(); node++)
      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);

   if (!cli.WaitCommands(5)) {
      EOUT(("Cannot create devices of class %s", deviceclass));
      exit(1);
   }

   for (int nsender = 0; nsender < m.NumNodes(); nsender++) {
      for (int nreceiver = 0; nreceiver < m.NumNodes(); nreceiver++) {
          if (nsender==nreceiver) continue;

          std::string port1name, port2name;

          dabc::formats(port1name, "%s$Sender/Output%d", m.GetNodeName(nsender), nreceiver>nsender ? nreceiver-1 : nreceiver);
          dabc::formats(port2name, "%s$Receiver/Input%d", m.GetNodeName(nreceiver), nsender>nreceiver ? nsender-1 : nsender);

          dabc::Command* cmd =
             new dabc::CmdConnectPorts(port1name.c_str(),
                                          port2name.c_str(),
                                          devname);

          cmd->SetBool(dabc::xmlUseAcknowledge, TestUseAckn);

          m.SubmitCl(cli, cmd);
      }
   }
   bool res = cli.WaitCommands(5);
   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));
}

void SetPriorities(dabc::StandaloneManager &m, int prio = 0)
{
   dabc::CommandClient cli;

   for (int node=0;node<m.NumNodes();node++) {
      dabc::Command* cmd = new dabc::Command("SetPriority");
      cmd->SetInt("Priority",prio);
      m.SubmitRemote(cli, cmd, node, "Receiver");
   }
   for (int node=0;node<m.NumNodes();node++) {
      dabc::Command* cmd = new dabc::Command("SetPriority");
      cmd->SetInt("Priority",prio);
      m.SubmitRemote(cli, cmd, node, "Sender");
   }

   bool res = cli.WaitCommands(1);
   DOUT1(("SetPriorities res = %s", DBOOL(res)));
}

void StartAll(dabc::StandaloneManager &m, int numworkers = 0)
{
   dabc::CommandClient cli;

   for (int node=0;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdStartAllModules(), node);

   bool res = cli.WaitCommands(1);
   DOUT1(("StartAll() res = %s", DBOOL(res)));
}

void EnableSending(dabc::StandaloneManager &m, bool on = true, int maxnode = -1)
{
   dabc::CommandClient cli;

   for (int node=0;node< (maxnode>=0 ? maxnode : m.NumNodes()); node++) {
      dabc::Command* cmd = new dabc::Command("EnableSending");
      cmd->SetBool("Enable", on);
      m.SubmitRemote(cli, cmd, node, "Sender");
   }
   cli.WaitCommands(1);
}

void ChangeSleepTime(dabc::StandaloneManager &m, int tm=0, int selectnode = -1)
{
   dabc::CommandClient cli;

   for (int node=0;node<m.NumNodes();node++) {
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

   for (int node=0;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdStopAllModules(), node);
   bool res = cli.WaitCommands(5);
   DOUT1(("StopAll res = %s", DBOOL(res)));
}

void CleanupAll(dabc::StandaloneManager &m)
{
   dabc::CommandClient cli;

   for (int node=0;node<m.NumNodes();node++)
      m.SubmitRemote(cli, new dabc::CmdCleanupManager(), node);
   bool res = cli.WaitCommands(5);
   DOUT1(("CleanupAll res = %s", DBOOL(res)));
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
   DOUT1(("Insert 1000 items need %7.6f", dabc::TimeDistance(tm1, tm2)));

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

void RunStandaloneTest(dabc::StandaloneManager &m, int nmodules = 1)
{
   DOUT1(("STANDALONE TEST N = %d", nmodules));

   dabc::CommandClient cli;

   const char* thrdname = 0; // "CommonThread";

   for (int nm=0;nm<nmodules;nm++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("TRecvModule", FORMAT(("Receiver%d",nm)), thrdname);
      cmd->SetInt("NumPorts", nmodules);
      cmd->SetInt("BufferSize", TestBufferSize);
      m.SubmitCl(cli, cmd);
   }

   for (int nm=0;nm<nmodules;nm++) {
      dabc::Command* cmd =
         new dabc::CmdCreateModule("TSendModule",FORMAT(("Sender%d",nm)), thrdname);
      cmd->SetInt("NumPorts", nmodules);
      cmd->SetInt("BufferSize", TestBufferSize);
      m.SubmitCl(cli, cmd);
   }

   for (int nm1=0;nm1<nmodules;nm1++)
      for (int nm2=0;nm2<nmodules;nm2++)
         m.SubmitCl(cli,
            new dabc::CmdConnectPorts(
                  FORMAT(("Sender%d/Output%d", nm1, nm2)),
                  FORMAT(("Receiver%d/Input%d", nm2, nm1))));
   cli.WaitCommands(1);

   m.SubmitCl(cli, new dabc::CmdStartAllModules());
   cli.WaitCommands(1);

   for (int nm=0;nm<nmodules;nm++) {
      dabc::Command* cmd = new dabc::Command("EnableSending");
      cmd->SetBool("Enable", true);
      m.SubmitLocal(cli, cmd, FORMAT(("Sender%d",nm)));
   }
   cli.WaitCommands(1);

   sleep(3);

   for (int nm=0;nm<nmodules;nm++) {
      dabc::Command* cmd = new dabc::Command("EnableSending");
      cmd->SetBool("Enable", false);
      m.SubmitLocal(cli, cmd, FORMAT(("Sender%d",nm)));
   }
   cli.WaitCommands(1);

   m.SubmitCl(cli, new dabc::CmdStopAllModules());
   cli.WaitCommands(1);

   m.SubmitCl(cli, new dabc::CmdCleanupManager());
   cli.WaitCommands(5);
}

void AllToAllLoop(dabc::StandaloneManager &manager, int device, int buffersize, int numworkers)
{
   long arena1 = dabc::GetProcVirtMem();
   DOUT1(("AllToAllLoop Start with: %ld",arena1));

   dabc::CpuStatistic cpu;

   CreateAllModules(manager, buffersize, numworkers);

   ConnectModules(manager, device);

   sleep(1);

   // dabc::Iterator::PrintHieararchy(&manager);
   StartAll(manager);
   sleep(1);
   EnableSending(manager, true);
   cpu.Reset();
   sleep(10);
   cpu.Measure();
   EnableSending(manager, false);
   sleep(1);
   DOUT1(("AllToAllLoop Works with: %ld  CPU: %5.1f", dabc::GetProcVirtMem(), cpu.CPUutil()*100.));
   StopAll(manager);

   CleanupAll(manager);
   sleep(2);

   long arena2 = dabc::GetProcVirtMem();
   DOUT1(("CleanupAll  DIFF = %ld", arena2-arena1));

}

void OneToAllLoop(dabc::StandaloneManager &m, int deviceid)
{
   dabc::CommandClient cli;

   long arena1 = dabc::GetProcVirtMem();
   DOUT1(("OneToAllLoop Start with: %ld",arena1));

   dabc::Command* cmd = 0;

   for (int node=1;node<m.NumNodes();node++) {
      cmd = new dabc::CmdCreateModule("Test2RecvModule","Receiver");
      cmd->SetInt("NumPorts", 1);
      cmd->SetInt("BufferSize", TestBufferSize);
      m.SubmitRemote(cli, cmd, node);
   }

   cmd = new dabc::CmdCreateModule("Test2SendModule","Sender");
   cmd->SetInt("NumPorts", m.NumNodes()-1);
   cmd->SetInt("BufferSize", TestBufferSize);
   m.SubmitLocal(cli, cmd);

   bool res = cli.WaitCommands(5);

   DOUT1(("CreateAllModules() res = %s", DBOOL(res)));

   const char* devname = "Test2Dev";
   const char* deviceclass = dabc::typeSocketDevice;
   if (deviceid==2) deviceclass = "verbs::Device";

   for (int node = 0; node < m.NumNodes(); node++)
      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);

   if (!cli.WaitCommands(5)) {
      EOUT(("Cannot create devices of class %s", deviceclass));
      exit(1);
   }

   for (int nreceiver = 1; nreceiver < m.NumNodes(); nreceiver++) {
      std::string port1name, port2name;

      dabc::formats(port1name, "%s$Sender/Output%d", m.GetNodeName(0), nreceiver-1);
      dabc::formats(port2name, "%s$Receiver/Input0", m.GetNodeName(nreceiver));

      cmd = new dabc::CmdConnectPorts(port1name.c_str(),
                                         port2name.c_str(),
                                         devname);

      m.SubmitCl(cli, cmd);
   }
   res = cli.WaitCommands(5);
   DOUT1(("ConnectAllModules() res = %s", DBOOL(res)));
   sleep(1);

   StartAll(m);
   sleep(1);
   EnableSending(m, true, 1);
   sleep(5);
   EnableSending(m, false, 1);
   sleep(1);
   StopAll(m);

   CleanupAll(m);
   sleep(2);

   long arena2 = dabc::GetProcVirtMem();
   DOUT1(("CleanupAll  DIFF = %ld", arena2-arena1));
}

void TimeSyncLoop(dabc::StandaloneManager &m, int deviceid)
{
   dabc::CommandClient cli;

   const char* devname = "TSyncDev";
   const char* deviceclass = dabc::typeSocketDevice;
   const char* thrdclass = "SocketThread";
   if (deviceid==2) {
       deviceclass = "verbs::Device";
       thrdclass = "VerbsThread";
   }

   for (int node = 0; node < m.NumNodes(); node++) {
      m.SubmitRemote(cli, new dabc::CmdCreateDevice(deviceclass, devname), node);
      m.SubmitRemote(cli, new dabc::CmdCreateThread("TSyncThrd", thrdclass, devname), node);
   }

   if (!cli.WaitCommands(5)) {
      EOUT(("Cannot create devices of class %s", deviceclass));
      exit(1);
   }

   dabc::Command* cmd = 0;

   for (int node=0;node<m.NumNodes();node++) {
      cmd = new dabc::CmdCreateModule("TimeSyncModule","TSync", "TSyncThrd");

      if (node==0)
         cmd->SetInt("NumSlaves", m.NumNodes()-1);
      else
         cmd->SetBool("MasterConn", true);
      m.SubmitRemote(cli, cmd, node);
   }

   bool res = cli.WaitCommands(5);

   DOUT1(("Create TimeSync modules() res = %s", DBOOL(res)));

   for (int nslave = 1; nslave < m.NumNodes(); nslave++) {
      std::string port1name, port2name;

      dabc::formats(port1name, "%s$TSync/Slave%d", m.GetNodeName(0), nslave-1);
      dabc::formats(port2name, "%s$TSync/Master", m.GetNodeName(nslave));

      cmd = new dabc::CmdConnectPorts(port1name.c_str(),
                                         port2name.c_str(),
                                         devname, "TSyncThrd");

      m.SubmitCl(cli, cmd);
   }
   res = cli.WaitCommands(5);
   DOUT1(("Connect TimeSync Modules() res = %s", DBOOL(res)));

   StartAll(m);
   sleep(1);


   cmd = new dabc::CommandDoTimeSync(false, 100, true, false);
   m.SubmitLocal(cli, cmd, "TSync");
   res = cli.WaitCommands(5);

   dabc::ShowLongSleep("First pause", 5);

   cmd = new dabc::CommandDoTimeSync(false, 100, true, true);
   m.SubmitLocal(cli, cmd, "TSync");
   res = cli.WaitCommands(5);

   dabc::ShowLongSleep("Second pause", 10);

   cmd = new dabc::CommandDoTimeSync(false, 100, false, false);
   m.SubmitLocal(cli, cmd, "TSync");
   res = cli.WaitCommands(5);

   sleep(1);
   StopAll(m);

   CleanupAll(m);
   sleep(2);
}

int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);


   int nodeid = 0;
   int numnodes = 4;
   int devices = 11; // only sockets
   const char* controllerID = "file.txt";
   if (numc>1) nodeid = atoi(args[1]);
   if (numc>2) numnodes = atoi(args[2]);
   if (numc>3) devices = atoi(args[3]);
   if (numc>4) controllerID = args[4];

   dabc::SetDebugLevel(1);

   dabc::StandaloneManager manager(0, nodeid, numnodes, false);
   new Test2Plugin();

   if (numc<2) {
      RunStandaloneTest(manager, 5);

      return 0;
   }

   manager.ConnectCmdChannel(numnodes, devices / 10, controllerID);

   DOUT1(("READY"));

   if (nodeid==0) {
//       dabc::SetDebugLevel(3);

       sleep(1);

       int bufsize = 4*1024;

       for (int cnt=0;cnt<4;cnt++) {

//          CheckNewDelete();

          DOUT1(("\n\n\n =============== START AGAIN %d ============", cnt));

          DOUT1(("BufSize %d Basic %ld Transports %ld",
             bufsize,
             dabc::Basic::NumInstances(),
             dabc::Transport::NumTransports()));

//          AllToAllLoop(manager, devices % 10, bufsize, 0);

//          OneToAllLoop(manager, devices % 10);

          TimeSyncLoop(manager, devices % 10);
          bufsize *= 2;
       }

       // after cleanup try same again
//       AllToAllLoop(manager, devices % 10, 0);

   }

   return 0;
}

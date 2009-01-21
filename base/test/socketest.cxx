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
#include "dabc/SocketThread.h"
#include "dabc/SocketDevice.h"


#define BUFFERSIZE 1024*16
#define QUEUESIZE 10

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
         fPool = CreatePoolHandle("Pool", BUFFERSIZE, 5);

         if (fKind>0)
            fInput = CreateInput("Input", fPool, QUEUESIZE, 0);

         if (fKind<2)
            fOutput = CreateOutput("Output", fPool, QUEUESIZE, 0);

         fCounter = 0;
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         if ((fKind==1) && !fOutput->CanSend()) return;

         dabc::Buffer* buf = fInput->Recv();

         if (fKind==2) {
//            DOUT1(("Get: %s", buf->GetDataLocation()));

            dabc::Buffer::Release(buf);
            fCounter++;
         } else
            fOutput->Send(buf);

      }

      void ProcessOutputEvent(dabc::Port* port)
      {
         if (fKind==0) GeneratePacket(); else
         if ((fKind==1) && fInput->CanRecv())
            fOutput->Send(fInput->Recv());
      }

      void ProcessDisconnectEvent(dabc::Port* port)
      {
         DOUT1(("Port %s is disconnected, stop module", port->GetName()));
         Stop();
      }


      virtual void BeforeModuleStart()
      {
         DOUT3(("TestModuleAsync %s starts", GetName()));
         if (fKind==0)
           for(int n=0; n<QUEUESIZE; n++)
              GeneratePacket();
      }

      void GeneratePacket()
      {
         dabc::Buffer* buf = fPool->TakeBuffer(BUFFERSIZE);

         sprintf((char*)buf->GetDataLocation(), "Buffer %ld", fGlobalCnt);
         fGlobalCnt++;

         fOutput->Send(buf);
      }

      virtual void AfterModuleStop()
      {
         DOUT3(("TestModuleAsync %s stops %ld", GetName(), fCounter));
         if (fCounter>0) fGlobalCnt = fCounter;
      }
};


void TestNewSocketDevice(int numc, char* args[])
{
   dabc::Manager mgr("local");

   mgr.CreateDevice(dabc::typeSocketDevice, "SOCKET");

   dabc::Device* dev = mgr.FindDevice("SOCKET");
   if (dev==0) {
      EOUT(("Socket device not created"));
      exit(1);
   }

   dabc::Module* m = 0;
   dabc::Command* cmd = 0;

   if (numc==1) {
      // this is server
      m = new TestModuleAsync("Sender", 0);

      cmd = new dabc::CmdDirectConnect(true, "Sender/Output");

   } else {
      // this is client

      const char* serverid = args[1];
      DOUT1(("Connect to server %s", serverid));

      dabc::Command* dcmd = new dabc::Command("Print");
      dabc::CommandClient cli;
      cli.Assign(dcmd);
      dev->SubmitRemoteCommand(serverid, "channel", dcmd);
      cli.WaitCommands(5);

      m = new TestModuleAsync("Receiver", 2);

      cmd = new dabc::CmdDirectConnect(false, "Receiver/Input");

      cmd->SetPar("ServerId", serverid);
      cmd->SetBool("ServerUseAckn", true);
      cmd->SetUInt("ServerHeaderSize", 0);

   }

   mgr.MakeThreadForModule(m, "Thread1");

   cmd->SetInt("Timeout", 3);
   cmd->SetStr("ConnId", "TestConn");

   if (!dev->Execute(cmd, 6)) {
      EOUT(("Cannot establish connection"));
      exit(1);
   }

   dabc::CpuStatistic cpu;

   fGlobalCnt = 0;

   mgr.StartAllModules();
   dabc::TimeStamp_t tm1 = TimeStamp();

   cpu.Reset();

   dabc::ShowLongSleep("Main loop", 5);

   cpu.Measure();

   mgr.StopAllModules();

   dabc::TimeStamp_t tm2 = TimeStamp();

   if (fGlobalCnt<=0) fGlobalCnt = 1;

   DOUT1(("Time = %5.3f Cnt = %ld Per buffer = %5.3f ms CPU = %5.1f",
      dabc::TimeDistance(tm1,tm2), fGlobalCnt, dabc::TimeDistance(tm1,tm2)/fGlobalCnt*1e3, cpu.CPUutil()*100.));

   mgr.CleanupManager();

   DOUT1(("Did manager cleanup"));

   dabc::SetDebugLevel(1);
}



class TestConnectionClient : public dabc::SocketIOProcessor {
   public:
      TestConnectionClient(int fd, bool server) :
        dabc::SocketIOProcessor(fd),
        fServer(server)
        {
        }

      void StartJob()
      {
         if (fServer) {
            StartRecv(fInBuf, 100);
         } else {
            // we can start both send and recv operations simultaniousely,
            // while buffer will be received only when server answer on request
            strcpy(fOutBuf,"Request connection");
            StartSend(fOutBuf, 100);
            StartRecv(fInBuf, 100);
         }
      }

      virtual void OnSendCompleted()
      {
         if (fServer) {
            DOUT1(("Server job finished"));
            DestroyProcessor();
         } else {
      //      StartRecv(fInBuf, 100);
         }
      }

      virtual void OnRecvCompleted()
      {
         if (fServer) {
            if (strcmp(fInBuf, "Request connection")==0)
               strcpy(fOutBuf,"Connection accepted");
            else
               strcpy(fOutBuf,"Connection rejected");
            DOUT1(("Get request: %s Send back: %s",fInBuf, fOutBuf));

            StartSend(fOutBuf, 100);
         } else {
            DOUT1(("Server answer:%s", fInBuf));

            if (strcmp(fInBuf, "Connection accepted")==0) DOUT1(("Goood"));

            DOUT1(("Client job finished"));

            DestroyProcessor();
         }
      }

   protected:

      bool fServer;

      char fInBuf[1024];
      char fOutBuf[1024];
};



class TestServerPort : public dabc::SocketServerProcessor {
   public:
      TestServerPort(int serversocket, int portnum) :
         dabc::SocketServerProcessor(serversocket, portnum) {}

   protected:
      virtual void OnClientConnected(int connfd)
      {
         TestConnectionClient* cli = new TestConnectionClient(connfd, true);
         cli->AssignProcessorToThread(ProcessorThread());
         cli->StartJob();
         // DestroyProcessor(); we doit from main program
      }
};

class TestClientPort : public dabc::SocketClientProcessor {
   public:
      TestClientPort(const struct addrinfo* serv_addr, int fd = -1) :
         dabc::SocketClientProcessor(serv_addr, fd)
      {
      }
   protected:

      virtual void OnConnectionEstablished(int connfd)
      {
         DOUT1(("Server connected"));

         TestConnectionClient* cli = new TestConnectionClient(connfd, false);
         cli->AssignProcessorToThread(ProcessorThread());
         cli->StartJob();
//         DestroyProcessor();
      }

      virtual void OnConnectionFailed()
      {
         DOUT1(("Client connection request failed"));
//         DestroyProcessor();
      }
};

void TestServerClientThread(int numc, char* args[])
{
   dabc::Manager mgr("local");

   dabc::SocketThread* thrd = new dabc::SocketThread(mgr.GetThreadsFolder(true));

   thrd->Start(10);

   if (numc==1) {
      DOUT1(("Start as server"));

      int nport(-1);

      int fd = dabc::SocketThread::StartServer(nport, 7000, 9000);

      if (fd<0) return;

      DOUT1(("Start server on port %d, fd:%d", nport, fd));

      TestServerPort* serv = new TestServerPort(fd, nport);
      serv->AssignProcessorToThread(thrd);

      dabc::LongSleep(7);

      serv->DestroyProcessor();

   } else {
      DOUT1(("Start as client"));

      const char* host = args[1];
      const char* service = args[2];

      struct addrinfo *info;
      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      TestClientPort* cli = 0;

      if (getaddrinfo(host, service, &hints, &info)==0) {
         for (struct addrinfo *t = info; t; t = t->ai_next) {

            int sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

            if (sockfd<=0) continue;

            cli = new TestClientPort(t, sockfd);
            cli->SetRetryOpt(5, 0.5);
            cli->AssignProcessorToThread(thrd);
            break;
         }
         freeaddrinfo(info);
      }

      dabc::LongSleep(4);

      cli->DestroyProcessor();

/*      const char* host = args[1];
      int nport = atoi(args[2]);

      int fd = dabc::SocketThread::StartClient(host, nport);

      if (fd<0) {
         EOUT(("Cannot start client"));
         return;
      }

      DOUT1(("Connect to server via socket %d", fd));

      TestConnectionClient* cli = new TestConnectionClient(fd, false);
      cli->AssignProcessorToThread(thrd);
      cli->StartJob();

      dabc::LongSleep(4);

*/
   }

   thrd->Stop();

   delete thrd;

}

#include <netdb.h>

void TestGetHost(const char* name)
{
   struct hostent *host = ::gethostbyname(name);
   if (host==0) return;

   DOUT1(("Name %s Length = %d", name, host->h_length));

   int cnt = 0;
   while (host->h_addr_list[cnt]) {
      DOUT1(("Pointer %p %x", host->h_addr_list[cnt], *((int32_t*)host->h_addr_list[cnt])));
      cnt++;
   }

}



int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   TestGetHost(numc > 1 ? args[1] : "master");
   return 0;


//   TestServerClientThread(numc, args);

   TestNewSocketDevice(numc, args);
   return 0;
}


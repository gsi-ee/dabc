#include "dabc/logging.h"

#include <math.h>

#include "dabc/FileIO.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"
#include "dabc/statistic.h"

#include "mbs/EventAPI.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/ServerTransport.h"
#include "mbs/Iterator.h"

#define BUFFERSIZE 16*1024



double Gauss_Rnd(double mean, double sigma)
{

   double x, y, z;

//   srand(10);

   z = 1.* rand() / RAND_MAX;
   y = 1.* rand() / RAND_MAX;

   x = z * 6.28318530717958623;
   return mean + sigma*sin(x)*sqrt(-2*log(y));
}

bool GenerateMbsPacketForGo4(dabc::Buffer* buf, int &evid)
{
   if (buf==0) return false;

   mbs::WriteIterator iter(buf);

   while (iter.NewEvent(evid)) {

      bool eventdone = true;

      int cnt = 0;
      while (++cnt < 3)
         if (iter.NewSubevent(8 * sizeof(uint32_t), cnt)) {
            uint32_t* value = (uint32_t*) iter.rawdata();

            for (int nval=0;nval<8;nval++)
               *value++ = (uint32_t) Gauss_Rnd(nval*100 + 2000, 500./(nval+1));

            iter.FinishSubEvent(8 * sizeof(uint32_t));
         } else {
            eventdone = false;
            break;
         }

      if (!eventdone) break;

      if (!iter.FinishEvent()) break;

      evid++;
   }

   return true;
}



class GeneratorModule : public dabc::ModuleSync {
   protected:
      dabc::PoolHandle*       fPool;
      int                     fEventCnt;
      int                     fBufferCnt;

   public:
      GeneratorModule(const char* name) :
         dabc::ModuleSync(name),
         fPool(0),
         fEventCnt(1),
         fBufferCnt(0)

      {
         SetSyncCommands(true);

         fPool = CreatePool("Pool", 10, BUFFERSIZE);

         CreateOutput("Output", fPool, 5);
      }


      virtual void AfterModuleStop()
      {
         DOUT1(("GeneratorModule %s stoped, generated %d bufs %d evnts", GetName(), fBufferCnt, fEventCnt));
      }

      virtual void MainLoop()
      {
         while (TestWorking()) {

            dabc::Buffer* buf = TakeBuffer(fPool, BUFFERSIZE);

            GenerateMbsPacketForGo4(buf, fEventCnt);

            Send(Output(0), buf);

            DOUT4(("Send packet %d with event %d", fBufferCnt, fEventCnt));

            fBufferCnt++;
         }
      }
};

class TestModuleAsync : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool;
      dabc::Port*         fInput;
      long                fCounter;
      dabc::Ratemeter     fRate;

   public:
      TestModuleAsync(const char* name) :
         dabc::ModuleAsync(name),
         fPool(0),
         fInput(0)
      {
         fPool = CreatePool("Pool1", 5, BUFFERSIZE);

         fInput = CreateInput("Input", fPool, 5);

         fCounter = 0;

         fRate.Reset();
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer* buf = 0;
         fInput->Recv(buf);

         fRate.Packet(buf->GetTotalSize());

         mbs::ReadIterator iter(buf);

         int cnt = 0;

         while (iter.NextEvent()) cnt++;

         DOUT1(("Found %d events in MBS buffer", cnt));

         dabc::Buffer::Release(buf);
         fCounter++;
      }

      virtual void AfterModuleStop()
      {
         DOUT1(("TestModuleAsync %s stops %ld Rate: %5.1f MB/s", GetName(), fCounter, fRate.GetRate()));
      }
};


extern "C" void StartGenerator()
{
    if (dabc::mgr()==0) {
       EOUT(("Manager is not created"));
       exit(1);
    }

    DOUT0(("Start MBS generator module"));

    dabc::Module* m = new GeneratorModule("Generator");
    dabc::mgr()->MakeThreadForModule(m);
    dabc::mgr()->CreateMemoryPools();

    dabc::Command* cmd = new dabc::CmdCreateTransport("Modules/Generator/Ports/Output", mbs::typeServerTransport, "MbsTransport");
    cmd->SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer));
    cmd->SetInt(dabc::xmlBufferSize, BUFFERSIZE);
    if (!dabc::mgr()->Execute(cmd)) {
       EOUT(("Cannot create MBS transport server"));
       exit(1);
    }

//      if (!mgr.Execute(mgr.LocalCmd(cmd, "Devices/MBS"))) {
//         EOUT(("Cannot create MBS transport server"));
//         return 0;
//      }

    m->Start();
}

extern "C" void StartClient()
{
   const char* hostname = "lxg0526";
   int nport = 6000;

   if (dabc::mgr()==0) {
      EOUT(("Manager is not created"));
      exit(1);
   }

   DOUT1(("Start as client for node %s:%d", hostname, nport));

   dabc::Module* m = new TestModuleAsync("Receiver");

   dabc::mgr()->MakeThreadForModule(m);

   dabc::mgr()->CreateMemoryPools();

   dabc::Command* cmd = new dabc::CmdCreateTransport("Modules/Receiver/Ports/Input", mbs::typeClientTransport, "MbsTransport");
//   cmd->SetBool("IsClient", true);
   cmd->SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer));
   cmd->SetStr(mbs::xmlServerName, hostname);
   if (nport>0) cmd->SetInt(mbs::xmlServerPort, nport);
   cmd->SetInt(dabc::xmlBufferSize, BUFFERSIZE);

   if (!dabc::mgr()->Execute(cmd)) {
      EOUT(("Cannot create data input for receiver"));
      exit(1);
   }

   m->Start();

//   dabc::LongSleep(3);
}

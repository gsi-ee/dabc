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
      int                     fEventCnt;
      int                     fBufferCnt;

   public:
      GeneratorModule(const char* name) :
         dabc::ModuleSync(name),
         fEventCnt(1),
         fBufferCnt(0)

      {
         SetSyncCommands(true);

         CreatePoolHandle("Pool");

         CreateOutput("Output", Pool(), 5);
      }


      virtual void AfterModuleStop()
      {
         DOUT1(("GeneratorModule %s stoped, generated %d bufs %d evnts", GetName(), fBufferCnt, fEventCnt));
      }

      virtual void MainLoop()
      {
         while (ModuleWorking()) {

            dabc::Buffer buf = TakeBuffer(Pool());

            GenerateMbsPacketForGo4(buf, fEventCnt);

            Send(Output(0), buf);

            DOUT4(("Send packet %d with event %d", fBufferCnt, fEventCnt));

            fBufferCnt++;
         }
      }
};

class TestModuleAsync : public dabc::ModuleAsync {
   protected:
      dabc::Port*         fInput;
      long                fCounter;
      dabc::Ratemeter     fRate;

   public:
      TestModuleAsync(const char* name) :
         dabc::ModuleAsync(name),
         fInput(0)
      {
         fPool = CreatePoolHandle("Pool1");

         fInput = CreateInput("Input", Pool(), 5);

         fCounter = 0;

         fRate.Reset();
      }

      void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer buf = fInput->Recv();

         fRate.Packet(buf.GetTotalSize());

         mbs::ReadIterator iter(buf);

         int cnt = 0;

         while (iter.NextEvent()) cnt++;

         DOUT1(("Found %d events in MBS buffer", cnt));

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
       exit(134);
    }

    DOUT0(("Start MBS generator module"));

    dabc::Module* m = new GeneratorModule("Generator");
    dabc::mgr()->MakeThreadFor(m);

    dabc::CmdCreateTransport cmd("Generator/Output", mbs::typeServerTransport, "MbsTransport");
    cmd.SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer));
    cmd.SetInt(dabc::xmlBufferSize, BUFFERSIZE);
    if (!dabc::mgr.Execute(cmd)) {
       EOUT(("Cannot create MBS transport server"));
       exit(135);
    }

    m->Start();
}

extern "C" void StartClient()
{
   const char* hostname = "lxg0526";
   int nport = 6000;

   if (dabc::mgr()==0) {
      EOUT(("Manager is not created"));
      exit(136);
   }

   DOUT1(("Start as client for node %s:%d", hostname, nport));

   dabc::Module* m = new TestModuleAsync("Receiver");

   dabc::mgr()->MakeThreadFor(m);

   dabc::CmdCreateTransport cmd("Receiver/Input", mbs::typeClientTransport, "MbsTransport");
//   cmd.SetBool("IsClient", true);
   cmd.SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer));
   cmd.SetStr(mbs::xmlServerName, hostname);
   if (nport>0) cmd.SetInt(mbs::xmlServerPort, nport);
   cmd.SetInt(dabc::xmlBufferSize, BUFFERSIZE);

   if (!dabc::mgr.Execute(cmd)) {
      EOUT(("Cannot create data input for receiver"));
      exit(137);
   }

   m->Start();

//   dabc::mgr()->Sleep(3);
}

#include "mbs/GeneratorModule.h"

#include <math.h>

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Manager.h"
#include "mbs/Iterator.h"

double Gauss_Rnd(double mean, double sigma)
{

   double x, y, z;

//   srand(10);

   z = 1.* rand() / RAND_MAX;
   y = 1.* rand() / RAND_MAX;

   x = z * 6.28318530717958623;
   return mean + sigma*sin(x)*sqrt(-2*log(y));
}


mbs::GeneratorModule::GeneratorModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fEventCount = 0;

   fStartStopPeriod = GetCfgInt("StartStopPeriod", 0, cmd);
   fNumSubevents = GetCfgInt("NumSubevents", 2, cmd);
   fFirstProcId = GetCfgInt("FirstProcId", 0, cmd);
   fSubeventSize = GetCfgInt("SubeventSize", 32, cmd);
   fIsGo4RandomFormat = GetCfgBool("Go4Random", true, cmd);
   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);

   DOUT1(("Generator buffer size %u sub %u", fBufferSize, fNumSubevents));

   fPool = CreatePoolHandle("Pool", fBufferSize, 10);

   CreateOutput("Output", fPool, 5);
}

void mbs::GeneratorModule::FillRandomBuffer(dabc::Buffer* buf)
{
   if (buf==0) { EOUT(("No free buffer - generator will block")); return; }
   buf->SetDataSize(fBufferSize);

   mbs::WriteIterator iter(buf);

   while (iter.NewEvent(fEventCount)) {

      bool eventdone = true;

      for (unsigned subcnt = 0; subcnt < fNumSubevents; subcnt++)
         if (iter.NewSubevent(fSubeventSize, 0, fFirstProcId + subcnt)) {

            unsigned subsz = fSubeventSize;

            uint32_t* value = (uint32_t*) iter.rawdata();

            if (fIsGo4RandomFormat) {
               unsigned numval = fSubeventSize / sizeof(uint32_t);
               for (unsigned nval=0;nval<numval;nval++)
                  *value++ = (uint32_t) Gauss_Rnd(nval*100 + 2000, 500./(nval+1));

               subsz = numval * sizeof(uint32_t);
            } else {
               if (subsz>0) *value++ = fEventCount;
               if (subsz>4) *value++ = fFirstProcId + subcnt;
            }

            iter.FinishSubEvent(subsz);
         } else {
            eventdone = false;
            break;
         }

      if (!eventdone) break;

      if (!iter.FinishEvent()) break;

      fEventCount++;
   }

   iter.Close();

}


void mbs::GeneratorModule::ProcessOutputEvent(dabc::Port* port)
{
   dabc::Buffer* buf = fPool->TakeBuffer(fBufferSize);

   FillRandomBuffer(buf);

   port->Send(buf);
}

extern "C" void StartMbsGenerator()
{
   dabc::mgr()->CreateThread("GenThrd", dabc::typeSocketThread);
   dabc::mgr()->CreateModule("mbs::GeneratorModule", "Generator", "GenThrd");
   dabc::mgr()->CreateTransport("Generator/Output", mbs::typeServerTransport, "GenThrd");
   dabc::mgr()->StartModule("Generator");
}


class MbsTestReadoutModule : public dabc::ModuleAsync {

   public:
      MbsTestReadoutModule(const char* name) :
         dabc::ModuleAsync(name)
      {
         dabc::BufferSize_t size = GetCfgInt(dabc::xmlBufferSize, 16*1024);

         dabc::PoolHandle* pool = CreatePoolHandle("Pool1", size, 5);

         dabc::Port* port = CreateInput("Input", pool, 5);

         port->GetCfgStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer));
         port->GetCfgStr(mbs::xmlServerName, "lxg0526");
         port->GetCfgInt(mbs::xmlServerPort, DefualtServerPort(mbs::TransportServer));

         DOUT1(("Start as client for node %s:%d", port->GetParStr(mbs::xmlServerName).c_str(), port->GetParInt(mbs::xmlServerPort)));
      }

      virtual void ProcessInputEvent(dabc::Port* port)
      {
         dabc::Buffer* buf = port->Recv();

         unsigned cnt = mbs::ReadIterator::NumEvents(buf);

         DOUT1(("Found %u events in MBS buffer", cnt));

         dabc::Buffer::Release(buf);
      }

      virtual void AfterModuleStop()
      {
      }
};

extern "C" void StartMbsClient()
{
//   const char* hostname = "lxg0526";
//   int nport = 6000;

   if (dabc::mgr()==0) {
      EOUT(("Manager is not created"));
      exit(1);
   }

   dabc::Module* m = new MbsTestReadoutModule("Receiver");

   dabc::mgr()->MakeThreadForModule(m);

   dabc::Command* cmd = new dabc::CmdCreateTransport("Receiver/Input", mbs::typeClientTransport, "MbsTransport");
//   cmd->SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer));
//   cmd->SetStr(mbs::xmlServerName, hostname);
//   if (nport>0) cmd->SetInt(mbs::xmlServerPort, nport);
//   cmd->SetInt(dabc::xmlBufferSize, BUFFERSIZE);

   if (!dabc::mgr()->Execute(cmd)) {
      EOUT(("Cannot create data input for receiver"));
      exit(1);
   }

   m->Start();

//   dabc::LongSleep(3);
}

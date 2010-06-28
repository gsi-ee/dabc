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
#include "mbs/GeneratorModule.h"

#include <math.h>

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Application.h"
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
   fEventCount = GetCfgInt("FirstEventCount", 0, cmd);;

   fStartStopPeriod = GetCfgInt("StartStopPeriod", 0, cmd);
   fNumSubevents = GetCfgInt("NumSubevents", 2, cmd);
   fFirstProcId = GetCfgInt("FirstProcId", 0, cmd);
   fSubeventSize = GetCfgInt("SubeventSize", 32, cmd);
   fIsGo4RandomFormat = GetCfgBool("Go4Random", true, cmd);
   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);

   DOUT1(("Generator buffer size %u procid %u numsub %u", fBufferSize, fFirstProcId, fNumSubevents));

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
         if (iter.NewSubevent(fSubeventSize, fFirstProcId + subcnt + 1, fFirstProcId + subcnt)) {

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

extern "C" void InitMbsGenerator()
{
   dabc::mgr()->CreateThread("GenThrd", dabc::typeSocketThread);
   dabc::mgr()->CreateModule("mbs::GeneratorModule", "Generator", "GenModThrd");
   dabc::mgr()->CreateTransport("Generator/Output", mbs::typeServerTransport, "GenThrd");
}



mbs::ReadoutModule::ReadoutModule(const char* name,  dabc::Command* cmd) :
    dabc::ModuleAsync(name, cmd)
{
	dabc::BufferSize_t size = GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);

	dabc::PoolHandle* pool = CreatePoolHandle("Pool1", size, 5);

	dabc::Port* port = CreateInput("Input", pool, 5);

	port->GetCfgStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::TransportServer), cmd);
	port->GetCfgStr(mbs::xmlServerName, "lxg0526", cmd);
	port->GetCfgInt(mbs::xmlServerPort, DefualtServerPort(mbs::TransportServer), cmd);

	DOUT1(("Start as client for node %s:%d", port->GetParStr(mbs::xmlServerName).c_str(), port->GetParInt(mbs::xmlServerPort)));
}

void mbs::ReadoutModule::ProcessInputEvent(dabc::Port* port)
{
	dabc::Buffer* buf = port->Recv();

	unsigned cnt = mbs::ReadIterator::NumEvents(buf);

	DOUT1(("Found %u events in MBS buffer", cnt));

	dabc::Buffer::Release(buf);
}


mbs::TransmitterModule::TransmitterModule(const char* name, dabc::Command* cmd) :
	dabc::ModuleAsync(name, cmd)
{
	dabc::BufferSize_t size = GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);

	CreatePoolHandle("Pool", size, 5);

	CreateInput("Input", Pool(), 5);

	CreateOutput("Output", Pool(), 5);
}

void mbs::TransmitterModule::retransmit()
{
	bool dostop = false;
	while (Output(0)->CanSend() && Input(0)->CanRecv()) {
		dabc::Buffer* buf = Input(0)->Recv();
		if (buf->GetTypeId() == dabc::mbt_EOF) {
			DOUT0(("See EOF - stop module"));
			dostop = true;
		}
		Output(0)->Send(buf);
	}

	if (dostop) {
	   Stop();
	   dabc::mgr()->GetApp()->InvokeCheckModulesCmd();
	}
}

extern "C" void InitMbsClient()
{
   dabc::mgr()->CreateModule("mbs::ReadoutModule", "Receiver", "ModuleThrd");

   dabc::mgr()->CreateTransport("Receiver/Input", mbs::typeClientTransport, "MbsTransport");
}

extern "C" void InitMbsTransmitter()
{
   dabc::mgr()->CreateModule("mbs::TransmitterModule", "Transmitter", "WorkerThrd");

   dabc::mgr()->CreateTransport("Transmitter/Input", mbs::typeTextInput, "WorkerThrd");

   dabc::mgr()->CreateTransport("Transmitter/Output", mbs::typeServerTransport, "MbsTransport");
}


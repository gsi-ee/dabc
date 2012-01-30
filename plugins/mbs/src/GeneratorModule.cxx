/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

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


mbs::GeneratorModule::GeneratorModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fEventCount = Cfg("FirstEventCount",cmd).AsInt(0);;

   fStartStopPeriod = Cfg("StartStopPeriod",cmd).AsInt(0);
   fNumSubevents = Cfg("NumSubevents",cmd).AsInt(2);
   fFirstProcId = Cfg("FirstProcId",cmd).AsInt(0);
   fSubeventSize = Cfg("SubeventSize",cmd).AsInt(32);
   fIsGo4RandomFormat = Cfg("Go4Random",cmd).AsBool(true);
   fFullId = Cfg("FullId",cmd).AsUInt(0);

   fBufferSize = Cfg(dabc::xmlBufferSize,cmd).AsInt(16*1024);

   DOUT1(("Generator buffer size %u numsub %u procid %u or fullid 0x%x", fBufferSize, fNumSubevents, fFirstProcId, fFullId));

   fPool = CreatePoolHandle("Pool");

   fShowInfo = true;

   CreateOutput("Output", fPool, 5);

   // create parameter which will be updated once a second
   CreatePar("Info", "info").SetSynchron(true, 1.);

   dabc::Parameter par = CreatePar("Rate");
   par.SetRatemeter(true);
   par.SetUnits("ev");

   CreateCmdDef("ToggleInfo").AddArg("toggle","int", false);
}

int mbs::GeneratorModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ToggleInfo")) {
      fShowInfo = !fShowInfo;
      Par("Info").SetStr(dabc::format("%sable info, toggle %d", (fShowInfo ? "En" : "Dis"), cmd.Field("toggle").AsInt()));
      Par("Info").FireModified();
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void mbs::GeneratorModule::FillRandomBuffer(dabc::Buffer& buf)
{
   if (buf.null()) { EOUT(("No free buffer - generator will block")); return; }

   mbs::WriteIterator iter(buf);

   while (iter.NewEvent(fEventCount)) {

      bool eventdone = true;

      for (unsigned subcnt = 0; subcnt < fNumSubevents; subcnt++)
         if (iter.NewSubevent(fSubeventSize, fFirstProcId + subcnt + 1, fFirstProcId + subcnt)) {

            if ((fFullId!=0) && (fNumSubevents==1))
               iter.subevnt()->fFullId = fFullId;

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

      Par("Rate").SetDouble(1.);

      fEventCount++;
   }

   // When close iterator - take back buffer with correctly modified length field
   buf << iter.Close();

   if (fShowInfo)
      Par("Info").SetStr(dabc::format("Produce event %d", fEventCount));
}


void mbs::GeneratorModule::ProcessOutputEvent(dabc::Port* port)
{
   dabc::Buffer buf = Pool()->TakeBuffer(fBufferSize);

   FillRandomBuffer(buf);

   port->Send(buf);
}

extern "C" void InitMbsGenerator()
{
   dabc::mgr()->CreateThread("GenThrd", dabc::typeSocketThread);
   dabc::mgr.CreateMemoryPool("Pool");
   dabc::mgr.CreateModule("mbs::GeneratorModule", "Generator", "GenModThrd");
   dabc::mgr.CreateTransport("Generator/Output", mbs::typeServerTransport, "GenThrd");
}



mbs::ReadoutModule::ReadoutModule(const char* name,  dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
	CreatePoolHandle("Pool1");

	CreateInput("Input", Pool(), 5);

//	DOUT1(("Start as client for node %s:%d", port->Par(mbs::xmlServerName).AsStr(""), port->Par(mbs::xmlServerPort).AsInt()));
}

void mbs::ReadoutModule::ProcessInputEvent(dabc::Port* port)
{
	dabc::Buffer buf = port->Recv();

	unsigned cnt = mbs::ReadIterator::NumEvents(buf);

	DOUT1(("Found %u events in MBS buffer", cnt));

	buf.Release();
}


mbs::TransmitterModule::TransmitterModule(const char* name, dabc::Command cmd) :
	dabc::ModuleAsync(name, cmd)
{
	CreatePoolHandle("Pool");

	CreateInput("Input", Pool(), 5);

	CreateOutput("Output", Pool(), 5);

	fReconnect = Cfg("Reconnect", cmd).AsBool(false);

	// create timer, but do not enable it
	if (fReconnect) CreateTimer("Reconn", -1);

   CreatePar("TransmitData").SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 3.).SetUnits("Buf");
}

void mbs::TransmitterModule::BeforeModuleStart()
{
   // in case of reconnect allowed shoot timer to verify that connection is there
   if (fReconnect) ShootTimer("Reconn", 0.1);
}


void mbs::TransmitterModule::retransmit()
{
	bool dostop = false;
	while (Output(0)->CanSend() && Input(0)->CanRecv()) {
		dabc::Buffer buf = Input(0)->Recv();
		if (buf.GetTypeId() == dabc::mbt_EOF) {
			DOUT0(("See EOF - stop module"));
			dostop = true;
		} else {
		   Par("TransmitData").SetDouble(buf.GetTotalSize()/1024./1024);
		   Par("TransmitBufs").SetDouble(1.);
		}
		Output(0)->Send(buf);
	}

	if (dostop) {
	   DOUT0(("Doing stop???"));
	   dabc::mgr.StopApplication();
	}
}

void mbs::TransmitterModule::ProcessDisconnectEvent(dabc::Port* port)
{
   DOUT0(("Port %s disconnected from retransmitter", port->GetName()));

   if (fReconnect && port->IsName("Input")) {
      DOUT0(("We will try to reconnect input as far as possible"));
      ShootTimer("Reconn", 2.);
   } else {
      dabc::mgr.StopApplication();
   }
}

void mbs::TransmitterModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (!fReconnect || Input(0)->IsConnected()) return;

   std::string item = Input(0)->ItemName();

   bool res = dabc::mgr.CreateTransport(item, mbs::typeClientTransport, "MbsTransport");

   if (res) DOUT0(("Port %s is reconnected again!!!", item.c_str()));
       else ShootTimer("Reconn", 2.);
}



extern "C" void InitMbsClient()
{
   dabc::mgr.CreateModule("mbs::ReadoutModule", "Receiver", "ModuleThrd");

   dabc::mgr.CreateTransport("Receiver/Input", mbs::typeClientTransport, "MbsTransport");
}

extern "C" void InitMbsTransmitter()
{
   dabc::mgr.CreateModule("mbs::TransmitterModule", "Transmitter", "WorkerThrd");

   dabc::mgr.CreateTransport("Transmitter/Input", mbs::typeTextInput, "WorkerThrd");

   dabc::mgr.CreateTransport("Transmitter/Output", mbs::typeServerTransport, "MbsTransport");
}

extern "C" void InitMbsRepeater()
{
   dabc::mgr.CreateMemoryPool("Pool"); // size and buf number should be specified in xml file

   dabc::mgr.CreateModule("mbs::TransmitterModule", "Repeater", "WorkerThrd");

   dabc::mgr.CreateTransport("Repeater/Input", mbs::typeClientTransport, "MbsTransport");

   dabc::mgr.CreateTransport("Repeater/Output", mbs::typeServerTransport, "MbsTransport");
}


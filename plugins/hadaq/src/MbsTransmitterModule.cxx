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

#include "hadaq/MbsTransmitterModule.h"

#include <math.h>

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "mbs/MbsTypeDefs.h"



hadaq::MbsTransmitterModule::MbsTransmitterModule(const char* name, dabc::Command cmd) :
	dabc::ModuleAsync(name, cmd)
{
	CreatePoolHandle("Pool");

	CreateInput("Input", Pool(), 5);

	CreateOutput("Output", Pool(), 5);

//	fReconnect = Cfg("Reconnect", cmd).AsBool(false);
//
//	// create timer, but do not enable it
//	if (fReconnect) CreateTimer("Reconn", -1);

   CreatePar("TransmitData").SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 3.).SetUnits("Buf");
}

void hadaq::MbsTransmitterModule::BeforeModuleStart()
{
   // in case of reconnect allowed shoot timer to verify that connection is there
   //if (fReconnect) ShootTimer("Reconn", 0.1);
}


void hadaq::MbsTransmitterModule::retransmit()
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
		// TODO: put here wrapping  of hadtu format into mbs subevent like in go4





		Output(0)->Send(buf);
	}

	if (dostop) {
	   DOUT0(("Doing stop???"));
	   dabc::mgr.StopApplication();
	}
}

void hadaq::MbsTransmitterModule::ProcessDisconnectEvent(dabc::Port* port)
{
//   DOUT0(("Port %s disconnected from retransmitter", port->GetName()));
//
//   if (fReconnect && port->IsName("Input")) {
//      DOUT0(("We will try to reconnect input as far as possible"));
//      ShootTimer("Reconn", 2.);
//   } else {
//      dabc::mgr.StopApplication();
//   }
}

void hadaq::MbsTransmitterModule::ProcessTimerEvent(dabc::Timer* timer)
{
//   if (!fReconnect || Input(0)->IsConnected()) return;
//
//   std::string item = Input(0)->ItemName();
//
//   bool res = dabc::mgr.CreateTransport(item, hadaq::typeClientTransport, "MbsTransport");
//
//   if (res) DOUT0(("Port %s is reconnected again!!!", item.c_str()));
//       else ShootTimer("Reconn", 2.);
}




// This one will transmit file to mbs transport server:
extern "C" void InitHadaqMbsTransmitter()
{
   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "Transmitter", "WorkerThrd");

   dabc::mgr.CreateTransport("Transmitter/Input", hadaq::typeHldInput, "WorkerThrd");

   dabc::mgr.CreateTransport("Transmitter/Output", mbs::typeServerTransport, "MbsTransport");
}

// this will serve data at mbs transport/stream, as received on one udp (daq_netmem like) input
extern "C" void InitHadaqMbsServer()
{
   dabc::mgr.CreateMemoryPool("Pool"); // size and buf number should be specified in xml file

   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "Server", "WorkerThrd");

   dabc::mgr.CreateTransport("Repeater/Input", hadaq::typeUdpInput, "UdpThrd");

   dabc::mgr.CreateTransport("Repeater/Output", mbs::typeServerTransport, "MbsTransport");
}


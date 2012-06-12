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
#include "hadaq/Iterator.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"



hadaq::MbsTransmitterModule::MbsTransmitterModule(const char* name, dabc::Command cmd) :
	dabc::ModuleAsync(name, cmd)
{
   CreatePar(dabc::xmlBufferSize).DfltInt(65536);
	CreatePar(dabc::xmlNumBuffers).DfltInt(20);
	CreatePar(hadaq::xmlFileName).DfltStr("");
	CreatePar(mbs::xmlFileName).DfltStr("");


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
		dabc::Buffer inbuf = Input(0)->Recv();
		if (inbuf.GetTypeId() == dabc::mbt_EOF) {
			DOUT1(("See EOF - stop module"));
			dostop = true;
		}

		// TODO: put here wrapping  of hadtu format into mbs subevent like in go4
		dabc::Buffer outbuf = Pool()->TakeBuffer();
		dabc::BufferSize_t usedsize=0;
		mbs::WriteIterator miter(outbuf);
		//unsigned eventcounter=0;
		hadaq::ReadIterator hiter(inbuf);
		hadaq::Event* hev;
		while(hiter.NextEvent())
		{
		   hev=hiter.evnt();
		   size_t evlen=hev->GetSize();
		   DOUT3(("retransmit - event %d of size %d",hev->GetSeqNr(),evlen));

		   unsigned id = hev->GetId();
		   unsigned evcount=hev->GetSeqNr();
		   // assign and check event/subevent header for wrapper structures:
		   if (!miter.NewEvent(evcount))
		      {
		         DOUT1(("Can not add new mbs event to output buffer anymore - stop module. Check mempool setup!"));
		         dostop = true;
		         break;
		      }
		   mbs::EventHeader* mev=miter.evnt();
		   usedsize+=sizeof(mbs::EventHeader);
		   if (!miter.NewSubevent(evlen))
		      {
		           DOUT1(("Can not add new mbs subevent to output buffer anymore - stop module. Check mempool setup!"));
		           dostop = true;
		           break;
		      }
		   usedsize+=sizeof(mbs::SubeventHeader);
		   // OK, we copy full hadaq event into mbs subevent payload:
		    mbs::SubeventHeader* msub=miter.subevnt();
		    msub->fFullId = id; // TODO: configure mbs ids from parameters
		    memcpy(miter.rawdata(),hev,evlen);
		    usedsize+=evlen;
		    miter.FinishSubEvent(evlen);
		    miter.FinishEvent();
		    mev->SetSubEventsSize(evlen+sizeof(mbs::SubeventHeader));
		    DOUT3(("retransmit - used size %d",usedsize));
		} // while hiter.NextEvent()





		outbuf.SetTotalSize(usedsize);
		outbuf.SetTypeId(mbs::mbt_MbsEvents);
		Par("TransmitData").SetDouble(outbuf.GetTotalSize()/1024./1024);
		Par("TransmitBufs").SetDouble(1.);
		Output(0)->Send(outbuf);
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
   dabc::mgr.CreateMemoryPool("Pool",65536,100); // size and buf number should be specified in xml file

   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "HldServer", "WorkerThrd");



   dabc::mgr.CreateTransport("HldServer/Input", hadaq::typeHldInput, "WorkerThrd");

   dabc::mgr.CreateTransport("HldServer/Output", mbs::typeServerTransport, "MbsTransport");
}


extern "C" void InitHadaqMbsConverter()
{
   dabc::mgr.CreateMemoryPool("Pool",65536,100); // size and buf number should be specified in xml file

   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "HldConv", "WorkerThrd");



   dabc::mgr.CreateTransport("HldConv/Input", hadaq::typeHldInput, "WorkerThrd");

   dabc::mgr.CreateTransport("HldConv/Output", mbs::typeLmdOutput, "MbsTransport");
}



// this will serve data at mbs transport/stream, as received on one udp (daq_netmem like) input
extern "C" void InitHadaqMbsServer()
{
   dabc::mgr.CreateMemoryPool("Pool"); // size and buf number should be specified in xml file

   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "Repeater", "WorkerThrd");

   dabc::mgr.CreateTransport("Repeater/Input", hadaq::typeUdpInput, "UdpThrd");

   dabc::mgr.CreateTransport("Repeater/Output", mbs::typeServerTransport, "MbsTransport");
}


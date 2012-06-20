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

	CreatePoolHandle("Pool");
	CreateInput("Input", Pool(), 5);
	CreateOutput("Output", Pool(), 5);

   CreatePar("TransmitData").SetRatemeter(false, 10.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 10.).SetUnits("Buf");
   CreatePar("TransmitEvents").SetRatemeter(false, 10.).SetUnits("Evts");
   Par("TransmitData").SetDebugLevel(1);
   Par("TransmitBufs").SetDebugLevel(1);
   Par("TransmitEvents").SetDebugLevel(1);
}



void hadaq::MbsTransmitterModule::retransmit()
{
	bool dostop = false;
	while (Output(0)->CanSend() && Input(0)->CanRecv()) {
		dabc::Buffer inbuf = Input(0)->Recv();
		if (inbuf.GetTypeId() == dabc::mbt_EOF) {
			DOUT1(("See EOF - stop module"));
			dostop = true;
			break;
		}

		// here wrapping  of hadtu format into mbs subevent like in go4 user source
		dabc::Buffer outbuf = Pool()->TakeBuffer();
		dabc::BufferSize_t usedsize=0;
		mbs::WriteIterator miter(outbuf);
		hadaq::ReadIterator hiter(inbuf);
		hadaq::Event* hev;
		while(hiter.NextEvent())
		{
		   hev=hiter.evnt();
		   size_t evlen=hev->GetPaddedSize();
		   DOUT5(("retransmit - event %d of size %d",hev->GetSeqNr(),evlen));

		   unsigned id = hev->GetId();
		   unsigned evcount=hev->GetSeqNr();
		   // assign and check event/subevent header for wrapper structures:
		   if (!miter.NewEvent(evcount))
		      {
		         DOUT1(("Can not add new mbs event to output buffer anymore - stop module. Check mempool setup!"));
		         dostop = true;
		         break;
		      }
		   //mbs::EventHeader* mev=miter.evnt();
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
		    Par("TransmitEvents").SetDouble(1.);
		    DOUT3(("retransmit - used size %d",usedsize));
		} // while hiter.NextEvent()

		if(dostop) break;
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




// This one will transmit file to mbs transport server:
extern "C" void InitHadaqMbsTransmitter()
{
   dabc::mgr.CreateMemoryPool("Pool");
   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "HldServer", "WorkerThrd");
   dabc::mgr.CreateTransport("HldServer/Input", hadaq::typeHldInput, "WorkerThrd");
   dabc::mgr.CreateTransport("HldServer/Output", mbs::typeServerTransport, "MbsTransport");

   unsigned secs=30;
   DOUT1(("InitHadaqMbsTransmitter sleeps %d seconds before client connect", secs));
   dabc::mgr.Sleep(secs);
}


extern "C" void InitHadaqMbsConverter()
{
   dabc::mgr.CreateMemoryPool("Pool");
   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "HldConv", "WorkerThrd");
   dabc::mgr.CreateTransport("HldConv/Input", hadaq::typeHldInput, "WorkerThrd");
   dabc::mgr.CreateTransport("HldConv/Output", mbs::typeLmdOutput, "WorkerThrd");
}



// this will serve data at mbs transport/stream, as received on one udp (daq_netmem like) input
extern "C" void InitHadaqMbsServer()
{
   dabc::mgr.CreateMemoryPool("Pool"); // size and buf number should be specified in xml file
   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "NetmemServer", "WorkerThrd");
   dabc::mgr.CreateTransport("NetmemServer/Input", hadaq::typeUdpInput, "UdpThrd");
   dabc::mgr.CreateTransport("NetmemServer/Output", mbs::typeServerTransport, "MbsTransport");
}


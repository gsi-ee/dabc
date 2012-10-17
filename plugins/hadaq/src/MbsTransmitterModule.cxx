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




//#define HADAQ_MBSTRANSMITTER_TESTRECEIVE 1

hadaq::MbsTransmitterModule::MbsTransmitterModule(const char* name, dabc::Command cmd) :
	dabc::ModuleAsync(name, cmd)
{

   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStdStr(dabc::xmlWorkPool);

	CreatePoolHandle(poolname.c_str());

	CreateInput(mbs::portInput, Pool(), 5);
	CreateOutput(mbs::portOutput, Pool(), 5);

	fSubeventId = Cfg(hadaq::xmlMbsSubeventId, cmd).AsInt(0x000001F);
	fMergeSyncedEvents = Cfg(hadaq::xmlMbsMergeSyncMode, cmd).AsBool(false);
	fMergeSyncMaxEvents = Cfg(hadaq::xmlMbsMergeLimit, cmd).AsInt(20);

	DOUT0(("hadaq:TransmitterModule subevid = 0x%x, merge sync mode = %d", (unsigned) fSubeventId, fMergeSyncedEvents));

   CreatePar("TransmitData").SetRatemeter(false, 5.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 5.).SetUnits("Buf");
   CreatePar("TransmitEvents").SetRatemeter(false, 5.).SetUnits("Evts");
   if (Par("TransmitData").GetDebugLevel()<0) Par("TransmitData").SetDebugLevel(1);
   if (Par("TransmitBufs").GetDebugLevel()<0) Par("TransmitBufs").SetDebugLevel(1);
   if (Par("TransmitEvents").GetDebugLevel()<0) Par("TransmitEvents").SetDebugLevel(1);
}


void hadaq::MbsTransmitterModule::retransmit()
{
   DOUT5(("MbsTransmitterModule::retransmit() starts"));
#ifdef HADAQ_MBSTRANSMITTER_TESTRECEIVE
      if(Input(0)->CanRecv())
      {
         dabc::Buffer inbuf = Input(0)->Recv();
         size_t bufferlen=0;
         if(!inbuf.null()) bufferlen=inbuf.GetTotalSize();
         Par("TransmitData").SetDouble(bufferlen/1024./1024);
         Par("TransmitBufs").SetDouble(1.);
         inbuf.Release();
      }
      return;
#endif

      bool firstevent=true;
      unsigned mergecount=0;


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
		int evcount=0;
		size_t totalevlen=0;
		void* destcursor=0;
		while(hiter.NextEvent())
		{
		   hev=hiter.evnt();
		   size_t evlen=hev->GetPaddedSize();
         if (!firstevent) {
            if (fMergeSyncedEvents && evcount == hev->GetSeqNr()
                  && mergecount++ < fMergeSyncMaxEvents) {
               // OK, we continue merging to first event
            } else {
               DOUT0(
                     ("close output event %d of size %d, mergecount:%d",hev->GetSeqNr(),totalevlen, mergecount));
               // close current mbs event, start next:
               miter.FinishSubEvent(totalevlen);
               miter.FinishEvent();
               Par("TransmitEvents").SetDouble(1.);
               DOUT5(("retransmit - used size %d",usedsize));
               firstevent = true;
               totalevlen = 0;
               usedsize = 0;
            }
         } //  if(!firstevent)
		   totalevlen+=evlen;
		   DOUT0(("retransmit - event %d of size %d",hev->GetSeqNr(),evlen));

         if (firstevent) {
            firstevent = false;
            //unsigned id = hev->GetId();
            evcount = hev->GetSeqNr();
            // assign and check event/subevent header for wrapper structures:
            if (!miter.NewEvent(evcount)) {
               DOUT1(
                     ("Count limit. Can not add new mbs event to output buffer anymore - stop module. Check mempool setup!"));
               dostop = true;
               break;
            }
            //mbs::EventHeader* mev=miter.evnt();
            usedsize += sizeof(mbs::EventHeader);
            if (!miter.NewSubevent(evlen)) {
               DOUT1(("Len limit: %u Buffer: %u Can not add new mbs subevent to output buffer anymore - stop module. Check mempool setup!", (unsigned) evlen, (unsigned) inbuf.GetTotalSize()));
               dostop = true;
               break;
            }
            usedsize += sizeof(mbs::SubeventHeader);
            destcursor = miter.rawdata();
         }  // firstevent


		   // OK, we copy full hadaq event into mbs subevent payload:
		    mbs::SubeventHeader* msub=miter.subevnt();
		    msub->iProcId=fSubeventId; // configured for TTrbProc etc.
		    memcpy(destcursor,hev,evlen);
		    usedsize+=evlen;

		} // while hiter.NextEvent()

		if(dostop) break;
		outbuf.SetTotalSize(usedsize);
		outbuf.SetTypeId(mbs::mbt_MbsEvents);
		Par("TransmitData").SetDouble(outbuf.GetTotalSize()/1024./1024);
		Par("TransmitBufs").SetDouble(1.);
		Output(0)->Send(outbuf);
	}
	DOUT5(("MbsTransmitterModule::retransmit() leaves"));
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


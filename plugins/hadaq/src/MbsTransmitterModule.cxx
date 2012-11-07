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

   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStdStr(dabc::xmlWorkPool);

   CreatePoolHandle(poolname.c_str());

   CreateInput(mbs::portInput, Pool(), 5);
   CreateOutput(mbs::portOutput, Pool(), 5);

   fSubeventId = Cfg(hadaq::xmlMbsSubeventId, cmd).AsInt(0x000001F);
   fMergeSyncedEvents = Cfg(hadaq::xmlMbsMergeSyncMode, cmd).AsBool(false);
   fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(3);
   fPrintSync = Cfg("PrintSync", cmd).AsBool(false);

   DOUT0(("hadaq:TransmitterModule subevid = 0x%x, merge sync mode = %d", (unsigned) fSubeventId, fMergeSyncedEvents));

   CreatePar("TransmitData").SetRatemeter(false, 5.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 5.).SetUnits("Buf");
   CreatePar("TransmitEvents").SetRatemeter(false, 5.).SetUnits("Ev");

   fLastEventNumber = -1;
   if (fFlushTimeout > 0.)
      CreateTimer("FlushTimer", fFlushTimeout, false);
      
   fIgnoreEvent = -1;
   fCounter = 0;
}


bool hadaq::MbsTransmitterModule::retransmit()
{
   DOUT5(("MbsTransmitterModule::retransmit() starts"));

   // we need at least one entry in the outout queue before we start doing something
   if (!Output(0)->CanSend()) return false;

   // nothing to do
   if (!Input(0)->CanRecv()) return false;

   if (fTgtBuf.null()) {
      fTgtBuf = Pool()->TakeBuffer();
      if (fTgtBuf.null()) return false;
      fHdrPtr = fTgtBuf.GetPointer();
      fDataPtr.reset();
      fLastEventNumber = -1;
      // fDataPtr = fHdrPtr; fDataPtr.shift(sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader));
   }

   dabc::Buffer inbuf = Input(0)->Recv();
   if (inbuf.GetTypeId() == dabc::mbt_EOF) {
      DOUT1(("See EOF - stop module"));
      dabc::mgr.StopApplication();
      return false;
   }

   hadaq::ReadIterator hiter(inbuf);
   while(hiter.NextEvent()) {
       if ((fIgnoreEvent>=0) && (hiter.evnt()->GetSeqNr()==fIgnoreEvent)) continue;
     
      if (!fMergeSyncedEvents || (fLastEventNumber != hiter.evnt()->GetSeqNr())) {
         // first close existing events
         CloseCurrentEvent();
         if (fHdrPtr.fullsize()<200) FlushBuffer();
      }

      size_t evlen = hiter.evnt()->GetPaddedSize();

      if (fLastEventNumber<0) {
         // start header

         if (fHdrPtr.null()) {
            EOUT(("Something wrong with header"));
            exit(5);
         }

         fDataPtr = fHdrPtr;
         fDataPtr.shift(sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader));
      //   DOUT0(("Starting dummy event with size %u", fHdrPtr.distance_to(fDataPtr)));
         fLastEventNumber = hiter.evnt()->GetSeqNr();
         fFlushTimeout = -1;

         if (fDataPtr.null()) {
            EOUT(("Something went wrong"));
            exit(5);
         }

      }

      if (fDataPtr.fullsize() <= evlen) FlushBuffer();

      if ((fDataPtr.fullsize() <= evlen) && (fHdrPtr.fullsize() == fTgtBuf.GetTotalSize())) {
         // event in very beginning, flush it and ignore all its other parts
          FlushBuffer(true);
          DOUT2(("Ignore rest of the event %d", fIgnoreEvent));
          fIgnoreEvent = hiter.evnt()->GetSeqNr();
          continue;
      }

      if (fDataPtr.fullsize() <= evlen) {
         EOUT(("Should not be - do something  dataptr %u  evlen %u", fDataPtr.fullsize(), evlen));
         exit(2);
      }

//      if (fDataPtr.fullsize() == evlen) EOUT(("DANGER!!!!"));

      fDataPtr.copyfrom(hiter.evnt(), evlen);
      // append to the buffer

      fDataPtr.shift(evlen);
   }

   if (!fMergeSyncedEvents) {
      // first close existing events
      CloseCurrentEvent();
      if (fHdrPtr.fullsize()<200) FlushBuffer();
   }

   return true;
}

void hadaq::MbsTransmitterModule::CloseCurrentEvent()
{

    // do nothing
   if (fLastEventNumber<0) return;

   if (fDataPtr.null() && (fLastEventNumber>=0)) {
      EOUT(("Something wrong evid = %d, but data empty", fLastEventNumber));
      return;
   }
   

   unsigned fullsize = fHdrPtr.distance_to(fDataPtr);
   mbs::EventHeader ev;
   ev.Init(fLastEventNumber);
   ev.SetFullSize(fullsize);

   DOUT2(("Building event %d of size %u", fLastEventNumber, fullsize));
   fCounter++;

   mbs::SubeventHeader sub;
   sub.fFullId = fSubeventId;
   sub.SetFullSize(fullsize - sizeof(ev));

   if (fHdrPtr.fullsize() < 30) {
      EOUT(("Something went wrong   evid = %d hdrptr size = %u", fLastEventNumber, fHdrPtr.fullsize()));
   }

   fHdrPtr.copyfrom(&ev, sizeof(ev));
   fHdrPtr.shift(sizeof(ev));
   fHdrPtr.copyfrom(&sub, sizeof(sub));
   fHdrPtr.shift(fullsize - sizeof(ev));
   fDataPtr.reset();

   fLastEventNumber = -1;
}



void hadaq::MbsTransmitterModule::FlushBuffer(bool force)
{
   dabc::Buffer newbuf;
   unsigned newbufused(0);
   int newevid = -1;

   if (fLastEventNumber >= 0) {
      // we need to copy part of event in the new buffer
      if (fHdrPtr.fullsize() == fTgtBuf.GetTotalSize()) {
         // this is a case when event start from buffer begin, therefore copy of event data will not help
         if (!force) return;
         DOUT2(("Very special case - closing event when events starts from buffer begin and must be flushed"));
         CloseCurrentEvent();
      } else {
         // we copy partial data  to new buffer

//          DOUT0(("Coping of partial data into new buffer"));

         DOUT2(("Current event %d  isdatanull %s", fLastEventNumber, DBOOL(fDataPtr.null())));

         newbuf = Pool()->TakeBuffer();
         dabc::Pointer new_ptr = newbuf.GetPointer();
         newbufused = fHdrPtr.distance_to(fDataPtr);
         
         DOUT2(("Shift data to new buffer size:%u move:%u", newbuf.GetTotalSize(), newbufused));
         
         new_ptr.copyfrom(fHdrPtr, newbufused);

         newevid = fLastEventNumber;
         // mark as we do not fill data in the fDataPtr
         fDataPtr.reset();
         fLastEventNumber = -1;
      }
   }

   if (fLastEventNumber >= 0) {
      EOUT(("Something went wrong"));
      return;
   }

   if (!fTgtBuf.null() && (fHdrPtr.fullsize() < fTgtBuf.GetTotalSize())) {
       fTgtBuf.SetTotalSize(fTgtBuf.GetTotalSize() - fHdrPtr.fullsize());
       fTgtBuf.SetTypeId(mbs::mbt_MbsEvents);

       Par("TransmitBufs").SetDouble(1.);
       Par("TransmitData").SetDouble(fTgtBuf.GetTotalSize()/1024./1024.);
       Par("TransmitEvents").SetDouble(fCounter);
       fCounter = 0;
//       DOUT0(("Send buffe of size %u",  fTgtBuf.GetTotalSize()));
       if (Output()->CanSend())
          Output()->Send(fTgtBuf.HandOver());
       else {
          fTgtBuf.Release();
          DOUT0(("Drop buffer !!!"));
       }
       fLastFlushTime.GetNow();
   }


   if (newbuf.null())
      fTgtBuf = Pool()->TakeBuffer();
   else
      fTgtBuf = newbuf.HandOver();

   fHdrPtr = fTgtBuf.GetPointer();
   fDataPtr.reset();

   if ((newbufused>0) && (newevid>=0)) {
      if (fHdrPtr.null()) {
         EOUT(("Something went wrong"));
         return;
      }

      fDataPtr = fHdrPtr;
      fDataPtr.shift(newbufused);
      fLastEventNumber = newevid;

         if (fDataPtr.null()) {
            EOUT(("Something went wrong"));
            exit(5);
         }

   }
}

void hadaq::MbsTransmitterModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (fLastFlushTime.null()) fLastFlushTime.GetNow();

   double tm = fLastFlushTime.SpentTillNow();

   if (tm > fFlushTimeout)
      FlushBuffer( tm > 5*fFlushTimeout);
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

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

#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"


hadaq::MbsTransmitterModule::MbsTransmitterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   // we need at least one input and one output port
   EnsurePorts(1, 1, dabc::xmlWorkPool);

   fSubeventId = Cfg(hadaq::xmlMbsSubeventId, cmd).AsInt(0x000001F);
   fMergeSyncedEvents = Cfg(hadaq::xmlMbsMergeSyncMode, cmd).AsBool(false);
   double flushtime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(3);
   // fPrintSync = Cfg("PrintSync", cmd).AsBool(false);

   DOUT0("hadaq:TransmitterModule subevid = 0x%x, merge sync mode = %d", (unsigned) fSubeventId, fMergeSyncedEvents);

   CreatePar("TransmitData").SetRatemeter(false, 5.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 5.).SetUnits("Buf");
   CreatePar("TransmitEvents").SetRatemeter(false, 5.).SetUnits("Ev");

   if (flushtime > 0.)
      CreateTimer("FlushTimer", flushtime, false);

   fCurrentEventNumber = -1;

   fIgnoreEvent = -1;
   fEvCounter = 0;
   fFlushCnt = 2;
}


bool hadaq::MbsTransmitterModule::retransmit()
{
   DOUT5("MbsTransmitterModule::retransmit() starts");

   // nothing to do
   if (!CanSendToAllOutputs()) return false;

   // is source do not have pointer, reset it to read new buffer
   if (!fSrcIter.IsData()) {
      if (!CanRecv()) return false;

      dabc::Buffer buf = Recv();
      if (buf.GetTypeId() == dabc::mbt_EOF) {
         DOUT1("See EOF - stop module");
         SendToAllOutputs(buf);
         return false;
      }
      fSrcIter.Reset(buf);
      // locate to the first event
      fSrcIter.NextEvent();
      return true; // we could try next buffer as soon as possible
   }

   // ignore all events with selected id
   if (fIgnoreEvent>=0) {
      if (fIgnoreEvent == fSrcIter.evnt()->GetSeqNr()) {
         fSrcIter.NextEvent();
         return true;
      }
      fIgnoreEvent = -1;
   }


   if (!fTgtIter.IsBuffer()) {
       dabc::Buffer buf = TakeBuffer();
       if (buf.null()) return false;
       fTgtIter.Reset(buf);
       // fDataPtr = fHdrPtr; fDataPtr.shift(sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader));
    }

   // close current event if it not will be merged with next source event
   if (fTgtIter.IsEventStarted()) {
      if (!fMergeSyncedEvents || (fCurrentEventNumber != fSrcIter.evnt()->GetSeqNr())) {
         // first close existing events
         fTgtIter.FinishSubEvent();
         fTgtIter.FinishEvent();
         fCurrentEventNumber = -1;
         fEvCounter++;
      }
   }


   size_t evlen = fSrcIter.evnt()->GetPaddedSize();


   bool has_required_place = false;
   if (fTgtIter.IsEventStarted())
      has_required_place = fTgtIter.IsPlaceForRawData(evlen);
   else
      has_required_place = fTgtIter.IsPlaceForEvent(evlen, true);

   bool doflush = (fFlushCnt<=0);

   if (!has_required_place || doflush) {

      // this is completely empty buffer, we should ignore event when it does not fit into empty buffer
      if (!has_required_place && !fTgtIter.IsAnyData()) {
         fIgnoreEvent = fSrcIter.evnt()->GetSeqNr();
         return true;
      }

      // mo sense to make flush when no any
      if (doflush && !fTgtIter.IsAnyEvent()) return false;

      dabc::Buffer sendbuf;

      if (fTgtIter.IsAnyUncompleteData()) {
         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) return false;

         sendbuf = fTgtIter.CloseAndTransfer(buf);
      } else {
         sendbuf = fTgtIter.Close();
      }

      fFlushCnt = 2; // two timout without sending should happen before flush will appear

      Par("TransmitData").SetDouble(sendbuf.GetTotalSize() / 1024./1024.);
      Par("TransmitBufs").SetDouble(1.);
      Par("TransmitEvents").SetDouble(fEvCounter);
      fEvCounter = 0;

      SendToAllOutputs(sendbuf);
      return true;
   }


   if (!fTgtIter.IsEventStarted()) {
      fCurrentEventNumber = fSrcIter.evnt()->GetSeqNr();

      // create proper MBS event header and
      fTgtIter.NewEvent(fCurrentEventNumber);
      fTgtIter.NewSubevent2(fSubeventId);
   }

   fTgtIter.AddRawData(fSrcIter.evnt(), evlen);

   fSrcIter.NextEvent();

   // close current event if next portion will have another id
   if (fSrcIter.IsData()) {
      if (!fMergeSyncedEvents || (fCurrentEventNumber != fSrcIter.evnt()->GetSeqNr())) {
         // first close existing events
         fTgtIter.FinishSubEvent();
         fTgtIter.FinishEvent();
         fCurrentEventNumber = -1;
         fEvCounter++;
      }
   }


   return true;
}

void hadaq::MbsTransmitterModule::CloseCurrentEvent()
{

}



void hadaq::MbsTransmitterModule::FlushBuffer(bool force)
{
}

void hadaq::MbsTransmitterModule::ProcessTimerEvent(unsigned timer)
{
   if (fFlushCnt>0) fFlushCnt--;
}


// This one will transmit file to mbs transport server:
extern "C" void InitHadaqMbsTransmitter()
{
   dabc::mgr.CreateMemoryPool("Pool");
   dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "HldServer", "WorkerThrd");
   dabc::mgr.CreateTransport("HldServer/Input0");
   dabc::mgr.CreateTransport("HldServer/Output0");

//   unsigned secs=30;
//   DOUT1("InitHadaqMbsTransmitter sleeps %d seconds before client connect", secs);
//   dabc::mgr.Sleep(secs);
}

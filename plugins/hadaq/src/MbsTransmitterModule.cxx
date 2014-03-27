// $Id$

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
   dabc::ModuleAsync(name, cmd),
   fSrcIter(),
   fTgtIter(),
   fCurrentEventNumber(0),
   isCurrentEventNumber(false),
   fIgnoreEvent(0),
   isIgnoreEvent(false),
   fEvCounter(0),
   fSubeventId(0),
   fMergeSyncedEvents(false),
   fFlushCnt(0),
   feofbuf()
{
   // we need at least one input and one output port
   EnsurePorts(1, 1, dabc::xmlWorkPool);

   fSubeventId = Cfg(hadaq::xmlMbsSubeventId, cmd).AsInt(0x000001F);
   fMergeSyncedEvents = Cfg(hadaq::xmlMbsMergeSyncMode, cmd).AsBool(false);
   double flushtime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(3);

   DOUT0("hadaq:TransmitterModule subevid = 0x%x, merge sync mode = %d", (unsigned) fSubeventId, fMergeSyncedEvents);

   CreatePar("TransmitData").SetRatemeter(false, 5.).SetUnits("MB");
   CreatePar("TransmitBufs").SetRatemeter(false, 5.).SetUnits("Buf");
   CreatePar("TransmitEvents").SetRatemeter(false, 5.).SetUnits("Ev");

   CreateCmdDef("StartLmdFile")
      .AddArg("filename", "string", true, "file.lmd")
      .AddArg(dabc::xml_maxsize, "int", false, 50)
      .SetArgMinMax(dabc::xml_maxsize, 1, 1000);
   CreateCmdDef("StopLmdFile");

   CreatePar("TransmitInfo","info").SetSynchron(true, 2., false);

   if (flushtime > 0.)
      CreateTimer("FlushTimer", flushtime, false);

   fCurrentEventNumber = 0;
   isCurrentEventNumber = false;

   fIgnoreEvent = 0;
   isIgnoreEvent = false;

   fEvCounter = 0;
   fFlushCnt = 2;

   PublishPars("$CONTEXT$/HadaqTransmitter");
}


bool hadaq::MbsTransmitterModule::retransmit()
{
   // method performs reads at maximum one event from input and
   // push it with MBS header to output
   // Method will be called from ProcessRecv/ProcessSend methods,
   // means as long as any buffer in the recv queue exists or
   // any new buffer to send queue can be provided

//   DOUT0("MbsTransmitterModule::retransmit() starts");

   // bool do_debug = CanRecv() && CanSend();

   // if (do_debug) DOUT0("Enter retransmit send to all %s", DBOOL(CanSendToAllOutputs()));

   // nothing to do
   if (!CanSendToAllOutputs()) return false;

   if (!feofbuf.null()) {

      // DOUT1("MbsTransmitterModule sends EOF to all outputs is any outdata %s canrecv %s", DBOOL(fTgtIter.IsAnyData()), DBOOL(CanRecv()));

      SendToAllOutputs(feofbuf);

//      DOUT0("MbsTransmitterModule::retransmit() exit with false 77");

      return false;
   }

   // is source do not have pointer, reset it to read new buffer
   if (!fSrcIter.IsData()) {

      if (!CanRecv()) {
//         DOUT0("MbsTransmitterModule::retransmit() exit with false 86");
         return false;
      }

      dabc::Buffer buf = Recv();

      // DOUT0("+++++++++++++++ RECV %u  ++++++++++++++", buf.GetTotalSize());


      if (buf.GetTypeId() == dabc::mbt_EOF) {

         // DOUT0("+++++++++++++++ GET EOF ++++++++++++++");

         feofbuf << buf;

         if (fTgtIter.IsEventStarted()) {
            fTgtIter.FinishSubEvent();
            fTgtIter.FinishEvent();
            fCurrentEventNumber = 0;
            isCurrentEventNumber = false;
         }

         if (fTgtIter.IsAnyData()) {

            dabc::Buffer sendbuf = fTgtIter.Close();

            SendToAllOutputs(sendbuf);

            return true;
         }

         SendToAllOutputs(feofbuf);

//         DOUT0("MbsTransmitterModule::retransmit() exit with false 116");

         return false;
      }

      // DOUT0("Recv   %u totalsize %u", (unsigned) buf.SegmentId(0), (unsigned) buf.GetTotalSize() );

      fSrcIter.Reset(buf);
      // locate to the first event
      fSrcIter.NextEvent();

//      DOUT0("MbsTransmitterModule::retransmit() done while starts with new buffer");

      return true; // we could try next buffer as soon as possible
   }

   // ignore all events with selected id
   if (isIgnoreEvent) {
      if (fIgnoreEvent == fSrcIter.evnt()->GetSeqNr()) {
         fSrcIter.NextEvent();
//         DOUT0("Ignore event %u", (unsigned) fIgnoreEvent);
         return true;
      }
      isIgnoreEvent = false;
      fIgnoreEvent = 0;
   }


   if (!fTgtIter.IsBuffer()) {
       dabc::Buffer buf = TakeBuffer();
       if (buf.null()) {
//          DOUT0("MbsTransmitterModule::retransmit() exit with false 147");
          return false;
       }

       fTgtIter.Reset(buf);
    }

   // close current event if it not will be merged with next source event
   if (fTgtIter.IsEventStarted()) {
      if (!fMergeSyncedEvents || (isCurrentEventNumber && (fCurrentEventNumber != fSrcIter.evnt()->GetSeqNr()))) {
         // first close existing events
         fTgtIter.FinishSubEvent();
         fTgtIter.FinishEvent();
         fCurrentEventNumber = 0;
         isCurrentEventNumber = false;
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
   // if it is not enough space - try to flush
   if (!has_required_place) doflush = true;
   // if nothing is for output, but we could provide it, postpone flush
   if (doflush && has_required_place && !fTgtIter.IsAnyEvent()) doflush = false;

   // if (do_debug) DOUT0("Deciding has_place %s for evnt %d doflush %s", DBOOL(has_required_place), evlen, DBOOL(doflush));

   if (doflush) {

      // this is completely empty buffer, we should ignore event when it does not fit into empty buffer
      if (!has_required_place && !fTgtIter.IsAnyData()) {
         fIgnoreEvent = fSrcIter.evnt()->GetSeqNr();
         isIgnoreEvent = true;
         // if (do_debug) DOUT0("MbsTransmitterModule::retransmit() wants to ignore %u", (unsigned) fIgnoreEvent);
         return true;
      }

      // we want to flush, but there is no data
      if (!fTgtIter.IsAnyEvent()) {
         // no sense to make flush when no any data
         // if (do_debug) DOUT0("MbsTransmitterModule::retransmit() exit with false 188");
         return false;
      }

      dabc::Buffer sendbuf;

      if (fTgtIter.IsAnyUncompleteData()) {
         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) {
            // if (do_debug) DOUT0("MbsTransmitterModule::retransmit() exit with false 197");
            return false;
         }

         sendbuf = fTgtIter.CloseAndTransfer(buf);
      } else {
         sendbuf = fTgtIter.Close();
      }

      fFlushCnt = 2; // two timout without sending should happen before flush will appear

      Par("TransmitData").SetValue(sendbuf.GetTotalSize() / 1024./1024.);
      Par("TransmitBufs").SetValue(1.);
      Par("TransmitEvents").SetValue(fEvCounter);
      fEvCounter = 0;

      // if (do_debug) DOUT0("MbsTransmitterModule::retransmit() dosend");

      // this is normal place where packed data are send
      SendToAllOutputs(sendbuf);
      return true;
   }


   if (!fTgtIter.IsEventStarted()) {
      fCurrentEventNumber = fSrcIter.evnt()->GetSeqNr();
      isCurrentEventNumber = true;

      // create proper MBS event header and

      fTgtIter.NewEvent(fCurrentEventNumber);
      fTgtIter.NewSubevent2(fSubeventId);
   }

   fTgtIter.AddRawData(fSrcIter.evnt(), evlen);

   fSrcIter.NextEvent();

   // close current event if next portion will have another id
   if (fSrcIter.IsData()) {
      if (!fMergeSyncedEvents || (isCurrentEventNumber && (fCurrentEventNumber != fSrcIter.evnt()->GetSeqNr()))) {
         // first close existing events
         fTgtIter.FinishSubEvent();
         fTgtIter.FinishEvent();
         fCurrentEventNumber = 0;
         isCurrentEventNumber = false;
         fEvCounter++;
      }
   }

//   DOUT0("MbsTransmitterModule::retransmit() done and want build next event");

   return true;
}


void hadaq::MbsTransmitterModule::ProcessTimerEvent(unsigned timer)
{
   if (fFlushCnt>0) fFlushCnt--;

   // DOUT0("hadaq::MbsTransmitterModule cansend %s canrecv %s", DBOOL(CanSend()), DBOOL(CanRecv()));

   // restart event loop if it was stopped
   ActivateOutput();
}

int hadaq::MbsTransmitterModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("StartLmdFile")) {

      std::string fname = cmd.GetStr("filename");
      int maxsize = cmd.GetInt(dabc::xml_maxsize, 30);

      std::string url = dabc::format("lmd://%s?%s=%d", fname.c_str(), dabc::xml_maxsize, maxsize);

      // we guarantee, that at least two ports will be created
      EnsurePorts(0, 2);

      bool res = dabc::mgr.CreateTransport(OutputName(1, true), url);

      DOUT0("Start LMD file %s res = %s", fname.c_str(), DBOOL(res));

      return cmd_bool(res);
   } else
   if (cmd.IsName("StopLmdFile")) {

      bool res = true;

      if (NumOutputs()>1)
         res = DisconnectPort(OutputName(1));

      DOUT0("Stop LMD file res = %s", DBOOL(res));

      return cmd_bool(res);
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

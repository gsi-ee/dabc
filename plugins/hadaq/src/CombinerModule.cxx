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

#include "hadaq/CombinerModule.h"

#include <math.h>
#include <iostream>

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Observer.h"
#include "mbs/MbsTypeDefs.h"

// this define will just receive all buffers, note their size and discard them, no event building
//#define HADAQ_COMBINER_TESTRECEIVE 1

hadaq::CombinerModule::CombinerModule(const char* name, dabc::Command cmd) :
      dabc::ModuleAsync(name, cmd),
      fBufferSize(0),
      fInp(),
      fOut(),
      fFlushFlag(false),
      fUpdateCountersFlag(false),
      fFileOutput(false),
      fServOutput(false),
      fWithObserver(false),
      fBuildCompleteEvents(true)
{
   fTotalRecvBytes = 0;
   fTotalRecvEvents = 0;
   fTotalDiscEvents = 0;
   fTotalDroppedEvents = 0;
   fTotalTagErrors = 0;
   fTotalDataErrors = 0;

   for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn)
      fErrorbitPattern[ptrn] = 0;

   for (unsigned i = 0; i < HADAQ_NEVTIDS; i++)
      fEventIdCount[i] = 0;

   fRunNumber = hadaq::Event::CreateRunId(); // runid from configuration time.

   fBufferSize = Cfg(dabc::xmlBufferSize, cmd).AsInt(16384);

   fTriggerNrTolerance = 50000;
   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStdStr(dabc::xmlWorkPool);

   CreatePoolHandle(poolname.c_str());

   fNumInputs = Cfg(dabc::xmlNumInputs, cmd).AsInt(1);

   fDoOutput = Cfg(mbs::xmlNormalOutput,cmd).AsBool(false);
   fFileOutput = Cfg(mbs::xmlFileOutput, cmd).AsBool(false);
   fServOutput = Cfg(mbs::xmlServerOutput, cmd).AsBool(false);
   fWithObserver = Cfg(hadaq::xmlObserverEnabled, cmd).AsBool(false);

   fUseSyncSeqNumber= Cfg(hadaq::xmlSyncSeqNumberEnabled, cmd).AsBool(true); // if true, use vulom/roc syncnumber for event sequence number
   fSyncSubeventId=   Cfg(hadaq::xmlSyncSubeventId, cmd).AsInt(0x8000);//0x8000; // TODO: configuration from xml
   if (fUseSyncSeqNumber)
      DOUT0(("HADAQ combiner module with VULOM sync event sequence number from cts subevent id 0x%0x",fSyncSubeventId));
   else
      DOUT0(("HADAQ combiner module with independent event sequence number"));


//     fBuildCompleteEvents = Cfg(mbs::xmlCombineCompleteOnly,cmd).AsBool(true);
//
//
//     fEventIdTolerance = Cfg(mbs::xmlEvidTolerance,cmd).AsInt(0);
//
//     std::string ratesprefix = Cfg(mbs::xmlCombinerRatesPrefix, cmd).AsStdStr("Hadaq");
   std::string ratesprefix = "Hadaq";

   double flushtmout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);

   for (int n = 0; n < fNumInputs; n++) {
      CreateInput(FORMAT((mbs::portInputFmt, n)), Pool(), 10);

      //      DOUT0(("  Port%u: Capacity %u", n, Input(n)->InputQueueCapacity()));

      fInp.push_back(ReadIterator());
      fCfg.push_back(InputCfg());
      fInp[n].Close();
      fCfg[n].Reset();
   }

//     fNumObligatoryInputs = NumInputs();
//
   if (fDoOutput)
      CreateOutput(mbs::portOutput, Pool(), 5);
   if (fFileOutput)
      CreateOutput(mbs::portFileOutput, Pool(), 5);
   if (fServOutput)
      CreateOutput(mbs::portServerOutput, Pool(), 5);

   if (flushtmout > 0.)
      CreateTimer("FlushTimer", flushtmout, false);

//   CreatePar("RunId");
//   Par("RunId").SetInt(fRunNumber); // to communicate with file components


   fEventRateName = ratesprefix + "Events";
   fEventDiscardedRateName = ratesprefix + "DiscardedEvents";
   fEventDroppedRateName = ratesprefix + "DroppedEvents";
   fDataRateName = ratesprefix + "Data";
   fInfoName = ratesprefix + "Info";

   CreatePar(fDataRateName).SetRatemeter(false, 5.).SetUnits("MB");
   CreatePar(fEventRateName).SetRatemeter(false, 5.).SetUnits("Ev");
   CreatePar(fEventDiscardedRateName).SetRatemeter(false, 5.).SetUnits("Ev");
   CreatePar(fEventDroppedRateName).SetRatemeter(false, 5.).SetUnits("Ev");
   // must be configured in xml file
   //   fDataRate->SetDebugOutput(true);

//     CreateCmdDef(mbs::comStartFile).AddArg(mbs::xmlFileName, "string", true).AddArg(mbs::xmlSizeLimit, "int", false, "1000");
//
//     CreateCmdDef(mbs::comStopFile);
//
//     CreateCmdDef(mbs::comStartServer).AddArg(mbs::xmlServerKind, "string", true, mbs::ServerKindToStr(mbs::StreamServer));
//
//     CreateCmdDef(mbs::comStopServer);
//
   // SL FIXME - this is only workaround, we keep pool reference to release it after all buffers
   fPool = dabc::mgr.FindItem("Pool");

   CreatePar(fInfoName, "info").SetSynchron(true, 2., false);
   SetInfo(
         dabc::format(
               "HADAQ combiner module ready. Runid:%d, fileout:%d, servout:%d flush:%3.1f",
               fRunNumber, fFileOutput, fServOutput, flushtmout), true);
   DOUT0(("HADAQ combiner module ready. Runid:%d, fileout:%d, servout:%d flush:%3.1f",fRunNumber, fFileOutput, fServOutput, flushtmout));
   Par(fDataRateName).SetDebugLevel(1);
   Par(fEventRateName).SetDebugLevel(1);
   Par(fEventDiscardedRateName).SetDebugLevel(1);
   Par(fEventDroppedRateName).SetDebugLevel(1);
   RegisterExportedCounters();

   fNumCompletedBuffers = 0;
}

hadaq::CombinerModule::~CombinerModule()
{
   fOut.Close().Release();

   fCfg.clear();

   fInp.clear();

   fPool.Release();
}

void hadaq::CombinerModule::SetInfo(const std::string& info, bool forceinfo)
{

   Par(fInfoName).SetStr(info);
   if (forceinfo)
      Par(fInfoName).FireModified();
}

void hadaq::CombinerModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (fFlushFlag)
      FlushOutputBuffer();
   fFlushFlag = true;
   if (fUpdateCountersFlag)
      UpdateExportedCounters();
   fUpdateCountersFlag = true;

   BuildSeveralEvents();
}

void hadaq::CombinerModule::ProcessInputEvent(dabc::Port* port)
{
   BuildSeveralEvents();
}

void hadaq::CombinerModule::ProcessOutputEvent(dabc::Port* port)
{
   BuildSeveralEvents();
}


void hadaq::CombinerModule::BuildSeveralEvents()
{
   // SL - FIXME: workaround, process maximum 3 events a time,
   //  otherwise combiner can accumulate too many events in the queues
   //  From other side, 3 can be too few - one could have 100 events in the buffer and
   // we get one event per buffer from DABC. First try was wrong

   int cnt(1000);

   fNumCompletedBuffers = 0;

   while (BuildEvent() && (cnt-->0)) {
      // WORKAROUND: break long loop when at least two buffer were taken,
      // but also at least buffer per input
      // TODO: should be done absolutely different - via new queue and state changes
      if ((fNumCompletedBuffers>=2) && (fNumCompletedBuffers>=fNumInputs)) break;
   }

}


void hadaq::CombinerModule::ProcessConnectEvent(dabc::Port* port)
{
   DOUT0(("HadaqCombinerModule ProcessConnectEvent %s", port->GetName()));
}

void hadaq::CombinerModule::ProcessDisconnectEvent(dabc::Port* port)
{
   DOUT0(("HadaqCombinerModule ProcessDisconnectEvent %s", port->GetName()));
}

void hadaq::CombinerModule::BeforeModuleStart()
{
   DOUT0(("hadaq::CombinerModule::BeforeModuleStart name: %s ", GetName()));
}

void hadaq::CombinerModule::AfterModuleStop()
{
   DOUT0(("%s: Complete Events:%d , BrokenEvents:%d, DroppedEvents:%d, RecvBytes:%d, data errors:%d, tag errors:%d",
           GetName(), (int) fTotalRecvEvents, (int) fTotalDiscEvents , (int) fTotalDroppedEvents, (int) fTotalRecvBytes ,(int) fTotalDataErrors ,(int) fTotalTagErrors));

//   std::cout << "----- Combiner Module Statistics: -----" << std::endl;
//   std::cout << "Complete Events:" << fTotalRecvEvents << ", BrokenEvents:"
//         << fTotalDiscEvents << ", DroppedEvents:" << fTotalDroppedEvents
//         << ", RecvBytes:" << fTotalRecvBytes << ", data errors:"
//         << fTotalDataErrors << ", tag errors:" << fTotalTagErrors << std::endl;

}

///////////////////////////////////////////////////////////////
//////// OUTPUT BUFFER METHODS:

bool hadaq::CombinerModule::EnsureOutputBuffer(uint32_t payload)
{
   // check if we have enough space in current buffer
   if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(payload)) {
      // no, we close current buffer
      if (!FlushOutputBuffer()) {
         DOUT3(("CombinerModule::EnsureOutputBuffer - could not flush buffer"));
         return false;
      }
   }
   // after flushing last buffer, take next one:
   if (!fOut.IsBuffer()) {
      dabc::Buffer buf = Pool()->TakeBufferReq(fBufferSize);
      if (buf.null()) {
         DOUT3(("CombinerModule::EnsureOutputBuffer - could not take new buffer"));
         return false;
      }
      if (!fOut.Reset(buf)) {
         SetInfo("Cannot use buffer for output - hard error!!!!", true);
         buf.Release();
         dabc::mgr.StopApplication();
         return false;
      }
   }
   // now check working buffer for space:
   if (!fOut.IsPlaceForEvent(payload)) {
      DOUT0(("CombinerModule::EnsureOutputBuffer - new buffer has not enough space!"));
      return false;
   }

   return true;
}

bool hadaq::CombinerModule::FlushOutputBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) {
      DOUT3(("FlushOutputBuffer has no buffer to flush"));
      return false;
   }
   if (!CanSendToAllOutputs()) {
      DOUT3(("FlushOutputBuffer can't send to all outputs"));
      return false;
   }
   dabc::Buffer buf = fOut.Close();
   DOUT3(("FlushOutputBuffer() Send buffer of size = %d", buf.GetTotalSize()));
   SendToAllOutputs(buf.HandOver());
   fFlushFlag = false; // indicate that next flush timeout one not need to send buffer

   return true;
}

void hadaq::CombinerModule::RegisterExportedCounters()
{
   if(!fWithObserver) return;
   DOUT0(("##### CombinerModule::RegisterExportedCounters for shmem"));
   CreateEvtbuildPar("prefix");
   dabc::Port* fileoutput = FindPort(mbs::portFileOutput);
   if(fileoutput)
     {
        std::string filename=fileoutput->Cfg(hadaq::xmlFileName).AsStdStr();
        //std::cout <<"!! Has filename:"<<filename << std::endl;
        // strip leading path if any:
        size_t pos = filename.rfind("/");
        if (pos!=std::string::npos)
          {
             filename = filename.substr(pos+1, std::string::npos);
          }
        //std::cout <<"!!!!!! Register prefix:"<<filename << std::endl;
        SetEvtbuildPar("prefix",hadaq::Observer::Args_prefixCode(filename.c_str()));
       }


   CreateEvtbuildPar("evtsDiscarded");
   CreateEvtbuildPar("evtsComplete");
   CreateEvtbuildPar("evtsDataError");
   CreateEvtbuildPar("evtsTagError");
   CreateEvtbuildPar("bytesWritten");
   CreateEvtbuildPar("runId");
   CreateEvtbuildPar("nrOfMsgs");
   CreateNetmemPar("nrOfMsgs");


   for (int i = 0; i < fNumInputs; i++)
      {
         CreateEvtbuildPar(dabc::format("trigNr%d",i));
         CreateEvtbuildPar(dabc::format("errBit%d",i));
         CreateEvtbuildPar(dabc::format("evtbuildBuff%d", i));
         for (int p = 0; p < HADAQ_NUMERRPATTS; p++)
         {
            CreateEvtbuildPar(dabc::format("errBitStat%d_%d",p,i));
         }
      }

   for (unsigned i = 0; i < HADAQ_NEVTIDS; i++) {
      CreateEvtbuildPar(dabc::format("evtId%d",i));
   }


   for (int p = 0; p < HADAQ_NUMERRPATTS; p++) {
        CreateEvtbuildPar(dabc::format("errBitPtrn%d",p));
   }
}

void hadaq::CombinerModule::ClearExportedCounters()
{
     // TODO: we need mechanism that calls this method whenever a new run begins
     // problem: run id is decided by hld file output, need command communication
     fTotalRecvBytes = 0;
     fTotalRecvEvents = 0;
     fTotalDiscEvents = 0;
     fTotalDroppedEvents = 0;
     fTotalTagErrors = 0;
     fTotalDataErrors = 0;
     UpdateExportedCounters();
}


bool hadaq::CombinerModule::UpdateExportedCounters()
{
   if(!fWithObserver) return false;
   fUpdateCountersFlag = false; // do not call this from timer again while it is processed
   SetEvtbuildPar("nrOfMsgs",fNumInputs);
   SetNetmemPar("nrOfMsgs",fNumInputs);
   SetEvtbuildPar("evtsDiscarded",fTotalDiscEvents+fTotalDroppedEvents);
   SetEvtbuildPar("evtsComplete",fTotalRecvEvents);
   SetEvtbuildPar("evtsDataError",fTotalDataErrors);
   SetEvtbuildPar("evtsTagError", fTotalTagErrors);

   // written bytes is updated by hldoutput directly, required by epics control of file size limit
   //SetEvtbuildPar("bytesWritten", fTotalRecvBytes);


   // runid is generated by epics, we do not update it here!
   //SetEvtbuildPar("runId",fRunNumber);
   //SetEvtbuildPar("runId",Par("RunId").AsUInt()); // sync local parameter to observer export
   //std::cout <<"!!!!!! Update run id:"<<Par("RunId").AsUInt() << std::endl;
   for (int i = 0; i < fNumInputs; i++)
   {

      //unsigned trignr= (fCfg[i].fTrigNr << 8) |  (fCfg[i].fTrigTag & 0xff);
      SetEvtbuildPar(dabc::format("trigNr%d",i),fCfg[i].fLastEvtBuildTrigId); // prerecorded id of last complete event
      SetEvtbuildPar(dabc::format("errBit%d",i),fCfg[i].fErrorBits);
      // note: if we get those values here, we are most likely in between event building. use prerecorded values instead
      //              dabc::Port* input=Input(i);
      //              if(input)
      //                 {
      //                    float ratio=0;
      //                    if(input->InputQueueSize()>0) ratio = input->InputPending() / input->InputQueueSize();
      //                    unsigned fillevel= 100*ratio;
      //                    SetEvtbuildPar(dabc::format("evtbuildBuff%d", i), fillevel);
      //                 }

      SetEvtbuildPar(dabc::format("evtbuildBuff%d", i), 100 * fCfg[i].fQueueLevel);

      for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn){
         SetEvtbuildPar(dabc::format("errBitStat%d_%d",ptrn,i),fCfg[i].fErrorbitStats[ptrn]);
      }
   }

   for (int p = 0; p < HADAQ_NUMERRPATTS;++p)
      SetEvtbuildPar(dabc::format("errBitPtrn%d",p), fErrorbitPattern[p]);

   for (unsigned i = 0; i < HADAQ_NEVTIDS; i++)
      SetEvtbuildPar(dabc::format("evtId%d",i),fEventIdCount[i]);

   return true;
}




///////////////////////////////////////////////////////////////
//////// INPUT BUFFER METHODS:

bool hadaq::CombinerModule::SkipInputBuffer(unsigned ninp)
{
   fNumCompletedBuffers++;

   DOUT5(("CombinerModule::SkipInputBuffer  %d ", ninp));
   return (Input(ninp)->SkipInputBuffers(1));
}


bool hadaq::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   DOUT5(("CombinerModule::ShiftToNextBuffer %d ", ninp));
   fInp[ninp].Close();

//   if(!SkipInputBuffer(ninp))
//               return false; // discard previous first buffer if we can receive a new one
   if(!Input(ninp)->CanRecv()) return false;

   dabc::Buffer buf = Input(ninp)->Recv();

   fNumCompletedBuffers++;

   if(!fInp[ninp].Reset(buf.HandOver())) {
     // use new first buffer of input for work iterator, but leave it in input queue
               DOUT5(("CombinerModule::ShiftToNextBuffer %d could not reset to FirstInputBuffer", ninp));
               // skip buffer and try again
               //SkipInputBuffer(ninp);
               return false;
            }
   ;
   return true;
}

bool hadaq::CombinerModule::ShiftToNextHadTu(unsigned ninp)
{
   DOUT5(("CombinerModule::ShiftToNextHadTu %d begins", ninp));
   bool foundhadtu(false);
   //static unsigned ccount=0;
   while (!foundhadtu) {
         if (!fInp[ninp].IsData()) {
            if (!ShiftToNextBuffer(ninp))
            return false;
      }
     bool res = fInp[ninp].NextHadTu();
      if (!res || (fInp[ninp].hadtu() == 0)) {
         DOUT5(("CombinerModule::ShiftToNextHadTu %d has zero NextHadTu()", ninp));

         if(!ShiftToNextBuffer(ninp)) return false;
         //return false;
         DOUT5(("CombinerModule::ShiftToNextHadTu in continue %d", (int) ccount++));
         continue;
      }
      foundhadtu = true;
   } //  while (!foundhadtu)
   return true;

}

bool hadaq::CombinerModule::ShiftToNextSubEvent(unsigned ninp)
{
   DOUT5(("CombinerModule::ShiftToNextSubEvent %d ", ninp));
   fCfg[ninp].Reset();
   bool foundevent(false);
   //static unsigned ccount=0;
   while (!foundevent) {
      bool res = fInp[ninp].NextSubEvent();
      if (!res || (fInp[ninp].subevnt() == 0)) {
         DOUT5(("CombinerModule::ShiftToNextSubEvent %d with zero NextSubEvent()", ninp));
         // retry in next hadtu container
         if (!ShiftToNextHadTu(ninp))
            return false; // no more input buffers available
         DOUT5(("CombinerModule::ShiftToNextSubEvent in continue %d", (int) ccount++));
         continue;
      }

      foundevent = true;

      fCfg[ninp].fTrigNr = fInp[ninp].subevnt()->GetTrigNr() >> 8;
      fCfg[ninp].fTrigTag = fInp[ninp].subevnt()->GetTrigNr() & 0xFF;
      if (fInp[ninp].subevnt()->GetSize() > sizeof(hadaq::Subevent)) {
         fCfg[ninp].fEmpty = false;
      }
      fCfg[ninp].fDataError = fInp[ninp].subevnt()->GetDataError();

      fCfg[ninp].fSubId = fInp[ninp].subevnt()->GetId() & 0x7fffffffUL ;

      /* evaluate trigger type as in production eventbuilders here:*/
      unsigned wordNr=2;
      uint32_t bitmask=0xf000000;
      uint32_t bitshift = 24;
      // above from args.c defaults
      uint32_t val=fInp[ninp].subevnt()->Data(wordNr-1);
      fCfg[ninp].fTrigType = (val & bitmask) >> bitshift;

      fCfg[ninp].fErrorBits = fInp[ninp].subevnt()->GetErrBits();



   }

   return true;
}

bool hadaq::CombinerModule::DropAllInputBuffers()
{
   DOUT0(("hadaq::CombinerModule::DropAllInputBuffers()..."));
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      fInp[ninp].Close();
      while (SkipInputBuffer(ninp))
         ; // drop input port queue buffers until no more there
   }
   return true;
}

bool hadaq::CombinerModule::BuildEvent()
{
   // RETURN VALUE: true - event is successfully build, recall immediately
   //               false - leave event loop for framework (other modules input is required!)

   // eventbuilding on hadtu streams here:

   // this is daq_evtbuild logic:
   // first check eventnumber of master channel
   // here loop over all channels: skip subevts with too old eventnumbers
   // if event is not complete, discard this and try next master channel index

#ifdef HADAQ_COMBINER_TESTRECEIVE
   // Testing: just discard all input buffers immediately:
   for (unsigned ninp=0; ninp<fCfg.size(); ninp++) {
      if (!Input(ninp)->CanRecv()) continue;
      dabc::Buffer buf =Input(ninp)->FirstInputBuffer();
      size_t bufferlen=0;
      if(!buf.null()) bufferlen=buf.GetTotalSize();
      fTotalRecvBytes+=bufferlen;
      Par(fDataRateName).SetDouble( bufferlen /1024 ./1024.);
      SkipInputBuffer(ninp);
   }
   return false; // always leave process event function immediately
#endif

    // adjust run number that might have changed by file output
   //fRunNumber=GetEvtbuildParValue("runId"); // PERFORMANCE?
   // note: file outout will overwrite this number in event header to be consistent with file name
   // for online monitor, we could live with different run numbers

   ///////////////////////////////////////////////////////////
   // alternative approach like a simplified mbs event building:
   //////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////////////
   // first input loop: find out maximum trignum of all inputs = current event trignumber
   //static unsigned ccount=0;


   int num_valid = 0;
   unsigned masterchannel = 0;
   uint32_t subeventssize = 0;
   hadaq::EventNumType mineventid(0), maxeventid(0);
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      if (fInp[ninp].subevnt() == 0)
         if (!ShiftToNextSubEvent(ninp)) {
            return false; // could not get subevent data on any channel. let framework do something before next try
         }

      hadaq::EventNumType evid = fCfg[ninp].fTrigNr;
      if (num_valid == 0) {
         mineventid = evid;
         maxeventid = evid;
         masterchannel = ninp;
      } else {
         if (evid < mineventid) {
            mineventid = evid;
         } else if (evid > maxeventid) {
            maxeventid = evid;
            masterchannel = ninp;
         }
      }
      num_valid++;
   } // for ninp

   if (fCfg[masterchannel].fTrigNr != maxeventid) {
      EOUT(("Combiner Module NEVER COME HERE: buildevid %u inconsistent with id %u at master channel %u, ", maxeventid, fCfg[masterchannel].fTrigNr, masterchannel));
      dabc::mgr.StopApplication();
      return false; // to leave event processing loops
   }

   //if (num_valid==0) return false;
   // we always build event with maximum trigger id = newest event, discard incomplete older events
   hadaq::EventNumType buildevid = maxeventid;
   hadaq::EventNumType buildtag = fCfg[masterchannel].fTrigTag;
   hadaq::EventNumType diff = maxeventid - mineventid;

   ///////////////////////////////////////////////////////////////////////////////
   // check too large triggertag difference on input channels, flush input buffers
   if ((fTriggerNrTolerance > 0) && (diff > fTriggerNrTolerance)) {
      SetInfo(
            dabc::format(
                  "Event id difference %u exceeding tolerance window %u, flushing buffers!",
                  diff, fTriggerNrTolerance), true);
      DOUT0(("Event id difference %u exceeding tolerance window %u, maxid:%u minid:%u, flushing buffers!",
                  diff, fTriggerNrTolerance, maxeventid, mineventid));
      DropAllInputBuffers();
      return false; // retry on next set of buffers
   }

   ////////////////////////////////////////////////////////////////////////
   // second input loop: skip all subevents until we reach current trignum
   // select inputs which will be used for building
   //bool eventIsBroken=false;
   bool dataError = false;
   bool tagError = false;
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      bool foundsubevent = false;
      while (!foundsubevent) {

         hadaq::EventNumType trignr = fCfg[ninp].fTrigNr;
         hadaq::EventNumType trigtag = fCfg[ninp].fTrigTag;
         bool isempty = fCfg[ninp].fEmpty;
         bool haserror = fCfg[ninp].fDataError;
         DoErrorBitStatistics(ninp); // also for not complete events
         if (trignr == buildevid) {

            if (!isempty) {
               // check also trigtag:
               if (trigtag != buildtag) {
                  tagError = true;
               }
               if (haserror) {
                  dataError = true;
               }
               subeventssize += fInp[ninp].subevnt()->GetPaddedSize();
            }
            foundsubevent = true;
            break;

         } else if (trignr < buildevid) {

            if(!ShiftToNextSubEvent(ninp)) return false;
            // try with next subevt until reaching buildevid

            // TODO: account dropped subevents to statistics
            DOUT5(("CombinerModule::BuildEvent ch:%d trignr:%d buildevid:%d in continue %d",
                  (int) ninp, (int) trignr, (int) buildevid, (int)ccount++));
            continue;
         } else {
            // can happen when the subevent of the buildevid is missing on this channel
            // we account broken event and let call BuildEvent again, rescan buildevid without shifting other subevts.
            Par(fEventDiscardedRateName).SetInt(1);
            fTotalDiscEvents++;
            return false; // we give the framework some time to do other things though
         }

      } // while foundsubevent
   } // for ninpt

   // here all inputs should be aligned to buildevid

   // for sync sequence number, check first if we have error from cts:
   hadaq::EventNumType sequencenumber=fTotalRecvEvents;
   bool hascorrectsync=false;
   if(fUseSyncSeqNumber)
         {
      // we may put sync id from subevent payload to event sequence number already here.
         hadaq::Subevent* syncsub= fInp[0].subevnt(); // for the moment, sync number must be in first udp input
                            // TODO: put this to configuration

         if (syncsub->GetId() != fSyncSubeventId) {
            // main subevent has same id as cts/hub subsubevent
            DOUT1(("***  --- sync subevent at input 0x%x has wrong id 0x%x !!! Check configuration.", 0, syncsub->GetId()));
         }

         else {
            unsigned syncdata=0, syncnum=0;
            unsigned datasize = syncsub->GetNrOfDataWords();
            unsigned ix = 0;
            while (ix < datasize) {
               //scan through trb3 data words and look for the cts subsubevent
               unsigned data = syncsub->Data(ix);
               //! Central hub header and inside
               if ((data & 0xFFFF) == (unsigned) fSyncSubeventId) {
                  unsigned centHubLen = ((data >> 16) & 0xFFFF);
                  DOUT5(("***  --- central hub header: 0x%x, size=%d", data, centHubLen));
                  syncdata = syncsub->Data(ix + centHubLen);
                  syncnum = (syncdata & 0xFFFFFF);
                  DOUT1(("***  --- found sync data: 0x%x, sync number is %d", syncdata, syncnum));
                  //fOut.evnt()->SetSeqNr(syncnum);

                  break;
               }
               ++ix;
            }
            if(syncnum==0)
               {
                  DOUT1(("***  --- Found zero sync number!, full sync data:0x%x",syncdata));
               }
            else if ( (syncdata >> 31) & 0x1 == 0x1)
               {
                  DOUT1(("***  --- Found error bit at sync number: 0x%x, full sync data:0x%x", syncnum, syncdata));
               }
            else
               {
                  sequencenumber=syncnum;
                  hascorrectsync=true;
               }


         } // if (syncsub->GetId() != fSyncSubeventId)

         } // if(fUseSyncSeqNumber)
   else
      {
            hascorrectsync=true;
      }



   // ensure that we have output buffer that is big enough:
   if (hascorrectsync && EnsureOutputBuffer(subeventssize)) {
      // EVENT BUILDING IS HERE
      fOut.NewEvent(sequencenumber, fRunNumber); // like in hadaq, event sequence number is independent of trigger.
      fTotalRecvEvents++;

      fOut.evnt()->SetDataError((dataError || tagError));
      if (dataError)
         fTotalDataErrors++;
      if (tagError)
         fTotalTagErrors++;

      // here event id, always from "cts master channel" 0
      unsigned currentid=fCfg[0].fTrigType | (3 << 12); // DAQVERSION=3 for dabc
      fEventIdCount[currentid & (HADAQ_NEVTIDS - 1)]++;
      fOut.evnt()->SetId(currentid & (HADAQ_NEVTIDS_IN_FILE - 1));


//      if(fUseSyncSeqNumber)
//         {
//      // we may put sync id from subevent payload to event sequence number already here.
//         hadaq::Subevent* syncsub= fInp[0].subevnt(); // for the moment, sync number must be in first udp input
//                            // TODO: put this to configuration
//
//         // look into subevent for subsubevent with id of cts payload
////         char* cursor =  (char*) syncsub;
////         char* endofdata= cursor+ syncsub->GetSize();
////         cursor+=sizeof(hadaq::Subevent); // move to begin of subsubevent data
////
//         if (syncsub->GetId() != fSyncSubeventId) {
//            // main subevent has same id as cts/hub subsubevent
//            DOUT1(("***  --- sync subevent at input 0x%x has wrong id 0x%x !!! Check configuration.\n", 0, syncsub->GetId()));
//         }
//
//         else {
//            unsigned datasize = syncsub->GetNrOfDataWords();
//            unsigned ix = 0;
//            while (ix < datasize) {
//               //scan through trb3 data words and look for the cts subsubevent
//               unsigned data = syncsub->Data(ix);
//               //! Central hub header and inside
//               if ((data & 0xFFFF) == (unsigned) fSyncSubeventId) {
//                  unsigned centHubLen = ((data >> 16) & 0xFFFF);
//                  DOUT5(("***  --- central hub header: 0x%x, size=%d\n", data, centHubLen));
//                  unsigned syncdata = syncsub->Data(ix + centHubLen);
//                  unsigned syncnum = (syncdata & 0xFFFFFF);
//                  DOUT5(("***  --- found sync data: 0x%x, sync number is %d\n", syncdata, syncnum));
//                  fOut.evnt()->SetSeqNr(syncnum);
//                  break;
//               }
//               ++ix;
//            }
//
//         } // if (syncsub->GetId() != fSyncSubeventId)
//
//         } // if(fUseSyncSeqNumber)

         // third input loop: build output event from all not empty subevents
      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
         if (fCfg[ninp].fEmpty)
            continue;
         fOut.AddSubevent(fInp[ninp].subevnt());
         DoInputSnapshot(ninp); // record current state of event tag and queue level for control system
      } // for ninp



      fOut.FinishEvent();

      Par(fEventRateName).SetInt(1);
      unsigned currentbytes = subeventssize + sizeof(hadaq::Event);
      fTotalRecvBytes += currentbytes;
      Par(fDataRateName).SetDouble(currentbytes / 1024. / 1024.);

   } else {
      DOUT3(("Skip event %u of size %u : wrong sync number or no output buffer of size %u available!", buildevid, subeventssize+ sizeof(hadaq::Event), fBufferSize));
      Par(fEventDroppedRateName).SetInt(1);
      fTotalDroppedEvents++;
   } // ensure outputbuffer

   // FINAL loop: proceed to next subevents
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
      ShiftToNextSubEvent(ninp);

   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true; // event is build successfully. try next one
}

void hadaq::CombinerModule::DoErrorBitStatistics(unsigned ninp)
{
// this is translation of how it is done in hadaq evtbuild code:
   for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn) {
      if (fErrorbitPattern[ptrn] == 0) {
         fErrorbitPattern[ptrn]=fCfg[ninp].fErrorBits;
         fCfg[ninp].fErrorbitStats[ptrn]++;
         break;
      } else
      if (fErrorbitPattern[ptrn] == fCfg[ninp].fErrorBits) {
         fCfg[ninp].fErrorbitStats[ptrn]++;
         break;
      }

   }
}

void  hadaq::CombinerModule::DoInputSnapshot(unsigned ninp)
{
   // copy here input properties at the moment of event building to stats:

   dabc::Port* input=Input(ninp);
   if(input)
   {
      float ratio=0;
      if(input->InputQueueSize()>0) ratio = input->InputPending() / input->InputQueueSize();
      fCfg[ninp].fQueueLevel=ratio;
   }
   fCfg[ninp].fLastEvtBuildTrigId = (fCfg[ninp].fTrigNr << 8) |  (fCfg[ninp].fTrigTag & 0xff);
}


int hadaq::CombinerModule::ExecuteCommand(dabc::Command cmd)
{
//   if (cmd.IsName(hadaq::comStartFile)) {
//      dabc::Port* port = FindPort(hadaq::portFileOutput);
//      if (port==0) port = CreateOutput(hadaq::portFileOutput, Pool(), 5);
//
//      std::string filename = port->Cfg(hadaq::xmlFileName, cmd).AsStdStr();
//      int sizelimit = port->Cfg(hadaq::xmlSizeLimit,cmd).AsInt(1000);
//
//      SetInfo(dabc::format("Start hadaq file %s sizelimit %d mb", filename.c_str(), sizelimit), true);
//
//      dabc::CmdCreateTransport dcmd(port->ItemName(), mbs::typeLmdOutput, "MbsFileThrd");
//      dcmd.SetStr(mbs::xmlFileName, filename);
//      dcmd.SetInt(mbs::xmlSizeLimit, sizelimit);
//      return dabc::mgr.Execute(dcmd) ? dcmd.GetResult() : dabc::cmd_false;
//   } else
//   if (cmd.IsName(mbs::comStopFile)) {
//      dabc::Port* port = FindPort(mbs::portFileOutput);
//      if (port!=0) port->Disconnect();
//
//      SetInfo("Stop file", true);
//
//      return dabc::cmd_true;
//   } else
//   if (cmd.IsName(mbs::comStartServer)) {
//      dabc::Port* port = FindPort(mbs::portServerOutput);
//      if (port==0) port = CreateOutput(mbs::portServerOutput, Pool(), 5);
//
//      std::string serverkind = port->Cfg(mbs::xmlServerKind, cmd).AsStdStr();
//      int kind = StrToServerKind(serverkind.c_str());
//      if (kind==mbs::NoServer) kind = mbs::TransportServer;
//      serverkind = ServerKindToStr(kind);
//
//      dabc::CmdCreateTransport dcmd(port->ItemName(), mbs::typeServerTransport, "MbsServThrd");
//      dcmd.SetStr(mbs::xmlServerKind, serverkind);
//      dcmd.SetInt(mbs::xmlServerPort, mbs::DefualtServerPort(kind));
//      bool res = dabc::mgr.Execute(dcmd);
//
//      SetInfo(dabc::format("%s: %s mbs server %s port %d",GetName(),
//            (res ? "Start" : " Fail to start"), serverkind.c_str(), DefualtServerPort(kind)), true);
//
//      return res ? dcmd.GetResult() : dabc::cmd_false;
//   } else
//   if (cmd.IsName(mbs::comStopServer)) {
//      dabc::Port* port = FindPort(mbs::portServerOutput);
//      if (port!=0) port->Disconnect();
//
//      SetInfo("Stop server", true);
//      return dabc::cmd_true;
//   } else
//   if (cmd.IsName("ConfigureInput")) {
//      unsigned ninp = cmd.GetUInt("Port", 0);
////      DOUT0(("Start input configure %u size %u", ninp, fCfg.size()));
//      if (ninp<fCfg.size()) {
//
////         DOUT0(("Do0 input configure %u size %u", ninp, fCfg.size()));
//
//         fCfg[ninp].real_mbs = cmd.GetBool("RealMbs", fCfg[ninp].real_mbs);
//         fCfg[ninp].real_evnt_num = cmd.GetBool("RealEvntNum", fCfg[ninp].real_evnt_num);
//         fCfg[ninp].no_evnt_num = cmd.GetBool("NoEvntNum", fCfg[ninp].no_evnt_num);
//
////         DOUT0(("Do1 input configure %u size %u", ninp, fCfg.size()));
//         fCfg[ninp].evntsrc_fullid = cmd.GetUInt("EvntSrcFullId", fCfg[ninp].evntsrc_fullid);
//         fCfg[ninp].evntsrc_shift = cmd.GetUInt("EvntSrcShift", fCfg[ninp].evntsrc_shift);
//
////         DOUT0(("Do2 input configure %u size %u", ninp, fCfg.size()));
//
//         std::string ratename = cmd.GetStdStr("RateName", "");
//         if (!ratename.empty())
//            Input(ninp)->SetInpRateMeter(CreatePar(ratename).SetRatemeter(false,1.));
//
////         DOUT0(("Do3 input configure %u size %u", ninp, fCfg.size()));
//
//         if (fCfg[ninp].no_evnt_num) {
//            fCfg[ninp].real_mbs = false;
//            if (fNumObligatoryInputs>1) fNumObligatoryInputs--;
//         }
//
//         DOUT1(("Configure input%u of module %s: RealMbs:%s RealEvntNum:%s EvntSrcFullId: 0x%x EvntSrcShift: %u",
//               ninp, GetName(),
//               DBOOL(fCfg[ninp].real_mbs), DBOOL(fCfg[ninp].real_evnt_num),
//               fCfg[ninp].evntsrc_fullid, fCfg[ninp].evntsrc_shift));
//
////         DOUT0(("Do4 input configure %u size %u", ninp, fCfg.size()));
//
//      }
//
//      return dabc::cmd_true;
//   }
//

   return dabc::ModuleAsync::ExecuteCommand(cmd);

}


std::string  hadaq::CombinerModule::GetEvtbuildParName(const std::string& name)
{
   return dabc::format("%s_%s",hadaq::EvtbuildPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateEvtbuildPar(const std::string& name)
{
   CreatePar(GetEvtbuildParName(name));
}

void hadaq::CombinerModule::SetEvtbuildPar(const std::string& name, unsigned value)
{
    Par(GetEvtbuildParName(name)).SetUInt(value);
}

unsigned hadaq::CombinerModule::GetEvtbuildParValue(const std::string& name)
{
   return Par(GetEvtbuildParName(name)).AsUInt();
}


std::string  hadaq::CombinerModule::GetNetmemParName(const std::string& name)
{
   return dabc::format("%s_%s",hadaq::NetmemPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateNetmemPar(const std::string& name)
{
   CreatePar(GetNetmemParName(name));
}

void hadaq::CombinerModule::SetNetmemPar(const std::string& name,
      unsigned value)
{
   Par(GetNetmemParName(name)).SetUInt(value);
}








extern "C" void InitHadaqEvtbuilder()
{
   if (dabc::mgr.null()) {
      EOUT(("Manager is not created"));
      exit(1);
   }

   DOUT0(("Create Hadaq combiner module"));

   //dabc::mgr.CreateMemoryPool(dabc::xmlWorkPool);

   dabc::mgr.CreateMemoryPool("Pool");

   hadaq::CombinerModule* m = new hadaq::CombinerModule("Combiner");
   dabc::mgr()->MakeThreadFor(m, "EvtBuilderThrd");

   for (unsigned n = 0; n < m->NumInputs(); n++) {
      if (!dabc::mgr.CreateTransport(
            FORMAT(("Combiner/%s%u", mbs::portInput, n)), hadaq::typeUdpInput, "UdpThrd")) {
         EOUT(("Cannot create Netmem client transport %d",n));
         exit(131);
      }
   }
   if (m->IsServOutput()) {
      DOUT0(("!!!!! Create Mbs transmitter module for server !!!!!"));
      dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "OnlineServer",
            "OnlineThrd");
      dabc::mgr.Connect(FORMAT(("OnlineServer/%s", mbs::portInput)),
            FORMAT(("Combiner/%s", mbs::portServerOutput)));
      dabc::mgr.CreateTransport(FORMAT(("OnlineServer/%s", mbs::portOutput)),
            mbs::typeServerTransport, "MbsTransport");
   }

   if (m->IsFileOutput()) {
      DOUT0(("!!!!! Create HLD file output !!!!!"));
      if (!dabc::mgr.CreateTransport("Combiner/FileOutput",
            hadaq::typeHldOutput, "HldFileThrd")) {
         EOUT(("Cannot create HLD file output"));
         exit(133);
      }
   }

   //    m->Start();

   //    DOUT0(("Start MBS combiner module done"));
}


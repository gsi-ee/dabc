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

#include "dabc/Application.h"
#include "dabc/Manager.h"
#include "dabc/logging.h"

#include "hadaq/Observer.h"

#include "mbs/MbsTypeDefs.h"

hadaq::CombinerModule::CombinerModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fInp(),
   fOut(),
   fFlushFlag(false),
   fUpdateCountersFlag(false),
   fWithObserver(false),
   fBuildCompleteEvents(true),
   fFlushTimeout(0.)
{
   EnsurePorts(0, 1, dabc::xmlWorkPool);

   fTotalRecvBytes = 0;
   fTotalRecvEvents = 0;
   fTotalDiscEvents = 0;
   fTotalDroppedData = 0;
   fTotalTagErrors = 0;
   fTotalDataErrors = 0;

   for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn)
      fErrorbitPattern[ptrn] = 0;

   for (unsigned i = 0; i < HADAQ_NEVTIDS; i++)
      fEventIdCount[i] = 0;

   fRunNumber = hadaq::RawEvent::CreateRunId(); // runid from configuration time.

   fTriggerNrTolerance = 50000;

   fLastTrigNr = 0;

   fWithObserver = Cfg(hadaq::xmlObserverEnabled, cmd).AsBool(false);

   fUseSyncSeqNumber = Cfg(hadaq::xmlSyncSeqNumberEnabled, cmd).AsBool(false); // if true, use vulom/roc syncnumber for event sequence number
   fSyncSubeventId = Cfg(hadaq::xmlSyncSubeventId, cmd).AsUInt(0x8000);        //0x8000;
   fSyncTriggerMask = Cfg(hadaq::xmlSyncAcceptedTriggerMask, cmd).AsInt(0x01); // chose bits of accepted trigge sources
   fPrintSync = Cfg("PrintSync", cmd).AsBool(false);

   if (fUseSyncSeqNumber)
      DOUT0("HADAQ combiner module with VULOM sync event sequence number from cts subevent:0x%0x, trigger mask:0x%0x", (unsigned) fSyncSubeventId, (unsigned) fSyncTriggerMask);
   else
      DOUT0("HADAQ combiner module with independent event sequence number");

   std::string ratesprefix = "Hadaq";

   for (unsigned n = 0; n < NumInputs(); n++) {
      fInp.push_back(ReadIterator());
      fCfg.push_back(InputCfg());
      fInp[n].Close();
      fCfg[n].Reset();
   }

   fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);

   if (fFlushTimeout > 0.)
      CreateTimer("FlushTimer", fFlushTimeout, false);

//   CreatePar("RunId");
//   Par("RunId").SetInt(fRunNumber); // to communicate with file components

   fDataRateName = ratesprefix + "Data";
   fEventRateName = ratesprefix + "Events";
   fLostEventRateName = ratesprefix + "LostEvents";
   fDataDroppedRateName = ratesprefix + "DroppedData";
   fInfoName = ratesprefix + "Info";

   CreatePar(fDataRateName).SetRatemeter(false, 5.).SetUnits("MB");
   CreatePar(fEventRateName).SetRatemeter(false, 5.).SetUnits("Ev");
   CreatePar(fLostEventRateName).SetRatemeter(false, 5.).SetUnits("Ev");
   CreatePar(fDataDroppedRateName).SetRatemeter(false, 5.).SetUnits("MB");

   CreateCmdDef("StartHldFile")
      .AddArg("filename", "string", true, "file.hld")
      .AddArg(dabc::xml_maxsize, "int", false, 50)
      .SetArgMinMax(dabc::xml_maxsize, 1, 1000);
   CreateCmdDef("StopHldFile");

   CreatePar(fInfoName, "info").SetSynchron(true, 2., false);
 
   PublishPars("Hadaq/Combiner");

   RegisterExportedCounters();

   fNumCompletedBuffers = 0;
}


hadaq::CombinerModule::~CombinerModule()
{
   fOut.Close().Release();

   fCfg.clear();

   fInp.clear();
}

void hadaq::CombinerModule::ModuleCleanup()
{
   fOut.Close().Release();
   
   for (unsigned n=0;n<fInp.size();n++)
      fInp[n].Reset();

   fCfg.clear();
   
//   DOUT0("First %06x Last %06x Num %u Time %5.2f", firstsync, lastsync, numsync, tm2-tm1);
//   if (numsync>0)
//      DOUT0("Step %5.2f rate %5.2f sync/s", (lastsync-firstsync + 0.) / numsync, (numsync + 0.) / (tm2-tm1));
}



void hadaq::CombinerModule::SetInfo(const std::string& info, bool forceinfo)
{
//   DOUT0("SET INFO: %s", info.c_str());

   dabc::InfoParameter par;

   if (!fInfoName.empty()) par = Par(fInfoName);

   par.SetValue(info);
   if (forceinfo)
      par.FireModified();
}

void hadaq::CombinerModule::ProcessTimerEvent(unsigned timer)
{
   if (fFlushFlag) {
      FlushOutputBuffer();
   }
   fFlushFlag = true;
   if (fUpdateCountersFlag)
      UpdateExportedCounters();
   fUpdateCountersFlag = true;

   ProcessOutputEvent();
}

bool hadaq::CombinerModule::ProcessBuffer(unsigned pool)
{
   ProcessOutputEvent();
   return false;
}


void hadaq::CombinerModule::BeforeModuleStart()
{
   DOUT3("hadaq::CombinerModule::BeforeModuleStart name: %s ", GetName());

   SetInfo("Start HADAQ combiner");

   std::string info = dabc::format(
         "HADAQ combiner module ready. Runid:%d, numinp:%u, numout:%u flush:%3.1f",
         fRunNumber, NumInputs(), NumOutputs(), fFlushTimeout);

   SetInfo(info, true);
   DOUT0(info.c_str());

}

void hadaq::CombinerModule::AfterModuleStop()
{
   DOUT0("%s: Complete Events:%d , BrokenEvents:%d, DroppedData:%d, RecvBytes:%d, data errors:%d, tag errors:%d",
           GetName(), (int) fTotalRecvEvents, (int) fTotalDiscEvents , (int) fTotalDroppedData, (int) fTotalRecvBytes ,(int) fTotalDataErrors ,(int) fTotalTagErrors);

//   std::cout << "----- Combiner Module Statistics: -----" << std::endl;
//   std::cout << "Complete Events:" << fTotalRecvEvents << ", BrokenEvents:"
//         << fTotalDiscEvents << ", DroppedData:" << fTotalDroppedData
//         << ", RecvBytes:" << fTotalRecvBytes << ", data errors:"
//         << fTotalDataErrors << ", tag errors:" << fTotalTagErrors << std::endl;

}


bool hadaq::CombinerModule::FlushOutputBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) {
      DOUT3("FlushOutputBuffer has no buffer to flush");
      return false;
   }
   if (!CanSendToAllOutputs()) {
      DOUT0("FlushOutputBuffer can't send to all outputs NumOutputs() = %u  CanSend(0) = %s CanSend(1) = %s", NumOutputs(), DBOOL(CanSend(0)), DBOOL(CanSend(1)));
      return false;
   }
   dabc::Buffer buf = fOut.Close();
   SendToAllOutputs(buf);
   fFlushFlag = false; // indicate that next flush timeout one not need to send buffer

   return true;
}

void hadaq::CombinerModule::RegisterExportedCounters()
{
   if(!fWithObserver) return;
   DOUT0("##### CombinerModule::RegisterExportedCounters for shmem");
   CreateEvtbuildPar("prefix");

   // FIXME: for time been use second port for file output
   // one should be able to find where file output now and be able to request transport parameters like file names

   dabc::PortRef port = FindPort(OutputName(1));

   if(!port.null()) {
       std::string filename = port.Cfg("url").AsStr();
       //std::cout <<"!! Has filename:"<<filename << std::endl;
       // strip leading path if any:
       size_t pos = filename.rfind("/");
       if (pos!=std::string::npos)
           filename = filename.substr(pos+1, std::string::npos);
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

   for (unsigned i = 0; i < NumInputs(); i++) {
      CreateEvtbuildPar(dabc::format("trigNr%u",i));
      CreateEvtbuildPar(dabc::format("errBit%u",i));
      CreateEvtbuildPar(dabc::format("evtbuildBuff%u", i));
      for (unsigned p = 0; p < HADAQ_NUMERRPATTS; p++)
         CreateEvtbuildPar(dabc::format("errBitStat%u_%u",p,i));
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
     fTotalDroppedData = 0;
     fTotalTagErrors = 0;
     fTotalDataErrors = 0;
     UpdateExportedCounters();
}


bool hadaq::CombinerModule::UpdateExportedCounters()
{
   if(!fWithObserver) return false;
   fUpdateCountersFlag = false; // do not call this from timer again while it is processed
   SetEvtbuildPar("nrOfMsgs", NumInputs());
   SetNetmemPar("nrOfMsgs", NumInputs());
   SetEvtbuildPar("evtsDiscarded",fTotalDiscEvents);
   SetEvtbuildPar("evtsComplete",fTotalRecvEvents);
   SetEvtbuildPar("evtsDataError",fTotalDataErrors);
   SetEvtbuildPar("evtsTagError", fTotalTagErrors);

   // written bytes is updated by hldoutput directly, required by epics control of file size limit
   //SetEvtbuildPar("bytesWritten", fTotalRecvBytes);


   // runid is generated by epics, we do not update it here!
   //SetEvtbuildPar("runId",fRunNumber);
   //SetEvtbuildPar("runId",Par("RunId").AsUInt()); // sync local parameter to observer export
   //std::cout <<"!!!!!! Update run id:"<<Par("RunId").AsUInt() << std::endl;
   for (unsigned i = 0; i < NumInputs(); i++)
   {
      //unsigned trignr= (fCfg[i].fTrigNr << 8) |  (fCfg[i].fTrigTag & 0xff);
      SetEvtbuildPar(dabc::format("trigNr%u",i),fCfg[i].fLastEvtBuildTrigId); // prerecorded id of last complete event
      SetEvtbuildPar(dabc::format("errBit%u",i),fCfg[i].fErrorBits);
      // note: if we get those values here, we are most likely in between event building. use prerecorded values instead
      //              dabc::InputPort* input=Input(i);
      //              if(input)
      //                 {
      //                    float ratio=0;
      //                    if(input->InputNumCanRecv()>0) ratio = input->InputPending() / input->InputNumCanRecv();
      //                    unsigned fillevel= 100*ratio;
      //                    SetEvtbuildPar(dabc::format("evtbuildBuff%d", i), fillevel);
      //                 }

      SetEvtbuildPar(dabc::format("evtbuildBuff%u", i), (int) (100 * fCfg[i].fQueueLevel));

      for (unsigned ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn)
         SetEvtbuildPar(dabc::format("errBitStat%u_%u",ptrn,i),fCfg[i].fErrorbitStats[ptrn]);
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

   DOUT5("CombinerModule::SkipInputBuffer  %d ", ninp);
   return SkipInputBuffers(ninp, 1);

}


bool hadaq::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   DOUT5("CombinerModule::ShiftToNextBuffer %d ", ninp);
   fInp[ninp].Close();

//   if(!SkipInputBuffer(ninp))
//               return false; // discard previous first buffer if we can receive a new one
   if(!CanRecv(ninp)) return false;

   dabc::Buffer buf = Recv(ninp);

   fNumCompletedBuffers++;

   if(!fInp[ninp].Reset(buf)) {
      // use new first buffer of input for work iterator, but leave it in input queue
      DOUT5("CombinerModule::ShiftToNextBuffer %d could not reset to FirstInputBuffer", ninp);
      // skip buffer and try again
      //SkipInputBuffer(ninp);
      return false;
   }

   return true;
}

bool hadaq::CombinerModule::ShiftToNextHadTu(unsigned ninp)
{
   DOUT5("CombinerModule::ShiftToNextHadTu %d begins", ninp);
   bool foundhadtu(false);
   //static unsigned ccount=0;
   while (!foundhadtu) {
      if (!fInp[ninp].IsData())
         if (!ShiftToNextBuffer(ninp)) return false;

      bool res = fInp[ninp].NextHadTu();
      if (!res || (fInp[ninp].hadtu() == 0)) {
         DOUT5("CombinerModule::ShiftToNextHadTu %d has zero NextHadTu()", ninp);

         if(!ShiftToNextBuffer(ninp)) return false;
         //return false;
         DOUT5("CombinerModule::ShiftToNextHadTu in continue ninp:%u", ninp);
         continue;
      }
      foundhadtu = true;
   } //  while (!foundhadtu)
   return true;
}

const uint32_t MaxHadaqTrigger = 0x1000000;

int CalcTrigNumDiff(const uint32_t& prev, const uint32_t& next)
{
   int res = (int) (next) - prev;
   if (res > (int) MaxHadaqTrigger/2) res-=MaxHadaqTrigger; else
   if (res < (int) MaxHadaqTrigger/-2) res+=MaxHadaqTrigger;
   return res;
}

bool hadaq::CombinerModule::ShiftToNextSubEvent(unsigned ninp)
{
   DOUT5("CombinerModule::ShiftToNextSubEvent %d ", ninp);
   fCfg[ninp].Reset();
   bool foundevent(false);
   //static unsigned ccount=0;
   while (!foundevent) {
      bool res = fInp[ninp].NextSubEvent();
      if (!res || (fInp[ninp].subevnt() == 0)) {
         DOUT5("CombinerModule::ShiftToNextSubEvent %d with zero NextSubEvent()", ninp);
         // retry in next hadtu container
         if (!ShiftToNextHadTu(ninp))
            return false; // no more input buffers available
         DOUT5("CombinerModule::ShiftToNextSubEvent in continue ninp %u", ninp);
         continue;
      }

      foundevent = true;

      fCfg[ninp].fLastTrigNr = fCfg[ninp].fTrigNr;

      fCfg[ninp].fTrigNr = fInp[ninp].subevnt()->GetTrigNr() >> 8;
      fCfg[ninp].fTrigTag = fInp[ninp].subevnt()->GetTrigNr() & 0xFF;
      if (fInp[ninp].subevnt()->GetSize() > sizeof(hadaq::RawSubevent)) {
         fCfg[ninp].fEmpty = false;
      }
      fCfg[ninp].fDataError = fInp[ninp].subevnt()->GetDataError();

//      DOUT0("Inp:%u trig:%08u", ninp, fCfg[ninp].fTrigNr);

      fCfg[ninp].fSubId = fInp[ninp].subevnt()->GetId() & 0x7fffffffUL ;

      /* Evaluate trigger type:*/
      /* NEW for trb3: trigger type is part of decoding word*/
      uint32_t val = fInp[ninp].subevnt()->GetTrigTypeTrb3();
      if (val) {
         fCfg[ninp].fTrigType = val;
         //DOUT0("Inp:%u found trb3 trigger type 0x%x", ninp, fCfg[ninp].fTrigType);
      } else {
         /* evaluate trigger type as in HADESproduction eventbuilders here:*/
         unsigned wordNr = 2;
         uint32_t bitmask = 0xff000000; /* extended mask to contain spill on/off bit*/
         uint32_t bitshift = 24;
         // above from args.c defaults
         val = fInp[ninp].subevnt()->Data(wordNr - 1);
         fCfg[ninp].fTrigType = (val & bitmask) >> bitshift;
         //DOUT0("Inp:%u use trb2 trigger type 0x%x", ninp, fCfg[ninp].fTrigType);
      }



      fCfg[ninp].fErrorBits = fInp[ninp].subevnt()->GetErrBits();


      int diff = 1;
      if (fCfg[ninp].fLastTrigNr != 0)
         diff = CalcTrigNumDiff(fCfg[ninp].fTrigNr, fCfg[ninp].fLastTrigNr);

      if (diff!=1)
         EOUT("%d triggers dropped on the input %u", diff, ninp);

   }

   return true;
}

bool hadaq::CombinerModule::DropAllInputBuffers()
{
   DOUT0("hadaq::CombinerModule::DropAllInputBuffers()...");
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


//   DOUT0("hadaq::CombinerModule::BuildEvent() starts");


   unsigned masterchannel = 0;
   uint32_t subeventssize = 0;
   uint32_t mineventid(0), maxeventid(0);
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      if (fInp[ninp].subevnt() == 0)
         if (!ShiftToNextSubEvent(ninp))
            return false; // could not get subevent data on any channel. let framework do something before next try

      uint32_t evid = fCfg[ninp].fTrigNr;

      if (ninp == 0) {
         mineventid = evid;
         maxeventid = evid;
         masterchannel = ninp;
      }

      if (CalcTrigNumDiff(evid, maxeventid) < 0) {
         maxeventid = evid;
         masterchannel = ninp;
      }

      if (CalcTrigNumDiff(mineventid, evid) < 0)
         mineventid = evid;
   } // for ninp

   // we always build event with maximum trigger id = newest event, discard incomplete older events
   uint32_t buildevid = maxeventid;
   uint32_t buildtag = fCfg[masterchannel].fTrigTag;
   int diff = CalcTrigNumDiff(mineventid, maxeventid);

//   DOUT0("Min:%8u Max:%8u diff:%5d", mineventid, maxeventid, diff);

   ///////////////////////////////////////////////////////////////////////////////
   // check too large triggertag difference on input channels, flush input buffers
   if ((fTriggerNrTolerance > 0) && (diff > fTriggerNrTolerance)) {
      SetInfo(
            dabc::format(
                  "Event id difference %d exceeding tolerance window %d, flushing buffers!",
                  diff, fTriggerNrTolerance), true);
      DOUT0("Event id difference %d exceeding tolerance window %d, maxid:%u minid:%u, flushing buffers!",
                  diff, fTriggerNrTolerance, maxeventid, mineventid);
      DropAllInputBuffers();
      return false; // retry on next set of buffers
   }

   ////////////////////////////////////////////////////////////////////////
   // second input loop: skip all subevents until we reach current trignum
   // select inputs which will be used for building
   //bool eventIsBroken=false;
   bool dataError(false), tagError(false);
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      bool foundsubevent = false;
      while (!foundsubevent) {

         uint32_t trignr = fCfg[ninp].fTrigNr;
         uint32_t trigtag = fCfg[ninp].fTrigTag;
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

         } else
         if (CalcTrigNumDiff(trignr, buildevid) > 0) {

            int droppedsize = fInp[ninp].subevnt() ? fInp[ninp].subevnt()->GetSize() : 1;

//            DOUT0("Drop data inp %u size %d", ninp, droppedsize);

            Par(fDataDroppedRateName).SetValue(droppedsize/1024/1024);
            fTotalDroppedData+=droppedsize;

            if(!ShiftToNextSubEvent(ninp)) return false;
            // try with next subevt until reaching buildevid

            continue;
         } else {
            // can happen when the subevent of the buildevid is missing on this channel
            // we account broken event and let call BuildEvent again, rescan buildevid without shifting other subevts.
            return false; // we give the framework some time to do other things though
         }

      } // while foundsubevent
   } // for ninpt

   // here all inputs should be aligned to buildevid

   // for sync sequence number, check first if we have error from cts:
   uint32_t sequencenumber = fTotalRecvEvents;
   bool hascorrectsync = true;
   
   if(fUseSyncSeqNumber) {
      hascorrectsync = false;

      // we may put sync id from subevent payload to event sequence number already here.
      hadaq::RawSubevent* syncsub = fInp[0].subevnt(); // for the moment, sync number must be in first udp input
      // TODO: put this to configuration

      if (syncsub->GetId() != fSyncSubeventId) {
         // main subevent has same id as cts/hub subsubevent
         DOUT1("***  --- sync subevent at input 0x%x has wrong id 0x%x !!! Check configuration.", 0, syncsub->GetId());
      } else {
         unsigned datasize = syncsub->GetNrOfDataWords();
         unsigned syncdata(0), syncnum(0), trigtype(0), trignum(0);
         for (unsigned ix = 0; ix < datasize; ix++) {
            //scan through trb3 data words and look for the cts subsubevent
            unsigned data = syncsub->Data(ix);
            //! Central hub header and inside
            if ((data & 0xFFFF) != fSyncSubeventId) continue;

            unsigned centHubLen = ((data >> 16) & 0xFFFF);
            DOUT5("***  --- central hub header: 0x%x, size=%d", data, centHubLen);

            // evaluate trigger type from cts:
            data = syncsub->Data(ix + 1);
            // old HADES style:
            //                   trigtype=(syncdata >> 24) & 0xFF;
            //                   trignum= (syncdata >> 16) & 0xFF;
            // new cts
            trigtype = (data & 0xFFFF);
            trignum = (data >> 16) & 0xF;
            DOUT5("***  --- CTS trigtype: 0x%x, trignum=0x%x", trigtype, trignum);
            fCfg[0].fTrigType=trigtype; // overwrite default trigger type from main hades cts format
            syncdata = syncsub->Data(ix + centHubLen);
            syncnum = (syncdata & 0xFFFFFF);
            DOUT5("***  --- found sync data: 0x%x, sync number is %d, errorbit %s", syncdata, syncnum, DBOOL((syncdata >> 31) & 0x1) );
            if (fPrintSync) DOUT1("SYNC: 0x%06X, errorbit %s", syncnum, DBOOL((syncdata >> 31) & 0x1) );
            break;
         }
         if(syncnum==0)
         {
            DOUT5("***  --- Found zero sync number!, full sync data:0x%x",syncdata);
         }
         else if ( ((syncdata >> 31) & 0x1) == 0x1)
         {
            DOUT5("***  --- Found error bit at sync number: 0x%x, full sync data:0x%x", syncnum, syncdata);
         }
         //else if (trigtype   != 0) // todo: configure which trigger type contains the sync, currently it'S 0
         else if ((trigtype & fSyncTriggerMask) == 0x0)
         {
            DOUT5("***  --- Found not accepted trigger type :0x%x , full sync data:0x%x ",trigtype,syncdata);
         } else {
            //                  DOUT1("FIND SYNC in HADAQ %06x", syncnum);
            sequencenumber=syncnum;
            hascorrectsync=true;
         }

      } // if (syncsub->GetId() != fSyncSubeventId)

   } // if(fUseSyncSeqNumber)




   // provide normal buffer

   if (hascorrectsync) {
      if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(subeventssize)) {
         // no, we close current buffer
         if (!FlushOutputBuffer()) {
            DOUT0("Could not flush buffer");
            return false;
         }

      }
      // after flushing last buffer, take next one:
      if (!fOut.IsBuffer()) {
         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) {
            // DOUT0("did not have new buffer - wait for it");
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
      if (!fOut.IsPlaceForEvent(subeventssize)) {
         DOUT0("New buffer has not enough space, skip subevent!");
         hascorrectsync = false;
      }
   }


   // now we should be able to build event
   if (hascorrectsync) {
      // EVENT BUILDING IS HERE
      fOut.NewEvent(sequencenumber, fRunNumber); // like in hadaq, event sequence number is independent of trigger.
      fTotalRecvEvents++;

      fOut.evnt()->SetDataError((dataError || tagError));
      if (dataError)
         fTotalDataErrors++;
      if (tagError)
         fTotalTagErrors++;

      // here event id, always from "cts master channel" 0
      unsigned currentid = fCfg[0].fTrigType | (2 << 12); // DAQVERSION=2 for dabc
      fEventIdCount[currentid & (HADAQ_NEVTIDS - 1)]++;
      fOut.evnt()->SetId(currentid & (HADAQ_NEVTIDS_IN_FILE - 1));

      // third input loop: build output event from all not empty subevents
      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
         if (fCfg[ninp].fEmpty) continue;
         fOut.AddSubevent(fInp[ninp].subevnt());
         DoInputSnapshot(ninp); // record current state of event tag and queue level for control system
      } // for ninp

      fOut.FinishEvent();


      int diff = 1;
      if (fLastTrigNr!=0) diff = CalcTrigNumDiff(fLastTrigNr, buildevid);

      fLastTrigNr = buildevid;

      Par(fEventRateName).SetValue(1);
      if (diff>1) {
         DOUT2("Events gap %d", diff);
         Par(fLostEventRateName).SetValue(diff);
         fTotalDiscEvents+=diff;
      }

      unsigned currentbytes = subeventssize + sizeof(hadaq::RawEvent);
      fTotalRecvBytes += currentbytes;
      Par(fDataRateName).SetValue(currentbytes / 1024. / 1024.);

   } else {
      EOUT("Event dropped hassync %s subevsize %u", DBOOL(hascorrectsync), subeventssize);
      Par(fLostEventRateName).SetValue(1);
      fTotalDiscEvents+=1;
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

   unsigned capacity = PortQueueCapacity(InputName(ninp));

   float ratio=0;
   if(capacity>0) ratio = 1. * NumCanRecv(ninp) / capacity;
   fCfg[ninp].fQueueLevel = ratio;
   fCfg[ninp].fLastEvtBuildTrigId = (fCfg[ninp].fTrigNr << 8) |  (fCfg[ninp].fTrigTag & 0xff);
}


int hadaq::CombinerModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("StartHldFile")) {

      std::string fname = cmd.GetStr("filename");
      int maxsize = cmd.GetInt(dabc::xml_maxsize, 30);

      std::string url = dabc::format("hld://%s?%s=%d", fname.c_str(), dabc::xml_maxsize, maxsize);

      // we guarantee, that at least two ports will be created
      EnsurePorts(0, 2);

      bool res = dabc::mgr.CreateTransport(OutputName(1, true), url);

      DOUT0("Start HLD file %s res = %s", fname.c_str(), DBOOL(res));

      SetInfo("Execute StartHldFile");

      return cmd_bool(res);
   } else
   if (cmd.IsName("StopHldFile")) {

      bool res = true;

      SetInfo("Execute StopHldFile");

      if (NumOutputs()>1)
         res = DisconnectPort(OutputName(1));

      DOUT0("Stop HLD file res = %s", DBOOL(res));

      return cmd_bool(res);
   }

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
    Par(GetEvtbuildParName(name)).SetValue(value);
}

unsigned hadaq::CombinerModule::GetEvtbuildParValue(const std::string& name)
{
   return Par(GetEvtbuildParName(name)).Value().AsUInt();
}


std::string  hadaq::CombinerModule::GetNetmemParName(const std::string& name)
{
   return dabc::format("%s_%s",hadaq::NetmemPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateNetmemPar(const std::string& name)
{
   CreatePar(GetNetmemParName(name));
}

void hadaq::CombinerModule::SetNetmemPar(const std::string& name, unsigned value)
{
   Par(GetNetmemParName(name)).SetValue(value);
}


extern "C" void InitHadaqEvtbuilder()
{
   if (dabc::mgr.null()) {
      EOUT("Manager is not created");
      exit(1);
   }

   DOUT0("Create Hadaq combiner module");

   //dabc::mgr.CreateMemoryPool(dabc::xmlWorkPool);

   dabc::mgr.CreateMemoryPool("Pool");

   dabc::ModuleRef m = dabc::mgr.CreateModule("hadaq::CombinerModule", "Combiner", "EvtBuilderThrd");

   for (unsigned n = 0; n < m.NumInputs(); n++)
      if (!dabc::mgr.CreateTransport(m.InputName(n))) {
         EOUT("Cannot create Netmem client transport %d",n);
         exit(131);
      }


   unsigned cnt(0);
   // create special module to produce MBS events
   if ((m.NumOutputs() > 0) && m.Cfg("DoServer").AsBool()) {
      dabc::ModuleRef m2 = dabc::mgr.CreateModule("hadaq::MbsTransmitterModule", "OnlineServer", "OnlineThrd");

      dabc::mgr.Connect("Combiner/Output0", "OnlineServer/Input0");

      for (unsigned n=0;n<m2.NumOutputs();n++)
         dabc::mgr.CreateTransport(m2.OutputName(n));
      cnt++;
   }

   // additional outputs, which can be used for files
   if ((m.NumOutputs() > 1) && m.Cfg("DoFile").AsBool())
      if (!dabc::mgr.CreateTransport(m.OutputName(1))) {
         EOUT("Cannot create file transport");
         exit(131);
      }
}


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

#include "hadaq/CombinerModule.h"

#include <math.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#include "dabc/Application.h"
#include "dabc/Manager.h"
#include "dabc/logging.h"

#include "hadaq/Observer.h"
#include "hadaq/UdpTransport.h"

#include "mbs/MbsTypeDefs.h"

//#define HADERRBITDEBUG 1


hadaq::CombinerModule::CombinerModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fInp(),
   fOut(),
   fFlushCounter(0),
   fWithObserver(false),
   fEpicsSlave(false),
   fRunToOracle(false),
   fBuildCompleteEvents(true),
   fFlushTimeout(0.),
   fEvnumDiffStatistics(true)
{
   EnsurePorts(0, 1, dabc::xmlWorkPool);

   fTotalRecvBytes = 0;
   fTotalBuildEvents = 0;
   fTotalDiscEvents = 0;
   fTotalDroppedData = 0;
   fTotalTagErrors = 0;
   fTotalDataErrors = 0;

   for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn)
      fErrorbitPattern[ptrn] = 0;

   for (unsigned i = 0; i < HADAQ_NEVTIDS; i++)
      fEventIdCount[i] = 0;

   fEBId = Cfg("NodeId", cmd).AsInt(-1);
   if (fEBId<0) fEBId = dabc::mgr.NodeId()+1; // hades eb ids start with 1

   fExtraDebug = Cfg("ExtraDebug", cmd).AsBool(true);

   fRunNumber = hadaq::CreateRunId(); // runid from configuration time.
   fEpicsRunNumber = 0;

   fLastTrigNr = 0;
   fMaxHadaqTrigger = 0;
   fTriggerRangeMask = 0;

   fWithObserver = Cfg(hadaq::xmlObserverEnabled, cmd).AsBool(false);
   fEpicsSlave =   Cfg(hadaq::xmlExternalRunid, cmd).AsBool(false);

   if(fEpicsSlave) fRunNumber=0; // ignore data without valid run id at beginning!

   fMaxHadaqTrigger = Cfg(hadaq::xmlHadaqTrignumRange, cmd).AsUInt(0x1000000);
   fTriggerRangeMask = fMaxHadaqTrigger-1;
   DOUT1("HADAQ combiner module using maxtrigger 0x%x, rangemask:0x%x", fMaxHadaqTrigger, fTriggerRangeMask);
   fEvnumDiffStatistics = Cfg(hadaq::xmlHadaqDiffEventStats, cmd).AsBool(true);

   fTriggerNrTolerance = fMaxHadaqTrigger / 4;
   fEventBuildTimeout = Cfg(hadaq::xmlEvtbuildTimeout, cmd).AsDouble(20.0); // 20 seconds configure this optionally from xml later
   fUseSyncSeqNumber = Cfg(hadaq::xmlSyncSeqNumberEnabled, cmd).AsBool(false); // if true, use vulom/roc syncnumber for event sequence number
   fSyncSubeventId = Cfg(hadaq::xmlSyncSubeventId, cmd).AsUInt(0x8000);        //0x8000;
   fSyncTriggerMask = Cfg(hadaq::xmlSyncAcceptedTriggerMask, cmd).AsInt(0x01); // chose bits of accepted trigge sources
   fPrintSync = Cfg("PrintSync", cmd).AsBool(false);

   if (fUseSyncSeqNumber)
      DOUT1("HADAQ combiner module with VULOM sync event sequence number from cts subevent:0x%0x, trigger mask:0x%0x", (unsigned) fSyncSubeventId, (unsigned) fSyncTriggerMask);
   else
      DOUT1("HADAQ combiner module with independent event sequence number");

   std::string ratesprefix = "Hadaq";

   for (unsigned n = 0; n < NumInputs(); n++) {
      fInp.push_back(ReadIterator());
      fCfg.push_back(InputCfg());
      fInp[n].Close();
      fCfg[n].Reset(true);
   }

   fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);

   // provide timeout with period/2, but trigger flushing after 3 counts
   // this will lead to effective flush time between FlushTimeout and FlushTimeout*1.5
   if (fFlushTimeout > 0.)
      CreateTimer("FlushTimer", fFlushTimeout/2., false);

   //CreatePar("RunId");
   //Par("RunId").SetValue(fRunNumber); // to communicate with file components

   fRunInfoToOraFilename = dabc::format("eb_runinfo2ora_%d.txt",fEBId);
   // TODO: optionally set this name
   fPrefix = Cfg("FilePrefix", cmd).AsStr("no");
   fRunToOracle = Cfg("Runinfo2ora", cmd).AsBool(false);

   fDataRateName = ratesprefix + "Data";
   fEventRateName = ratesprefix + "Events";
   fLostEventRateName = ratesprefix + "LostEvents";
   fDataDroppedRateName = ratesprefix + "DroppedData";
   fInfoName = ratesprefix + "Info";

   CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar(fEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar(fLostEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar(fDataDroppedRateName).SetRatemeter(false, 3.).SetUnits("MB");

   CreateCmdDef("StartHldFile")
      .AddArg("filename", "string", true, "file.hld")
      .AddArg(dabc::xml_maxsize, "int", false, 1500)
      .SetArgMinMax(dabc::xml_maxsize, 1, 5000);
   CreateCmdDef("StopHldFile");

   CreatePar(fInfoName, "info").SetSynchron(true, 2., false).SetDebugLevel(2);

   PublishPars("$CONTEXT$/HadaqCombiner");

   fWorkerHierarchy.SetField("_player", "DABC.HadaqDAQControl");

   if (fWithObserver) {
      CreateTimer("ObserverTimer", 0.2, false); // export timers 5 times a second
      RegisterExportedCounters();
   }

   fNumReadBuffers = 0;
}


hadaq::CombinerModule::~CombinerModule()
{
   fOut.Close().Release();

   fCfg.clear();

   fInp.clear();
}

void hadaq::CombinerModule::ModuleCleanup()
{
   StoreRunInfoStop(true); // run info with exit mode

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
   if (TimerName(timer) == "ObserverTimer") {
      UpdateExportedCounters();
      return;
   }

   if (++fFlushCounter > 2) {
      fFlushCounter = 0;
      FlushOutputBuffer();
   }

   // try to build event - just to keep event loop running
   int cnt =100;
   while (cnt > 0 && BuildEvent()) cnt--;

   // FIXME: should we keep it here ???
   ProcessOutputEvent(0);
}

bool hadaq::CombinerModule::ProcessBuffer(unsigned pool)
{
   // try to build event - just to keep event loop running
   int cnt =100;
   while (cnt > 0 && BuildEvent()) cnt--;

   // FIXME: should we keep it here ???
   ProcessOutputEvent(0);
   return false;
}


void hadaq::CombinerModule::BeforeModuleStart()
{
   std::string info = dabc::format(
         "HADAQ Combiner starts. Runid:%d, numinp:%u, numout:%u flush:%3.1f",
         (int) fRunNumber, NumInputs(), NumOutputs(), fFlushTimeout);

   SetInfo(info, true);
   DOUT0(info.c_str());
   fLastDropTm.GetNow();

   fLastProcTm.GetNow();
   fLastBuildTm.GetNow();

   // direct addon pointers can be used for terminal printout
   for (unsigned n=0;n<fCfg.size();n++) {
      dabc::Command cmd("GetHadaqTransportInfo");
      cmd.SetInt("id", n);
      SubmitCommandToTransport(InputName(n), Assign(cmd));
   }
}

void hadaq::CombinerModule::AfterModuleStop()
{
   std::string info = dabc::format(
      "HADAQ Combiner stopped. CompleteEvents:%d, BrokenEvents:%d, DroppedData:%d, RecvBytes:%d, data errors:%d, tag errors:%d",
       (int) fTotalBuildEvents, (int) fTotalDiscEvents , (int) fTotalDroppedData, (int) fTotalRecvBytes ,(int) fTotalDataErrors ,(int) fTotalTagErrors);

   SetInfo(info, true);
   DOUT0(info.c_str());
}


bool hadaq::CombinerModule::FlushOutputBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) {
      DOUT3("FlushOutputBuffer has no buffer to flush");
      return false;
   }

   if (!CanSendToAllOutputs()) return false;

   dabc::Buffer buf = fOut.Close();
   SendToAllOutputs(buf);
   fFlushCounter = 0; // indicate that next flush timeout one not need to send buffer

   return true;
}

void hadaq::CombinerModule::RegisterExportedCounters()
{
   if(!fWithObserver) return;
   DOUT3("##### CombinerModule::RegisterExportedCounters for shmem");
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

   CreateEvtbuildPar("diskNum"); // this gets number of partition from daq_disks
   SetEvtbuildPar("diskNum", 1); // default for testing!

   CreateEvtbuildPar("diskNumEB"); // this is used for epics gui
   SetEvtbuildPar("diskNumEB", 0); // default for testing!

   CreateEvtbuildPar("dataMover"); // rfio data mover number for epics gui
   SetEvtbuildPar("dataMover", 0); // default for testing!

   CreateEvtbuildPar("coreNr");

   // NOTE: this export has no effect, since pid is exported by default in worker mechanism
   // and worker names are not case sensitive!
   // thus epics will always take pid of the observer process that opens shared memory
   CreateEvtbuildPar("PID");
   //fPID=getpid();
   //fPID= syscall(SYS_gettid);

   SetEvtbuildPar("PID", (int) fPID);
   SetEvtbuildPar("coreNr", hadaq::CoreAffinity(fPID));
}


bool hadaq::CombinerModule::UpdateExportedCounters()
{
   if(!fWithObserver) return false;

   // written bytes is updated by hldoutput directly, required by epics control of file size limit
   //SetEvtbuildPar("bytesWritten", fTotalRecvBytes);

   // runid is generated by epics, also refresh it here!
   if(fEpicsSlave)
   {
      fEpicsRunNumber = GetEvtbuildParValue("runId");
      //std::cout <<"!!!!!! UpdateExportedCounters() got epics run id:"<< fEpicsRunNumber<< std::endl;

      if((fEpicsRunNumber!=0) && (fEpicsRunNumber!=fRunNumber)) {
        // run number 0 can occur when master eventbuilder terminates
        // ignore this case, since it may store wrong run info stop time
         DOUT1("Combiner in EPICS slave mode found new RUN ID %d (previous=%d)!",fEpicsRunNumber, fRunNumber);
         StoreRunInfoStop();
         fRunNumber = fEpicsRunNumber;

         fTotalRecvBytes = 0;
         fTotalBuildEvents = 0;
         fTotalDiscEvents = 0;
         fTotalDroppedData = 0;
         fTotalTagErrors = 0;
         fTotalDataErrors = 0;


         for (unsigned n=0;n<NumInputs();n++)
         {
            SubmitCommandToTransport(InputName(n), dabc::Command("ResetExportedCounters"));

            // JAM BUGFIX JUL14: errorbits not reset
            for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS;++ptrn) {
               fErrorbitPattern[ptrn] = 0;
               fCfg[n].fErrorbitStats[ptrn]=0;
            }
            fCfg[n].fLastEvtBuildTrigId=0;
         }

         for (unsigned i = 0; i < HADAQ_NEVTIDS; i++)
            fEventIdCount[i]=0;

         StoreRunInfoStart();
      }
   }

   //    static int affcount=0;
   //    if(affcount++ % 20)
   //     {
   //       SetEvtbuildPar("coreNr", hadaq::CoreAffinity(fPID));
   //       //SetEvtbuildPar("coreNr", hadaq::CoreAffinity(0));
   //     }


   SetEvtbuildPar("nrOfMsgs", NumInputs());
   SetNetmemPar("nrOfMsgs", NumInputs());
   SetEvtbuildPar("evtsDiscarded", fTotalDiscEvents);
   SetEvtbuildPar("evtsComplete", fTotalBuildEvents);
   SetEvtbuildPar("evtsDataError",fTotalDataErrors);
   SetEvtbuildPar("evtsTagError", fTotalTagErrors);

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
      SetEvtbuildPar(dabc::format("evtId%d",i), fEventIdCount[i]);


   // test: provide here some frequent output for logfile
   double drate= Par(fDataRateName).Value().AsDouble();
   double erate= Par(fEventRateName).Value().AsDouble();
   double losterate=Par(fLostEventRateName).Value().AsDouble();
   double dropdrate=Par(fDataDroppedRateName).Value().AsDouble();
   static double oldlostrate=0;
   /////////////////
   // lostrate of 1 is expected since we still have
   // wrong overflow behaviour for triggernumber.
   // so only logfile warnings beyond 1 ->
   if((losterate>1 || dropdrate>0) && (losterate!=oldlostrate))
   {
      oldlostrate=losterate;
      std::string info = dabc::format(
            "Lost Event rate: %.0f Ev/s, Dropped data rate: %.3f Mb/s  (at %.0f Ev/s, %.3f Mb/s)",
            losterate, dropdrate , erate, drate);
      SetInfo(info, true);
      DOUT0(info.c_str());
   }
   return true;
}


///////////////////////////////////////////////////////////////
//////// INPUT BUFFER METHODS:


bool hadaq::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   DOUT5("CombinerModule::ShiftToNextBuffer %d ", ninp);
   fInp[ninp].Close();

   if(!CanRecv(ninp)) return false;

   dabc::Buffer buf = Recv(ninp);

   fNumReadBuffers++;

   if(!fInp[ninp].Reset(buf)) {
      // use new first buffer of input for work iterator, but leave it in input queue
      DOUT5("CombinerModule::ShiftToNextBuffer %d could not reset to FirstInputBuffer", ninp);
      // skip buffer and try again
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


int hadaq::CombinerModule::CalcTrigNumDiff(const uint32_t& prev, const uint32_t& next)
{
   int res = (int) (next) - prev;
   if (res > (int) fMaxHadaqTrigger/2) res -= fMaxHadaqTrigger; else
   if (res < (int) fMaxHadaqTrigger/-2) res += fMaxHadaqTrigger;
   return res;
}

bool hadaq::CombinerModule::ShiftToNextSubEvent(unsigned ninp, bool fast, bool dropped)
{
   DOUT5("CombinerModule::ShiftToNextSubEvent %d ", ninp);

   fCfg[ninp].Reset(fast);
   bool foundevent(false);

   if (fast) DOUT0("FAST DROP on inp %d", ninp);

   // account when subevent exists but intentionally dropped
   if (dropped && fInp[ninp].subevnt()) fCfg[ninp].fDroppedTrig++;

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

      // no need to analyze data
      if (fast) return true;

      foundevent = true;

      fCfg[ninp].fTrigNr = ((fInp[ninp].subevnt()->GetTrigNr() >> 8) & fTriggerRangeMask); // only use 16 bit range for trb2/trb3

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
#ifndef HADERRBITDEBUG
      fCfg[ninp].fErrorBits = fInp[ninp].subevnt()->GetErrBits();
#else
      fCfg[ninp].fErrorBits = ninp;
#endif
      int diff = 1;
      if (fCfg[ninp].fLastTrigNr != 0)
         diff = CalcTrigNumDiff(fCfg[ninp].fLastTrigNr, fCfg[ninp].fTrigNr);
      fCfg[ninp].fLastTrigNr = fCfg[ninp].fTrigNr;

      if (diff>1) fCfg[ninp].fLostTrig += (diff-1);
   }

   return true;
}

bool hadaq::CombinerModule::DropAllInputBuffers()
{
   DOUT0("hadaq::CombinerModule::DropAllInputBuffers()...");

   unsigned maxnumsubev(0), droppeddata(0);

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      unsigned numsubev = 0;

      do {
         if (fInp[ninp].subevnt()) {
            numsubev++;
            droppeddata += fInp[ninp].subevnt()->GetSize();
         }
      } while (ShiftToNextSubEvent(ninp, true, true));

      if (numsubev>maxnumsubev) maxnumsubev = numsubev;

      fInp[ninp].Close();
      while (SkipInputBuffers(ninp, 100)); // drop input port queue buffers until no more there
   }

   Par(fLostEventRateName).SetValue(maxnumsubev);
   Par(fDataDroppedRateName).SetValue(droppeddata/1024./1024.);
   fTotalDiscEvents += maxnumsubev;
   fTotalDroppedData += droppeddata;

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

   if (fExtraDebug) {
      double tm = fLastProcTm.SpentTillNow(true);
      if (tm > fMaxProcDist) fMaxProcDist = tm;
   }

   // DOUT0("hadaq::CombinerModule::BuildEvent() starts");

   unsigned masterchannel(0), min_inp(0);
   uint32_t subeventssize = 0;
   uint32_t mineventid(0), maxeventid(0), buildevid(0);
   bool incomplete_data(false), any_data(false);
   int missing_inp(-1);

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      if (fInp[ninp].subevnt() == 0)
         if (!ShiftToNextSubEvent(ninp)) {
            // could not get subevent data on any channel.
            // let framework do something before next try
            if (fExtraDebug && fLastDebugTm.Expired(2.)) {
               DOUT1("Fail to build event while input %u is not ready maxtm = %5.3f ", ninp, fMaxProcDist);
               fLastDebugTm.GetNow();
               fMaxProcDist = 0;
            }

            missing_inp = ninp;
            incomplete_data = true;
            continue;
         }

      uint32_t evid = fCfg[ninp].fTrigNr;

      if (!any_data) {
         any_data = true;
         mineventid = evid;
         maxeventid = evid;
         buildevid = evid;
      }

      if (CalcTrigNumDiff(evid, maxeventid) < 0)
         maxeventid = evid;

      if (CalcTrigNumDiff(mineventid, evid) < 0) {
         mineventid = evid;
         min_inp = ninp;
      }
   } // for ninp

   // we always build event with maximum trigger id = newest event, discard incomplete older events
   int diff = incomplete_data ? 0 : CalcTrigNumDiff(mineventid, maxeventid);

//   DOUT0("Min:%8u Max:%8u diff:%5d", mineventid, maxeventid, diff);

   ///////////////////////////////////////////////////////////////////////////////
   // check too large triggertag difference on input channels, flush input buffers
   if (fLastDropTm.Expired(5.))
     if (((fTriggerNrTolerance > 0) && (diff > fTriggerNrTolerance)) || (fLastBuildTm.Expired(fEventBuildTimeout) && any_data)) {

        std::string msg;
        if ((fTriggerNrTolerance > 0) && (diff > fTriggerNrTolerance)) {
          msg = dabc::format(
              "Event id difference %d exceeding tolerance window %d (min input %u),",
              diff, fTriggerNrTolerance, min_inp);
        } else {
           msg = dabc::format("No events were build since at least %.1f seconds,", fEventBuildTimeout);
        }

        if (missing_inp >= 0) {
           msg += dabc::format(" missing data on input %d url: %s,", missing_inp, FindPort(InputName(missing_inp)).Cfg("url").AsStr().c_str());
        }
        msg += " drop all!";

        SetInfo(msg, true);
        DOUT0(msg.c_str());

        DropAllInputBuffers();

        fTotalFullDrops++;

        if (fExtraDebug && fLastDebugTm.Expired(1.)) {
           DOUT1("Drop all buffers");
           fLastDebugTm.GetNow();
        }
        fLastDropTm.GetNow();

        return false; // retry on next set of buffers
     }

   if (incomplete_data) return false;

   uint32_t buildtag = fCfg[masterchannel].fTrigTag;

   // printf("build evid = %u\n", buildevid);

   ////////////////////////////////////////////////////////////////////////
   // second input loop: skip all subevents until we reach current trignum
   // select inputs which will be used for building
   //bool eventIsBroken=false;
   bool dataError(false), tagError(false);

   bool hasCompleteEvent = true;

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

            // DOUT0("Drop data inp %u size %d", ninp, droppedsize);

            Par(fDataDroppedRateName).SetValue(droppedsize/1024./1024.);
            fTotalDroppedData += droppedsize;

            if(!ShiftToNextSubEvent(ninp, false, true)) {
               if (fExtraDebug && fLastDebugTm.Expired(2.)) {
                  DOUT1("Cannot shift data from input %d", ninp);
                  fLastDebugTm.GetNow();
               }

               return false;
            }
            // try with next subevt until reaching buildevid

            continue;
         } else {

            // we want to build event with id, defined by input 0
            // but subevent in this input has number bigger than buildevid
            // it will not be possible to build buildevid, therefore mark it as incomplete
            hasCompleteEvent = false;

            // let also verify all other channels
            break;
         }

      } // while foundsubevent
   } // for ninpt

   // here all inputs should be aligned to buildevid

   // for sync sequence number, check first if we have error from cts:
   uint32_t sequencenumber = fTotalBuildEvents + 1; // HADES convention: sequencenumber 0 is "start event" of file

   if(fUseSyncSeqNumber && hasCompleteEvent) {
      hasCompleteEvent = false;

      // we may put sync id from subevent payload to event sequence number already here.
      hadaq::RawSubevent* syncsub = fInp[0].subevnt(); // for the moment, sync number must be in first udp input
      // TODO: put this to configuration

      if (syncsub->GetId() != fSyncSubeventId) {
         // main subevent has same id as cts/hub subsubevent
         DOUT1("***  --- sync subevent at input 0x%x has wrong id 0x%x !!! Check configuration.", 0, syncsub->GetId());
      } else {
         unsigned datasize = syncsub->GetNrOfDataWords();
         unsigned syncdata(0), syncnum(0), trigtype(0); //, trignum(0);
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
            //trignum = (data >> 16) & 0xF;
            DOUT5("***  --- CTS trigtype: 0x%x, trignum=0x%x", trigtype, (data >> 16) & 0xF);
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
            hasCompleteEvent=true;
         }

      } // if (syncsub->GetId() != fSyncSubeventId)

   } // if(fUseSyncSeqNumber)


   // provide normal buffer

   if (hasCompleteEvent) {
      if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(subeventssize)) {
         // no, we close current buffer
         if (!FlushOutputBuffer()) {
            if (fExtraDebug && fLastDebugTm.Expired(1.)) {
               std::string sendmask;
               for (unsigned n=0;n<NumOutputs();n++)
                  sendmask.append(CanSend(n) ? "o" : "x");

               DOUT0("FlushOutputBuffer can't send to all %u outputs sendmask = %s", NumOutputs(), sendmask.c_str());
               fLastDebugTm.GetNow();
            }
            return false;
         }
      }
      // after flushing last buffer, take next one:
      if (!fOut.IsBuffer()) {
         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) {

            if (fExtraDebug && fLastDebugTm.Expired(1.)) {
               DOUT0("did not have new buffer - wait for it");
               fLastDebugTm.GetNow();
            }

            return false;
         }
         if (!fOut.Reset(buf)) {
            SetInfo("Cannot use buffer for output - hard error!!!!", true);
            buf.Release();
            dabc::mgr.StopApplication();
            if (fExtraDebug && fLastDebugTm.Expired(1.)) {
               DOUT0("Abort application completely");
               fLastDebugTm.GetNow();
            }
            return false;
         }
      }
      // now check working buffer for space:
      if (!fOut.IsPlaceForEvent(subeventssize)) {
         DOUT0("New buffer has not enough space, skip subevent!");
         hasCompleteEvent = false;
      }
   }

   // now we should be able to build event
   if (hasCompleteEvent) {
      // EVENT BUILDING IS HERE

      fOut.NewEvent(sequencenumber, fRunNumber); // like in hadaq, event sequence number is independent of trigger.
      fTotalBuildEvents++;

      fOut.evnt()->SetDataError((dataError || tagError));
      if (dataError)
         fTotalDataErrors++;
      if (tagError)
         fTotalTagErrors++;

      // here event id, always from "cts master channel" 0
      unsigned currentid = fCfg[0].fTrigType | (2 << 12); // DAQVERSION=2 for dabc
      //fEventIdCount[currentid & (HADAQ_NEVTIDS - 1)]++;
      fEventIdCount[currentid & 0xF]++; // JAM: problem with spill bit?
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
      if (fEvnumDiffStatistics && (diff>1)) {
         if (fExtraDebug && fLastDebugTm.Expired(1.)) {
            DOUT1("Events gap %d", diff-1);
            fLastDebugTm.GetNow();
         }
         Par(fLostEventRateName).SetValue(diff-1);
         fTotalDiscEvents+=(diff-1);
      }

      unsigned currentbytes = subeventssize + sizeof(hadaq::RawEvent);
      fTotalRecvBytes += currentbytes;
      Par(fDataRateName).SetValue(currentbytes / 1024. / 1024.);

      fLastBuildTm.GetNow();
   } else {
      Par(fLostEventRateName).SetValue(1);
      fTotalDiscEvents += 1;
   } // ensure outputbuffer

   std::string debugmask;
   debugmask.resize(fCfg.size(), ' ');

   // FINAL loop: proceed to next subevents
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
      if (fCfg[ninp].fTrigNr == buildevid) {
         debugmask[ninp] = 'o';
         ShiftToNextSubEvent(ninp, false, !hasCompleteEvent);
      } else {
         debugmask[ninp] = 'x';
      }

   if (fExtraDebug && fLastDebugTm.Expired(1.)) {
      DOUT1("Did building as usual mask %s complete = %5s maxdist = %5.3f s", debugmask.c_str(), DBOOL(hasCompleteEvent), fMaxProcDist);
      fLastDebugTm.GetNow();
      fMaxProcDist = 0;
      // put here update of tid
      // fPID= syscall(SYS_gettid);
   }

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
   fCfg[ninp].fNumCanRecv = NumCanRecv(ninp);
   if(capacity>0) ratio = 1. * fCfg[ninp].fNumCanRecv / capacity;
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
   return dabc::format("%s-%s",hadaq::EvtbuildPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateEvtbuildPar(const std::string& name)
{
   CreatePar(GetEvtbuildParName(name)).SetSynchron(true, 0.2);
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
   return dabc::format("%s-%s",hadaq::NetmemPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateNetmemPar(const std::string& name)
{
   CreatePar(GetNetmemParName(name)).SetSynchron(true, 0.2);
}

void hadaq::CombinerModule::SetNetmemPar(const std::string& name, unsigned value)
{
   Par(GetNetmemParName(name)).SetValue(value);
}


void hadaq::CombinerModule::StoreRunInfoStart()
{
   /* open ascii file eb_runinfo2ora.txt to store simple information for
      the started RUN. The format: start <run_id> <filename> <date> <time>
      where "start" is a key word which defines START RUN info. -S.Y.
    */
   if(!fRunToOracle || fRunNumber==0) return;
   time_t t = fRunNumber + hadaq::HADAQ_TIMEOFFSET; // new run number defines start time
   FILE *fp;
   char ltime[20];            /* local time */
   struct tm tm_res;
   strftime(ltime, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &tm_res));
   std::string filename=GenerateFileName(fRunNumber); // new run number defines filename
   fp = fopen(fRunInfoToOraFilename.c_str(), "a+");
   fprintf(fp, "start %u %d %s %s\n", fRunNumber, fEBId, filename.c_str(), ltime);
   fclose(fp);
   DOUT1("Write run info to %s - start: %lu %d %s %s ", fRunInfoToOraFilename.c_str(), fRunNumber, fEBId, filename.c_str(), ltime);

}

void hadaq::CombinerModule::StoreRunInfoStop(bool onexit)
{
   /* open ascii file eb_runinfo2ora.txt to store simple information for
      the stoped RUN. The format: stop <run_id> <date> <time> <events> <bytes>
      where "stop" is a key word which defines STOP RUN info. -S.Y.
    */

   if(!fRunToOracle || fRunNumber==0) return; // suppress void output at beginning
   // JAM we do not use our own time, but time of next run given by epics master
   // otherwise mismatch between run start time that comes before run stop time!
   // note that this problem also occured with old EBs
   // only exception: when eventbuilder is discarded we use termination time!
   time_t t;
   if(onexit)
      t = time(NULL);
   else
      t = fEpicsRunNumber + hadaq::HADAQ_TIMEOFFSET; // new run number defines stop time
   FILE *fp;
   char ltime[20];            /* local time */
   struct tm tm_res;
   strftime(ltime, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &tm_res));
   fp = fopen(fRunInfoToOraFilename.c_str(), "a+");
        std::string filename=GenerateFileName(fRunNumber); // old run number defines old filename
        fprintf(fp, "stop %u %d %s %s %s ", fRunNumber, fEBId, filename.c_str(), ltime, Unit(fTotalBuildEvents));
        fprintf(fp, "%s\n", Unit(fTotalRecvBytes));
        fclose(fp);
   DOUT1("Write run info to %s - stop: %lu %d %s %s %s %s", fRunInfoToOraFilename.c_str(), fRunNumber, fEBId, filename.c_str(), ltime, Unit(fTotalBuildEvents),Unit(fTotalRecvBytes));
}



char* hadaq::CombinerModule::Unit(unsigned long v)
{

  // JAM stolen from old hadaq eventbuilders to keep precisely same format
   static char retVal[6];
   static char u[] = " kM";
   unsigned int i;

   for (i = 0; v >= 10000 && i < sizeof(u) - 2; v /= 1000, i++) {
   }
   sprintf(retVal, "%4lu%c", v, u[i]);

   return retVal;
}

std::string hadaq::CombinerModule::GenerateFileName(unsigned runid)
{
   return fPrefix +  hadaq::FormatFilename(fRunNumber,fEBId) + std::string(".hld");
}


bool hadaq::CombinerModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetHadaqTransportInfo")) {

      unsigned id = cmd.GetUInt("id");

      if (id < fCfg.size()) {
         fCfg[id].fAddon = cmd.GetPtr("Addon");
         fCfg[id].fCalibr = cmd.GetPtr("CalibrModule");
      }

      return true;
   }

   return dabc::ModuleAsync::ReplyCommand(cmd);
}

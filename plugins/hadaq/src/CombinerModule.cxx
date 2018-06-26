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

//#define HADERRBITDEBUG 1


hadaq::CombinerModule::CombinerModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fCfg(),
   fOut(),
   fFlushCounter(0),
   fWithObserver(false),
   fEpicsSlave(false),
   fIsTerminating(false),
   fRunToOracle(false),
   fCheckTag(true),
   fFlushTimeout(0.),
   fBnetFileCmd(),
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

   fBNETsend = Cfg("BNETsend", cmd).AsBool(false);
   fBNETrecv = Cfg("BNETrecv", cmd).AsBool(false);
   fBNETbunch = Cfg("EB_EVENTS", cmd).AsInt(16);

   fExtraDebug = Cfg("ExtraDebug", cmd).AsBool(true);

   fCheckTag = Cfg("CheckTag", cmd).AsBool(true);

   fSkipEmpty = Cfg("SkipEmpty", cmd).AsBool(true);

   fEpicsRunNumber = 0;

   fLastTrigNr = 0xffffffff;
   fMaxHadaqTrigger = 0;
   fTriggerRangeMask = 0;

   fWithObserver = Cfg(hadaq::xmlObserverEnabled, cmd).AsBool(false);
   fEpicsSlave = Cfg(hadaq::xmlExternalRunid, cmd).AsBool(false);

   if(fEpicsSlave || fBNETrecv || fBNETsend) fRunNumber = 0; // ignore data without valid run id at beginning!
   else fRunNumber = hadaq::CreateRunId(); // runid from configuration time.

   fMaxHadaqTrigger = Cfg(hadaq::xmlHadaqTrignumRange, cmd).AsUInt(0x1000000);
   fTriggerRangeMask = fMaxHadaqTrigger-1;
   DOUT1("HADAQ %s module using maxtrigger 0x%x, rangemask:0x%x", GetName(), fMaxHadaqTrigger, fTriggerRangeMask);
   fEvnumDiffStatistics = Cfg(hadaq::xmlHadaqDiffEventStats, cmd).AsBool(true);

   fTriggerNrTolerance = Cfg(hadaq::xmlHadaqTriggerTollerance, cmd).AsInt(-1);
   if (fTriggerNrTolerance == -1) fTriggerNrTolerance = fMaxHadaqTrigger / 4;
   fEventBuildTimeout = Cfg(hadaq::xmlEvtbuildTimeout, cmd).AsDouble(20.0); // 20 seconds configure this optionally from xml later
   fHadesTriggerType = Cfg(hadaq::xmlHadesTriggerType, cmd).AsBool(false);
   fHadesTriggerHUB = Cfg(hadaq::xmlHadesTriggerHUB, cmd).AsUInt(0x8800);

   std::string ratesprefix = "Hadaq";

   for (unsigned n = 0; n < NumInputs(); n++) {
      fCfg.push_back(InputCfg());
      fCfg[n].Reset(true);
      fCfg[n].fResort = FindPort(InputName(n)).Cfg("resort").AsBool(false);
      if (fCfg[n].fResort) DOUT0("Do resort on input %u",n);
   }

   fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);

   // provide timeout with period/2, but trigger flushing after 3 counts
   // this will lead to effective flush time between FlushTimeout and FlushTimeout*1.5
   if (fFlushTimeout > 0.)
      CreateTimer("FlushTimer", fFlushTimeout/2.);

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

   if (fBNETrecv) {
      CreatePar("RunFileSize").SetUnits("MB").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);
      CreatePar("LtsmFileSize").SetUnits("MB").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);
      CreateCmdDef("BnetFileControl").SetField("_hidden", true);
   } else if (fBNETsend) {
      CreateCmdDef("BnetCalibrControl").SetField("_hidden", true);
   } else {
      CreateCmdDef("StartHldFile")
         .AddArg("filename", "string", true, "file.hld")
         .AddArg(dabc::xml_maxsize, "int", false, 1500)
         .SetArgMinMax(dabc::xml_maxsize, 1, 5000);
      CreateCmdDef("StopHldFile");
      CreateCmdDef("RestartHldFile");
   }

   CreatePar(fInfoName, "info").SetSynchron(true, 2., false).SetDebugLevel(2);

   if (IsName("Combiner"))
      PublishPars("$CONTEXT$/HadaqCombiner");
   else
      PublishPars(dabc::format("$CONTEXT$/%s", GetName()));

   fWorkerHierarchy.SetField("_player", "DABC.HadaqDAQControl");

   if (fBNETsend) fWorkerHierarchy.SetField("_bnet", "sender");
   if (fBNETrecv) fWorkerHierarchy.SetField("_bnet", "receiver");

   if (fBNETsend || fBNETrecv) {
      CreateTimer("BnetTimer", 1.); // check BNET values
      dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("State");
      item.SetField(dabc::prop_kind, "Text");
      item.SetField("value", "Init");
   }

   if (fWithObserver) {
      CreateTimer("ObserverTimer", 0.2); // export timers 5 times a second
      RegisterExportedCounters();
   }

   fNumReadBuffers = 0;
}


hadaq::CombinerModule::~CombinerModule()
{
   DOUT3("hadaq::CombinerModule::DTOR..does nothing now!.");
   //fOut.Close().Release();
   //fCfg.clear();
}

void hadaq::CombinerModule::ModuleCleanup()
{
   DOUT0("hadaq::CombinerModule::ModuleCleanup()");
   fIsTerminating = true;
   StoreRunInfoStop(true); // run info with exit mode
   fOut.Close().Release();

   for (unsigned n=0;n<fCfg.size();n++)
      fCfg[n].Reset();

   DOUT5("hadaq::CombinerModule::ModuleCleanup() after  fCfg[n].Reset()");

//   DOUT0("First %06x Last %06x Num %u Time %5.2f", firstsync, lastsync, numsync, tm2-tm1);
//   if (numsync>0)
//      DOUT0("Step %5.2f rate %5.2f sync/s", (lastsync-firstsync + 0.) / numsync, (numsync + 0.) / (tm2-tm1));
}


void hadaq::CombinerModule::SetInfo(const std::string &info, bool forceinfo)
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

   if (TimerName(timer) == "BnetTimer") {
      UpdateBnetInfo();
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
         "HADAQ %s starts. Runid:%d, numinp:%u, numout:%u flush:%3.1f",
         GetName(), (int) fRunNumber, NumInputs(), NumOutputs(), fFlushTimeout);

   SetInfo(info, true);
   DOUT0(info.c_str());
   fLastDropTm.GetNow();

   fLastProcTm.GetNow();
   fLastBuildTm.GetNow();

   // direct addon pointers can be used for terminal printout
   if (!fBNETrecv)
      for (unsigned n=0;n<fCfg.size();n++) {
         dabc::Command cmd("GetHadaqTransportInfo");
         cmd.SetInt("id", n);
         SubmitCommandToTransport(InputName(n), Assign(cmd));
      }
}

void hadaq::CombinerModule::AfterModuleStop()
{
   std::string info = dabc::format(
      "HADAQ %s stopped. CompleteEvents:%d, BrokenEvents:%d, DroppedData:%d, RecvBytes:%d, data errors:%d, tag errors:%d",
       GetName(), (int) fTotalBuildEvents, (int) fTotalDiscEvents , (int) fTotalDroppedData, (int) fTotalRecvBytes ,(int) fTotalDataErrors ,(int) fTotalTagErrors);

   SetInfo(info, true);
   DOUT0(info.c_str());

   // when BNET receiver module stopped, lead to application stop
   if (fBNETrecv) dabc::mgr.StopApplication();
}


bool hadaq::CombinerModule::FlushOutputBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) {
      DOUT3("FlushOutputBuffer has no buffer to flush");
      return false;
   }

   int dest = DestinationPort(fLastTrigNr);
   if (dest<0) {
      if (!CanSendToAllOutputs()) return false;
   } else {
      if (!CanSend(dest)) return false;
   }

   dabc::Buffer buf = fOut.Close();

   // if (fBNETsend) DOUT0("%s FLUSH buffer", GetName());

   if (dest<0)
      SendToAllOutputs(buf);
   else
      Send(dest, buf);

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

void hadaq::CombinerModule::UpdateBnetInfo()
{
   if (fBNETrecv) {

      if (!fBnetFileCmd.null() && fBnetFileCmd.IsTimedout()) fBnetFileCmd.Reply(dabc::cmd_false);

      dabc::Command cmd("GetTransportStatistic");
      if ((NumOutputs() < 2) || !SubmitCommandToTransport(OutputName(1), Assign(cmd))) {
         fWorkerHierarchy.SetField("runid", 0);
         fWorkerHierarchy.SetField("runsize", 0);
         fWorkerHierarchy.SetField("runname", std::string());
         fWorkerHierarchy.SetField("runprefix", std::string());
         fWorkerHierarchy.SetField("state", "NoFile");
         fWorkerHierarchy.GetHChild("State").SetField("value", "NoFile");
         fWorkerHierarchy.SetField("quality", 0.5); // not very bad - just inform that file not written
      }

      dabc::Command cmd2("GetTransportStatistic");
      cmd2.SetBool("#ltsm", true);
      if ((NumOutputs() < 3) || !SubmitCommandToTransport(OutputName(2), Assign(cmd2))) {
         fWorkerHierarchy.SetField("ltsmid", 0);
         fWorkerHierarchy.SetField("ltsmsize", 0);
         fWorkerHierarchy.SetField("ltsmname", std::string());
         fWorkerHierarchy.SetField("ltsmrefix", std::string());
         Par("LtsmFileSize").SetValue(0.);
      }

      dabc::Command cmd3("GetTransportStatistic");
      cmd3.SetBool("#mbs", true);
      if ((NumOutputs() < 1) || !SubmitCommandToTransport(OutputName(0), Assign(cmd3))) {
         fWorkerHierarchy.SetField("mbsinfo", "");
      }

      std::vector<int64_t> qsz;
      for (unsigned n=0;n<NumInputs();++n)
         qsz.push_back(NumCanRecv(n));
      fWorkerHierarchy.SetField("queues", qsz);
      fWorkerHierarchy.SetField("ninputs", NumInputs());
   }

   if (fBNETsend) {
      std::string nodestate = "";
      double node_quality = 1;
      int node_progress = 0;

      std::vector<uint64_t> hubs, ports, hubs_progress;
      std::vector<std::string> calibr, hubs_state, hubs_info;
      std::vector<double> hubs_quality;
      for (unsigned n=0;n<fCfg.size();n++) {
         InputCfg &inp = fCfg[n];

         hubs.push_back(inp.fHubId);
         ports.push_back(inp.fUdpPort);
         calibr.push_back(inp.fCalibr);

         if (!inp.fCalibrReq && !inp.fCalibr.empty()) {
            dabc::Command cmd("GetCalibrState");
            cmd.SetInt("indx",n);
            cmd.SetReceiver(inp.fCalibr);
            dabc::mgr.Submit(Assign(cmd));
            fCfg[n].fCalibrReq = true;
         }

         std::string state = "", sinfo = "";
         hadaq::TransportInfo *info = (hadaq::TransportInfo*) inp.fInfo;
         double rate = 0., hub_quality = 1;
         int hub_progress = 0;

         if (!info) {
            sinfo = "missing transport-info";
         } else {
            rate = (info->fTotalRecvBytes - inp.fHubLastSize)/1024.0/1024.0;
            inp.fHubSizeTmCnt++;

            // last 10 seconds calculate rate with previous value
            if ((rate==0.) && (inp.fHubSizeTmCnt<=10))
               rate = (info->fTotalRecvBytes - inp.fHubPrevSize)/1024.0/1024.0/inp.fHubSizeTmCnt;

            if (inp.fHubLastSize != info->fTotalRecvBytes) {
               inp.fHubSizeTmCnt = 0;
               inp.fHubPrevSize = inp.fHubLastSize;
            }

            inp.fHubLastSize = info->fTotalRecvBytes;
            sinfo = dabc::format("port:%d %5.3f MB/s data:%s pkts:%s buf:%s disc:%s d32:%s drop:%s lost:%s  ",
                       info->fNPort,
                       rate,
                       dabc::size_to_str(info->fTotalRecvBytes).c_str(),
                       dabc::number_to_str(info->fTotalRecvPacket,1).c_str(),
                       dabc::number_to_str(info->fTotalProducedBuffers).c_str(),
                       dabc::number_to_str(info->fTotalDiscardPacket).c_str(),
                       dabc::number_to_str(info->fTotalDiscard32Packet).c_str(),
                       dabc::number_to_str(inp.fDroppedTrig,0).c_str(),
                       dabc::number_to_str(inp.fLostTrig,0).c_str());

            sinfo += inp.TriggerRingAsStr(16);
         }

         if (rate<=0) { state = "NoData"; hub_quality = 0.1; } else
         if (!inp.fCalibr.empty() && (inp.fCalibrQuality < hub_quality)) {
            state = inp.fCalibrState;
            hub_quality = inp.fCalibrQuality;
            hub_progress = inp.fCalibrProgr;
         }

         if ((hub_progress>0) && ((node_progress==0) || (hub_progress<node_progress)))
            node_progress = hub_progress;

         if (hub_quality < node_quality) {
            node_quality = hub_quality;
            nodestate = state;
         }

         hubs_state.push_back(state);
         hubs_info.push_back(sinfo);
         hubs_quality.push_back(hub_quality);
         hubs_progress.push_back(inp.fCalibrProgr);
      }

      std::vector<int64_t> qsz;
      for (unsigned n=0;n<NumOutputs();++n)
         qsz.push_back(NumCanSend(n));

      if (nodestate.empty()) {
         nodestate = "Ready";
         node_quality = 1.;
      }

      fWorkerHierarchy.SetField("hubs", hubs);
      fWorkerHierarchy.SetField("hubs_info", hubs_info);
      fWorkerHierarchy.SetField("ports", ports);
      fWorkerHierarchy.SetField("calibr", calibr);
      fWorkerHierarchy.SetField("state", nodestate);
      fWorkerHierarchy.SetField("quality", node_quality);
      fWorkerHierarchy.SetField("progress", node_progress);
      fWorkerHierarchy.SetField("nbuilders", NumOutputs());
      fWorkerHierarchy.SetField("queues", qsz);
      fWorkerHierarchy.SetField("hubs_state", hubs_state);
      fWorkerHierarchy.SetField("hubs_quality", hubs_quality);
      fWorkerHierarchy.SetField("hubs_progress", hubs_progress);

      fWorkerHierarchy.GetHChild("State").SetField("value", nodestate);
   }
}

bool hadaq::CombinerModule::UpdateExportedCounters()
{
   if(!fWithObserver) return false;
   if(fIsTerminating) return false; // JAM2017: suppress warnings on termination
   // written bytes is updated by hldoutput directly, required by epics control of file size limit
   //SetEvtbuildPar("bytesWritten", fTotalRecvBytes);

   // runid is generated by epics, also refresh it here!
   if(fEpicsSlave)  {

      fEpicsRunNumber = GetEvtbuildParValue("runId");
      //std::cout <<"!!!!!! UpdateExportedCounters() got epics run id:"<< fEpicsRunNumber<< std::endl;

      if((fEpicsRunNumber!=0) && (fEpicsRunNumber!=fRunNumber)) {
        // run number 0 can occur when master eventbuilder terminates
        // ignore this case, since it may store wrong run info stop time
         DOUT1("Combiner in EPICS slave mode found new RUN ID %d (previous=%d)!",fEpicsRunNumber, fRunNumber);
         StoreRunInfoStop(false, fEpicsRunNumber);
         fRunNumber = fEpicsRunNumber;

         ResetInfoCounters();

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
   double drate = Par(fDataRateName).Value().AsDouble();
   double erate = Par(fEventRateName).Value().AsDouble();
   double losterate = Par(fLostEventRateName).Value().AsDouble();
   double dropdrate = Par(fDataDroppedRateName).Value().AsDouble();
   static double oldlostrate = 0;
   /////////////////
   // lostrate of 1 is expected since we still have
   // wrong overflow behaviour for triggernumber.
   // so only logfile warnings beyond 1 ->
   if((losterate>1 || dropdrate>0) && (losterate!=oldlostrate)) {
      oldlostrate = losterate;
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

   InputCfg& cfg = fCfg[ninp];

   ReadIterator& iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

   iter.Close();

   dabc::Buffer buf;

   if (cfg.fResortIndx < 0) {
      // normal way to take next buffer
      if(!CanRecv(ninp)) return false;
      buf = Recv(ninp);
      fNumReadBuffers++;
   } else {
      // do not try to look further than one more buffer
      if (cfg.fResortIndx>1) return false;
      // when doing resort, try to access buffers from the input queue
      buf = RecvQueueItem(ninp, cfg.fResortIndx++);
   }

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      // Stop();
      return false;
   }

   return iter.Reset(buf);
}

bool hadaq::CombinerModule::ShiftToNextHadTu(unsigned ninp)
{
   InputCfg& cfg = fCfg[ninp];
   ReadIterator& iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

   while (true) {

      bool res = false;
      if (iter.IsData())
         res = iter.NextSubeventsBlock();

      if (res && iter.IsData()) return true;

      if(!ShiftToNextBuffer(ninp)) return false;

      // DOUT0("Inp%u next buffer distance %u", ninp, iter.OnlyDebug());
   } //  while (!foundhadtu)

   return false;
}


int hadaq::CombinerModule::CalcTrigNumDiff(const uint32_t& prev, const uint32_t& next)
{
   int res = (int) (next) - prev;
   if (res > (int) fMaxHadaqTrigger/2) res -= fMaxHadaqTrigger; else
   if (res < (int) fMaxHadaqTrigger/-2) res += fMaxHadaqTrigger;
   return res;
}

bool hadaq::CombinerModule::ShiftToNextEvent(unsigned ninp, bool fast, bool dropped)
{
   // function used to shift to next event - used in BNET builder mode

   InputCfg& cfg = fCfg[ninp];

   if (dropped && cfg.has_data) cfg.fDroppedTrig++;

   cfg.Reset(fast);

   ReadIterator& iter = cfg.fIter;

   if (!iter.NextEvent())
      // retry in next hadtu container
      if (!ShiftToNextHadTu(ninp)) return false;

   // no need to analyze data
   if (fast) return true;

   // this is selected event
   cfg.evnt = iter.evnt();
   cfg.has_data = true;
   cfg.data_size = cfg.evnt->AllSubeventsSize();

   uint32_t seq = cfg.evnt->GetSeqNr();

   cfg.fTrigNr = (seq >> 8) & fTriggerRangeMask;
   cfg.fTrigTag = seq & 0xFF;

   cfg.fTrigNumRing[cfg.fRingCnt] = cfg.fTrigNr;
   cfg.fRingCnt = (cfg.fRingCnt+1) % HADAQ_RINGSIZE;

   cfg.fEmpty = (cfg.data_size == 0);
   cfg.fDataError = cfg.evnt->GetDataError();

   cfg.fTrigType = cfg.evnt->GetId() & 0xF;

   // int diff = CalcTrigNumDiff(cfg.fLastTrigNr,cfg.fTrigNr);
   // if (diff != 1)
   //   DOUT0("Inp%u Diff%d %x %x distance: %u", ninp, diff, cfg.fLastTrigNr, cfg.fTrigNr, iter.OnlyDebug());

   cfg.fLastTrigNr = cfg.fTrigNr;

   return true;
}


bool hadaq::CombinerModule::ShiftToNextSubEvent(unsigned ninp, bool fast, bool dropped)
{
   if (fBNETrecv) return ShiftToNextEvent(ninp, fast, dropped);

   DOUT5("CombinerModule::ShiftToNextSubEvent %d ", ninp);

   InputCfg& cfg = fCfg[ninp];

#ifdef HADAQ_DEBUG
   if (dropped && cfg.has_data)
      fprintf(stderr, "Input%u Trig:%6x Tag:%2x DROP\n", ninp, cfg.fTrigNr, cfg.fTrigTag);
#endif


   bool foundevent(false), doshift(true), tryresort(cfg.fResort);

   if (cfg.fResortIndx >= 0) {
      doshift = false; // do not shift event in main iterator
      if (cfg.subevnt) cfg.subevnt->SetTrigNr(0xffffffff); // mark subevent as used
      cfg.fResortIndx = -1;
      cfg.fResortIter.Close();
   } else {
      // account when subevent exists but intentionally dropped
      if (dropped && cfg.has_data) cfg.fDroppedTrig++;
   }

   cfg.Reset(fast);

   // if (fast) DOUT0("FAST DROP on inp %d", ninp);

   while (!foundevent) {
      ReadIterator& iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

      bool res = true;
      if (doshift) res = iter.NextSubEvent();
      doshift = true;

      if (!res || (iter.subevnt() == 0)) {
         DOUT5("CombinerModule::ShiftToNextSubEvent %d with zero NextSubEvent()", ninp);

         // retry in next hadtu container
         if (ShiftToNextHadTu(ninp)) continue;

         if ((cfg.fResortIndx>=0) && (NumCanRecv(ninp) > 1)) {
            // we have at least 2 buffers in the queue and cannot find required subevent
            // seems to be, we should use next event from normal queue
            cfg.fResortIndx = -1;
            cfg.fResortIter.Close();
            doshift = false;
            tryresort = false;
            continue;
         }

         // no more input buffers available
         return false;
      }

      // no need to analyze data
      if (fast) return true;

      if (tryresort && (cfg.fLastTrigNr!=0xffffffff)) {
         uint32_t trignr = iter.subevnt()->GetTrigNr();
         if (trignr==0xffffffff) continue; // this is processed trigger, exclude it

         int diff = CalcTrigNumDiff(cfg.fLastTrigNr, (trignr >> 8) & fTriggerRangeMask);

         if (diff!=1) {

            if (cfg.fResortIndx < 0) {
               cfg.fResortIndx = 0;
               cfg.fResortIter = cfg.fIter;
            }
            continue;
         }
      }

      foundevent = true;

      // this is selected subevent
      cfg.subevnt = iter.subevnt();
      cfg.has_data = true;
      cfg.data_size = cfg.subevnt->GetPaddedSize();

      cfg.fTrigNr = (cfg.subevnt->GetTrigNr() >> 8) & fTriggerRangeMask;
      cfg.fTrigTag = cfg.subevnt->GetTrigNr() & 0xFF;

#ifdef HADAQ_DEBUG
      fprintf(stderr, "Input%u Trig:%6x Tag:%2x\n", ninp, cfg.fTrigNr, cfg.fTrigTag);
#endif

      cfg.fTrigNumRing[cfg.fRingCnt] = cfg.fTrigNr;
      cfg.fRingCnt = (cfg.fRingCnt+1) % HADAQ_RINGSIZE;

      cfg.fEmpty = cfg.subevnt->GetSize() <= sizeof(hadaq::RawSubevent);
      cfg.fDataError = cfg.subevnt->GetDataError();

      cfg.fHubId = cfg.subevnt->GetId() & 0xffff;

      /* Evaluate trigger type:*/
      /* NEW for trb3: trigger type is part of decoding word*/
      if (!fHadesTriggerType) {
         cfg.fTrigType = cfg.subevnt->GetTrigTypeTrb3();
      } else if (cfg.fHubId == fHadesTriggerHUB) {
         unsigned wordNr = 2;
         uint32_t bitmask = 0xff000000; /* extended mask to contain spill on/off bit*/
         uint32_t bitshift = 24;
         // above from args.c defaults
         uint32_t val = cfg.subevnt->Data(wordNr - 1);
         cfg.fTrigType = (val & bitmask) >> bitshift;
         //DOUT0("Inp:%u use trb2 trigger type 0x%x", ninp, cfg.fTrigType);
      } else {
         cfg.fTrigType = 0;
      }

#ifndef HADERRBITDEBUG
      cfg.fErrorBits = cfg.subevnt->GetErrBits();
#else
      cfg.fErrorBits = ninp;
#endif
      int diff = 1;
      if (cfg.fLastTrigNr != 0xffffffff)
         diff = CalcTrigNumDiff(cfg.fLastTrigNr, cfg.fTrigNr);
      cfg.fLastTrigNr = cfg.fTrigNr;

      if (diff>1) cfg.fLostTrig += (diff-1);
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
         if (fCfg[ninp].has_data) numsubev++;
         droppeddata += fCfg[ninp].data_size;
      } while (ShiftToNextSubEvent(ninp, true, true));

      if (numsubev>maxnumsubev) maxnumsubev = numsubev;

      fCfg[ninp].Reset();
      fCfg[ninp].Close();
      while (SkipInputBuffers(ninp, 100)); // drop input port queue buffers until no more there
   }

   Par(fLostEventRateName).SetValue(maxnumsubev);
   Par(fDataDroppedRateName).SetValue(droppeddata/1024./1024.);
   fTotalDiscEvents += maxnumsubev;
   fTotalDroppedData += droppeddata;

   return true;
}

int hadaq::CombinerModule::DestinationPort(uint32_t trignr)
{
   if (!fBNETsend || (NumOutputs()<2)) return -1;

   return (trignr/fBNETbunch) % NumOutputs();
}

bool hadaq::CombinerModule::CheckDestination(uint32_t trignr)
{
   if (!fBNETsend || (fLastTrigNr==0xffffffff)) return true;

   return DestinationPort(fLastTrigNr) == DestinationPort(trignr);
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
      if (!fCfg[ninp].has_data)
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
     if (((fTriggerNrTolerance > 0) && (diff > fTriggerNrTolerance)) || ((fEventBuildTimeout>0) && fLastBuildTm.Expired(fEventBuildTimeout) && any_data && (fCfg.size()>1))) {

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

#ifdef HADAQ_DEBUG
      fprintf(stderr, "DROP ALL\n");
#endif

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

            if (!isempty || !fSkipEmpty) {
               // check also trigtag:
               if (trigtag != buildtag) tagError = true;
               if (haserror) dataError = true;
               subeventssize += fCfg[ninp].data_size;
            }
            foundsubevent = true;
            break;

         } else
         if (CalcTrigNumDiff(trignr, buildevid) > 0) {

            int droppedsize = fCfg[ninp].data_size;

#ifdef HADAQ_DEBUG
      fprintf(stderr, "Input%u TrigNr:%6x Skip while building %6x diff %u\n", ninp, trignr, buildevid, CalcTrigNumDiff(trignr, buildevid));
#endif

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

   if (fBNETsend)
      sequencenumber = (fCfg[masterchannel].fTrigNr << 8) | fCfg[masterchannel].fTrigTag;

   if (hasCompleteEvent && fCheckTag && tagError) {
      hasCompleteEvent = false;

      if (fBNETrecv) DOUT0("TAG error");

      fTotalTagErrors++;
   }

   // provide normal buffer

   if (hasCompleteEvent) {
      if (fOut.IsBuffer() && (!fOut.IsPlaceForEvent(subeventssize) || !CheckDestination(buildevid))) {
         // first we close current buffer
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
      if (dataError) fTotalDataErrors++;
      if (tagError) fTotalTagErrors++;

      unsigned trigtyp = 0;
      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
         trigtyp = fCfg[ninp].fTrigType;
         if (trigtyp) break;
      }

      // here event id, always from "cts master channel" 0
      unsigned currentid = trigtyp | (2 << 12); // DAQVERSION=2 for dabc
      //fEventIdCount[currentid & (HADAQ_NEVTIDS - 1)]++;
      fEventIdCount[currentid & 0xF]++; // JAM: problem with spill bit?
      fOut.evnt()->SetId(currentid & (HADAQ_NEVTIDS_IN_FILE - 1));

      // third input loop: build output event from all not empty subevents
      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
         if (fCfg[ninp].fEmpty && fSkipEmpty) continue;
         if (fBNETrecv)
            fOut.AddAllSubevents(fCfg[ninp].evnt);
         else
            fOut.AddSubevent(fCfg[ninp].subevnt);
         DoInputSnapshot(ninp); // record current state of event tag and queue level for control system
      } // for ninp

      fOut.FinishEvent();

      int diff = 1;
      if (fLastTrigNr!=0xffffffff) diff = CalcTrigNumDiff(fLastTrigNr, buildevid);

      //if (fBNETsend && (diff!=1))
      //   DOUT0("%s %x %x %d", GetName(), fLastTrigNr, buildevid, diff);
      // if (fBNETsend) DOUT0("%s trig %x size %u", GetName(), buildevid, subeventssize);

#ifdef HADAQ_DEBUG
      fprintf(stderr, "BUILD:%6x\n", buildevid);
#endif

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
   bool do_start = false, do_stop = false;

   if (cmd.IsName("StartHldFile")) {
      do_start = do_stop = true;
      SetInfo("Execute StartHldFile");

      DOUT0("******************* START HLD FILE *************");
   } else if (cmd.IsName("StopHldFile")) {
      do_stop = true;
      SetInfo("Execute StopHldFile");
      DOUT0("******************* STOP HLD FILE *************");

   } else if (cmd.IsName("RestartHldFile")) {
      if (NumOutputs()<2) return dabc::cmd_false;
      SetInfo("Execute RestartHldFile");
      cmd.ChangeName("RestartTransport");
      SubmitCommandToTransport(OutputName(1), cmd);
      return dabc::cmd_postponed;
   } else if (cmd.IsName("BnetFileControl")) {
      if (NumOutputs()<2) return dabc::cmd_false;
      if (!fBnetFileCmd.null()) fBnetFileCmd.Reply(dabc::cmd_false);

      std::string mode = cmd.GetStr("mode");

      if (mode == "start") {
         SetInfo("Execute BnetFileControl");
         for (unsigned k=1;k<NumOutputs();++k) {
            dabc::Command subcmd("RestartTransport");
            subcmd.SetBool("only_prefix", true);
            subcmd.SetStr("prefix", cmd.GetStr("prefix"));
            SubmitCommandToTransport(OutputName(k), Assign(subcmd));
         }
         fBnetFileCmd = cmd;
         fBnetFileCmd.SetInt("#replies", NumOutputs()-1);
         if (!fBnetFileCmd.IsTimeoutSet()) fBnetFileCmd.SetTimeout(30);
         return dabc::cmd_postponed;
      }

      if (mode == "stop") {
         if (fRunNumber) StoreRunInfoStop();
         // reset runid
         fRunNumber = 0;
      }

      return dabc::cmd_true;

   } else if (cmd.IsName("HCMD_DropAllBuffers")) {

      DropAllInputBuffers();
      fTotalFullDrops++;
      fLastDropTm.GetNow();

      if (fBNETsend && !fIsTerminating) {
         for (unsigned n = 0; n < NumInputs(); n++) {
            fCfg[n].fDroppedTrig = 0;
            fCfg[n].fLostTrig = 0;
            dabc::Command subcmd("ResetTransportStat");
            SubmitCommandToTransport(InputName(n), subcmd);
         }
      }

      cmd.SetStrRawData("true");
      return dabc::cmd_true;

   } else if (cmd.IsName("BnetCalibrControl")) {

      std::string rundir = hadaq::FormatFilename(cmd.GetUInt("runid"));

      DOUT0("Combiner get BnetCalibrControl mode %s rundir %s", cmd.GetStr("mode"), rundir.c_str());

      if (fBNETsend && !fIsTerminating)
         for (unsigned n = 0; n < NumInputs(); n++) {
            dabc::Command subcmd("TdcCalibrations");
            subcmd.SetStr("mode", cmd.GetStr("mode"));
            subcmd.SetStr("rundir", rundir);
            SubmitCommandToTransport(InputName(n), subcmd);
         }

      return dabc::cmd_true;
   } else {
      return dabc::ModuleAsync::ExecuteCommand(cmd);
   }

   bool res = true;

   if (do_stop) {
      if (NumOutputs()>1)
         res = DisconnectPort(OutputName(1));

      DOUT0("Stop HLD file res = %s", DBOOL(res));
   }

   if (do_start && res) {
      std::string fname = cmd.GetStr("filename", "file.hld");
      int maxsize = cmd.GetInt(dabc::xml_maxsize, 1500);

      std::string url = dabc::format("hld://%s?%s=%d", fname.c_str(), dabc::xml_maxsize, maxsize);

      // we guarantee, that at least two ports will be created
      EnsurePorts(0, 2);

      res = dabc::mgr.CreateTransport(OutputName(1, true), url);

      DOUT0("Start HLD file %s res = %s", fname.c_str(), DBOOL(res));
   }

   return cmd_bool(res);
}


std::string  hadaq::CombinerModule::GetEvtbuildParName(const std::string &name)
{
   return dabc::format("%s-%s",hadaq::EvtbuildPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateEvtbuildPar(const std::string &name)
{
   CreatePar(GetEvtbuildParName(name)).SetSynchron(true, 0.2);
}

void hadaq::CombinerModule::SetEvtbuildPar(const std::string &name, unsigned value)
{
    Par(GetEvtbuildParName(name)).SetValue(value);
}

unsigned hadaq::CombinerModule::GetEvtbuildParValue(const std::string &name)
{
   return Par(GetEvtbuildParName(name)).Value().AsUInt();
}

std::string  hadaq::CombinerModule::GetNetmemParName(const std::string &name)
{
   return dabc::format("%s-%s",hadaq::NetmemPrefix,name.c_str());
}

void hadaq::CombinerModule::CreateNetmemPar(const std::string &name)
{
   CreatePar(GetNetmemParName(name)).SetSynchron(true, 0.2);
}

void hadaq::CombinerModule::SetNetmemPar(const std::string &name, unsigned value)
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

void hadaq::CombinerModule::StoreRunInfoStop(bool onexit, unsigned newrunid)
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
   if(onexit || (newrunid==0))
      t = time(NULL);
   else
      t = newrunid + hadaq::HADAQ_TIMEOFFSET; // new run number defines stop time
   FILE *fp;
   char ltime[20];            /* local time */
   struct tm tm_res;
   strftime(ltime, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &tm_res));
   fp = fopen(fRunInfoToOraFilename.c_str(), "a+");
   std::string filename = GenerateFileName(fRunNumber); // old run number defines old filename
   fprintf(fp, "stop %u %d %s %s %s ", fRunNumber, fEBId, filename.c_str(), ltime, Unit(fTotalBuildEvents));
   fprintf(fp, "%s\n", Unit(fTotalRecvBytes));
   fclose(fp);
   DOUT1("Write run info to %s - stop: %lu %d %s %s %s %s", fRunInfoToOraFilename.c_str(), fRunNumber, fEBId, filename.c_str(), ltime, Unit(fTotalBuildEvents),Unit(fTotalRecvBytes));

}

void hadaq::CombinerModule::ResetInfoCounters()
{
   fTotalRecvBytes = 0;
   fTotalBuildEvents = 0;
   fTotalDiscEvents = 0;
   fTotalDroppedData = 0;
   fTotalTagErrors = 0;
   fTotalDataErrors = 0;

   if (!fBNETrecv && fWithObserver && !fIsTerminating)
      for (unsigned n = 0; n < NumInputs(); n++) {
         SubmitCommandToTransport(InputName(n), dabc::Command("ResetExportedCounters"));

         // JAM BUGFIX JUL14: errorbits not reset
         for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS; ++ptrn)
            fCfg[n].fErrorbitStats[ptrn] = 0;

         fCfg[n].fLastEvtBuildTrigId = 0;
      }

   for (int ptrn = 0; ptrn < HADAQ_NUMERRPATTS; ++ptrn)
      fErrorbitPattern[ptrn] = 0;

   for (unsigned i = 0; i < HADAQ_NEVTIDS; i++)
      fEventIdCount[i] = 0;
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
   return fPrefix + hadaq::FormatFilename(fRunNumber,fEBId) + std::string(".hld");
}

bool hadaq::CombinerModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetHadaqTransportInfo")) {
      unsigned id = cmd.GetUInt("id");
      if (id < fCfg.size()) {
         fCfg[id].fInfo = cmd.GetPtr("Info");
         fCfg[id].fUdpPort = cmd.GetUInt("UdpPort");
         fCfg[id].fCalibr = cmd.GetStr("CalibrModule");
      }
      return true;
   } else if (cmd.IsName("GetCalibrState")) {
      unsigned n = cmd.GetUInt("indx");
      if (n < fCfg.size()) {
         fCfg[n].fCalibrReq = false;
         // fCfg[n].trb = cmd.GetUInt("trb");
         // fCfg[n].tdcs = cmd.GetField("tdcs").AsUIntVect();
         fCfg[n].fCalibrProgr = cmd.GetInt("progress");
         fCfg[n].fCalibrState = cmd.GetStr("state");
         fCfg[n].fCalibrQuality = cmd.GetDouble("quality");
      }
      return true;
   } else if (cmd.IsName("GetTransportStatistic")) {
      if (cmd.GetBool("#mbs")) {
         fWorkerHierarchy.SetField("mbsinfo", cmd.GetStr("MbsInfo"));
         return true;
      }

      unsigned runid = cmd.GetUInt("RunId");
      std::string runname = cmd.GetStr("RunName");
      std::string runprefix = cmd.GetStr("RunPrefix");
      unsigned runsz = cmd.GetUInt("RunSize");

      if (cmd.GetBool("#ltsm")) {
         // this is LTSM info
         fWorkerHierarchy.SetField("ltsmid", runid);
         fWorkerHierarchy.SetField("ltsmsize", runsz);
         fWorkerHierarchy.SetField("ltsmname", runname);
         fWorkerHierarchy.SetField("ltsmprefix", runprefix);
         Par("LtsmFileSize").SetValue(runsz/1024./1024.);
         return true;
      }

      fWorkerHierarchy.SetField("runid", runid);
      fWorkerHierarchy.SetField("runsize", runsz);
      fWorkerHierarchy.SetField("runname", runname);
      fWorkerHierarchy.SetField("runprefix", runprefix);

      Par("RunFileSize").SetValue(runsz/1024./1024.);

      std::string state = "File";
      double quality = 0.98;
      if (Par(fEventRateName).Value().AsDouble() == 0) { state = "NoData"; quality = 0.1; } else
      if ((runid==0) && runname.empty()) { state = "NoFile"; quality = 0.5; }

      fWorkerHierarchy.SetField("state", state);
      fWorkerHierarchy.SetField("quality", quality);
      fWorkerHierarchy.GetHChild("State").SetField("value", state);

      return true;
   } else if (cmd.IsName("RestartTransport")) {
      int num = fBnetFileCmd.GetInt("#replies");
      if (num == 1) {
         unsigned newrunid = fBnetFileCmd.GetUInt("runid");
         if (fRunNumber) StoreRunInfoStop(false, newrunid);
         std::string newprefix = fBnetFileCmd.GetStr("prefix");
         if(!newprefix.empty()) fPrefix = newprefix; // need to reset prefix here for run info JAM2018
         fRunNumber = newrunid;
         ResetInfoCounters();
         StoreRunInfoStart();
         fBnetFileCmd.Reply(dabc::cmd_true);
      } else {
         fBnetFileCmd.SetInt("#replies", num-1);
      }
      return true;
   }

   return dabc::ModuleAsync::ReplyCommand(cmd);
}

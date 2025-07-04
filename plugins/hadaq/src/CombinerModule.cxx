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

#include <cmath>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

#include "dabc/Manager.h"

#include "hadaq/UdpTransport.h"

const unsigned kNoTrigger = 0xffffffff;


hadaq::CombinerModule::CombinerModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fCfg(),
   fOut(),
   fFlushCounter(0),
   fIsTerminating(false),
   fRunToOracle(false),
   fFlushTimeout(0.),
   fBnetFileCmd(),
   fEvnumDiffStatistics(true)
{
   EnsurePorts(0, 1, dabc::xmlWorkPool);

   fSpecialItemId = CreateUserItem("BuildEvents");
   fSpecialFired = false;
   fLastEventRate = 0.;
   fBldProfiler.Reserve(50);

   fRunRecvBytes = 0;
   fRunBuildEvents = 0;
   fRunDiscEvents = 0;
   fRunDroppedData = 0;
   fRunTagErrors = 0;
   fRunDataErrors = 0;

   fAllRecvBytes = 0;
   fAllBuildEvents = 0;
   fAllBuildEventsLimit = 0;
   fAllDiscEvents = 0;
   fAllDroppedData = 0;
   fAllFullDrops = 0;
   fMaxProcDist = 0;

   fEBId = Cfg("NodeId", cmd).AsInt(-1);
   if (fEBId < 0) fEBId = dabc::mgr.NodeId()+1; // hades eb ids start with 1

   fBNETsend = Cfg("BNETsend", cmd).AsBool(false);
   fBNETrecv = Cfg("BNETrecv", cmd).AsBool(false);
   fBNETbunch = Cfg("EB_EVENTS", cmd).AsInt(16);
   fBNETNumRecv = Cfg("BNET_NUMRECEIVERS", cmd).AsInt(1);
   fBNETNumSend = Cfg("BNET_NUMSENDERS", cmd).AsInt(1);

   fExtraDebug = Cfg("ExtraDebug", cmd).AsBool(true);

   fCheckTag = Cfg("CheckTag", cmd).AsBool(true);

   fSkipEmpty = Cfg("SkipEmpty", cmd).AsBool(true);

   fBNETCalibrDir = Cfg("CalibrDir", cmd).AsStr();
   fBNETCalibrPackScript = Cfg("CalibrPack", cmd).AsStr();

   fEpicsRunNumber = 0;

   fLastTrigNr = kNoTrigger;
   fMaxHadaqTrigger = 0;
   fTriggerRangeMask = 0;

   if (fBNETrecv || fBNETsend)
      fRunNumber = 0; // ignore data without valid run id at beginning!
   else
      fRunNumber = dabc::CreateHadaqRunId(); // runid from configuration time.

   fMaxHadaqTrigger = Cfg(hadaq::xmlHadaqTrignumRange, cmd).AsUInt(0x1000000);
   fTriggerRangeMask = fMaxHadaqTrigger-1;
   DOUT1("HADAQ %s module using maxtrigger 0x%x, rangemask:0x%x", GetName(), fMaxHadaqTrigger, fTriggerRangeMask);
   fEvnumDiffStatistics = Cfg(hadaq::xmlHadaqDiffEventStats, cmd).AsBool(true);

   fTriggerNrTolerance = Cfg(hadaq::xmlHadaqTriggerTollerance, cmd).AsInt(-1);
   if (fTriggerNrTolerance == -1) fTriggerNrTolerance = fMaxHadaqTrigger / 4;
   fEventBuildTimeout = Cfg(hadaq::xmlEvtbuildTimeout, cmd).AsDouble(20.0); // 20 seconds configure this optionally from xml later
   fAllBuildEventsLimit = Cfg(hadaq::xmlMaxNumBuildEvt, cmd).AsUInt(0);
   fHadesTriggerType = Cfg(hadaq::xmlHadesTriggerType, cmd).AsBool(false);
   fHadesTriggerHUB = Cfg(hadaq::xmlHadesTriggerHUB, cmd).AsUInt(0x8800);

   for (unsigned n = 0; n < NumInputs(); n++) {
      fCfg.emplace_back();
      fCfg[n].ninp = n;
      fCfg[n].Reset(true);
      fCfg[n].fResort = FindPort(InputName(n)).Cfg("resort").AsBool(false);
      if (fCfg[n].fResort)
         DOUT0("Do resort on input %u",n);
   }

   fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);

   // provide timeout with period/2, but trigger flushing after 3 counts
   // this will lead to effective flush time between FlushTimeout and FlushTimeout*1.5
   CreateTimer("FlushTimer", (fFlushTimeout > 0) ? fFlushTimeout/2. : 1.);

   //CreatePar("RunId");
   //Par("RunId").SetValue(fRunNumber); // to communicate with file components

   fRunInfoToOraFilename = dabc::format("eb_runinfo2ora_%d.txt",fEBId);
   // TODO: optionally set this name
   fPrefix = Cfg("FilePrefix", cmd).AsStr("no");
   fRunToOracle = Cfg("Runinfo2ora", cmd).AsBool(false);


   fDataRateName = "HadaqData";
   fEventRateName = "HadaqEvents";
   fLostEventRateName = "HadaqLostEvents";
   fDataDroppedRateName = "HadaqDroppedData";
   fInfoName = "HadaqInfo";
   fCheckBNETProblems = chkNone;
   fBNETProblem = "";

   CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar(fEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar(fLostEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar(fDataDroppedRateName).SetRatemeter(false, 3.).SetUnits("MB");

   fDataRateCnt = fEventRateCnt = fLostEventRateCnt = fDataDroppedRateCnt = 0;

   if (fBNETrecv) {
      CreatePar("RunFileSize").SetUnits("MB").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);
      CreatePar("LtsmFileSize").SetUnits("MB").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);
      CreateCmdDef("BnetFileControl").SetField("_hidden", true);
   } else if (fBNETsend) {
      CreateCmdDef("BnetCalibrControl").SetField("_hidden", true);
      CreateCmdDef("BnetCalibrRefresh").SetField("_hidden", true);
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
   if (fBNETrecv) {
      fWorkerHierarchy.SetField("_bnet", "receiver");
      fWorkerHierarchy.SetField("build_events", 0);
      fWorkerHierarchy.SetField("build_data", 0);
      fWorkerHierarchy.SetField("discard_events", 0);
   }

   if (fBNETsend || fBNETrecv) {
      CreateTimer("BnetTimer", 1.); // check BNET values
      dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("State");
      item.SetField(dabc::prop_kind, "Text");
      item.SetField("value", "Init");
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
   if (TimerName(timer) == "BnetTimer") {
      UpdateBnetInfo();
      return;
   }

   if ((fFlushTimeout > 0) && (++fFlushCounter > 2)) {
      fFlushCounter = 0;
      PROFILER_GURAD(fBldProfiler, "flush", 30)
      FlushOutputBuffer();
   }

   fTimerCalls++;

   Par(fDataRateName).SetValue(fDataRateCnt/1024./1024.);
   Par(fEventRateName).SetValue(fEventRateCnt);
   Par(fLostEventRateName).SetValue(fLostEventRateCnt);
   Par(fDataDroppedRateName).SetValue(fDataDroppedRateCnt/1024./1024.);

   fDataRateCnt = fEventRateCnt = fLostEventRateCnt = fDataDroppedRateCnt = 0;

   fLastEventRate = Par(fEventRateName).Value().AsDouble();

   // invoke event building, if necessary - reinjects events
   StartEventsBuilding();

   if ((fAllBuildEventsLimit > 0) && (fAllBuildEvents >= fAllBuildEventsLimit)) {
      FlushOutputBuffer();
      fAllBuildEventsLimit = 0; // invoke only once
      dabc::mgr.StopApplication();
   }
}

void hadaq::CombinerModule::StartEventsBuilding()
{
   int cnt = 10;
   if (fLastEventRate > 1000) cnt = 20;
   if (fLastEventRate > 30000) cnt = 50;

   while (IsRunning() && (cnt-- > 0)) {
      // no need to continue
      if (!BuildEvent()) return;
   }

   if (!fSpecialFired) {
      fSpecialFired = true;
      // DOUT0("Fire user event %d item %u", dabc::evntUser, fSpecialItemId);
      FireEvent(dabc::evntUser, fSpecialItemId);
   }
}

void hadaq::CombinerModule::ProcessUserEvent(unsigned item)
{
   if (fSpecialItemId == item) {
      // DOUT0("Get user event");
      fSpecialFired = false;
   } else {
      EOUT("Get wrong user event");
   }

   StartEventsBuilding();
}

void hadaq::CombinerModule::BeforeModuleStart()
{
   std::string info = dabc::format(
         "HADAQ %s starts. Runid:%d, numinp:%u, numout:%u flush:%3.1f",
         GetName(), (int) fRunNumber, NumInputs(), NumOutputs(), fFlushTimeout);

   SetInfo(info, true);
   DOUT0("%s", info.c_str());
   fLastDropTm.GetNow();

   fLastProcTm = fLastDropTm;
   fLastBuildTm = fLastDropTm;

   // activate BNET checks
   if (fBNETsend)
      fCheckBNETProblems = chkActive;

   // direct addon pointers can be used for terminal printout
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      if (fBNETrecv) continue;
      dabc::Command cmd("GetHadaqTransportInfo");
      cmd.SetInt("id", ninp);
      SubmitCommandToTransport(InputName(ninp), Assign(cmd));
   }
}

void hadaq::CombinerModule::AfterModuleStop()
{
   std::string info = dabc::format(
      "HADAQ %s stopped. CompleteEvents:%d, BrokenEvents:%d, DroppedData:%d, RecvBytes:%d, data errors:%d, tag errors:%d",
       GetName(), (int) fAllBuildEvents, (int) fAllDiscEvents , (int) fAllDroppedData, (int) fAllRecvBytes ,(int) fRunDataErrors ,(int) fRunTagErrors);

   SetInfo(info, true);
   DOUT0("%s", info.c_str());

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
   if (dest < 0) {
      if (!CanSendToAllOutputs()) return false;
   } else {
      if (!CanSend(dest)) return false;
   }

   dabc::Buffer buf = fOut.Close();

   // if (fBNETsend) DOUT0("%s FLUSH buffer", GetName());

   if (dest < 0)
      SendToAllOutputs(buf);
   else
      Send(dest, buf);

   fFlushCounter = 0; // indicate that next flush timeout one not need to send buffer

   return true;
}

void hadaq::CombinerModule::UpdateBnetInfo()
{
   fBldProfiler.MakeStatistic();

   PROFILER_GURAD(fBldProfiler, "info", 20)

   if (fBNETrecv) {

      if (!fBnetFileCmd.null() && fBnetFileCmd.IsTimedout())
         fBnetFileCmd.Reply(dabc::cmd_false);

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

      std::string info = "BnetRecv: ";
      std::vector<int64_t> qsz;
      for (unsigned n=0;n<NumInputs();++n) {
         unsigned len = NumCanRecv(n);
         info.append(" ");
         info.append(std::to_string(len));
         qsz.emplace_back(len);
      }
      fBnetInfo = info;

      fWorkerHierarchy.SetField("queues", qsz);
      fWorkerHierarchy.SetField("ninputs", NumInputs());
      fWorkerHierarchy.SetField("build_events", fAllBuildEvents);
      fWorkerHierarchy.SetField("build_data", fAllRecvBytes);
      fWorkerHierarchy.SetField("discard_events", fAllDiscEvents);
   }

   if (fBNETsend) {
      std::string node_state = "";
      double node_quality = 1;
      int node_progress = 0;

      std::vector<uint64_t> hubs, ports, hubs_progress, recv_sizes, recv_bufs, hubs_dropev, hubs_lostev;
      std::vector<std::string> calibr, hubs_state, hubs_info;
      std::vector<double> hubs_quality, hubs_rates;
      for (unsigned n=0;n<fCfg.size();n++) {
         InputCfg &inp = fCfg[n];

         hubs.emplace_back(inp.fHubId);
         ports.emplace_back(inp.fUdpPort);
         calibr.emplace_back(inp.fCalibr);

         unsigned nbuf = NumCanRecv(n);
         uint64_t bufsz = TotalSizeCanRecv(n);

         if (inp.fIter.IsData()) {
            nbuf++;
            bufsz += inp.fIter.remained_size();
         }

         recv_bufs.emplace_back(nbuf);
         recv_sizes.emplace_back(bufsz);

         if (!inp.fCalibrReq && !inp.fCalibr.empty()) {
            dabc::Command cmd("GetCalibrState");
            cmd.SetInt("indx",n);
            cmd.SetReceiver(inp.fCalibr);
            dabc::mgr.Submit(Assign(cmd));
            inp.fCalibrReq = true;
         }

         std::string hub_state = "", sinfo = "";
         hadaq::TransportInfo *info = (hadaq::TransportInfo *) inp.fInfo;
         double rate = 0., hub_quality = 1;
         int hub_progress = 100;

         if (!info) {
            sinfo = "missing transport-info";
         } else {
            inp.fHubSizeTmCnt++;

            if (info->fTotalRecvBytes > inp.fHubLastSize)
               rate = (info->fTotalRecvBytes - inp.fHubLastSize)/1024.0/1024.0;
            else if (inp.fHubSizeTmCnt <= 15)
               rate = (info->fTotalRecvBytes - inp.fHubPrevSize)/1024.0/1024.0/inp.fHubSizeTmCnt;

            if (inp.fHubLastSize != info->fTotalRecvBytes) {
               inp.fHubSizeTmCnt = 0;
               inp.fHubPrevSize = inp.fHubLastSize;
            } else if ((inp.fHubSizeTmCnt > 0.75*fEventBuildTimeout) && (hub_quality > 0.1)) {
               hub_state = "NoData";
               hub_quality = 0.1;
               hub_progress = 0;
            } else if ((inp.fHubSizeTmCnt > 7) && (hub_quality > 0.6)) {
               hub_state = "LowData";
               hub_quality = 0.6;
               hub_progress = 0;
            }

            inp.fHubLastSize = info->fTotalRecvBytes;
            sinfo = dabc::format("port:%d %5.3f MB/s data:%s pkts:%s buf:%s disc:%s d32:%s drop:%s lost:%s errbits:%s ",
                       info->fNPort,
                       rate,
                       dabc::size_to_str(info->fTotalRecvBytes).c_str(),
                       dabc::number_to_str(info->fTotalRecvPacket,1).c_str(),
                       dabc::number_to_str(info->fTotalProducedBuffers).c_str(),
                       info->GetDiscardString().c_str(),
                       info->GetDiscard32String().c_str(),
                       dabc::number_to_str(inp.fDroppedTrig,0).c_str(),
                       dabc::number_to_str(inp.fLostTrig,0).c_str(),
                       dabc::number_to_str(inp.fErrorBitsCnt,0).c_str());

            sinfo += inp.TriggerRingAsStr(16);
         }

         hubs_dropev.emplace_back(inp.fDroppedTrig);
         hubs_lostev.emplace_back(inp.fLostTrig);

         if (!inp.fCalibr.empty() && (inp.fCalibrQuality < hub_quality)) {
            hub_state = inp.fCalibrState;
            hub_quality = inp.fCalibrQuality;
            hub_progress = inp.fCalibrProgr;
         }

         if ((hub_progress > 0) && ((node_progress == 0) || (hub_progress < node_progress)))
            node_progress = hub_progress;

         if (hub_quality < node_quality) {
            node_quality = hub_quality;
            node_state = hub_state;
         }

         hubs_state.emplace_back(hub_state);
         hubs_info.emplace_back(sinfo);
         hubs_quality.emplace_back(hub_quality);
         hubs_progress.emplace_back(hub_progress);
         hubs_rates.emplace_back(rate);
      }

      std::string info = "BnetSend:";
      std::vector<int64_t> qsz;
      for (unsigned n=0;n<NumOutputs();++n) {
         unsigned len = NumCanSend(n);
         info.append(" ");
         info.append(std::to_string(len));
         qsz.emplace_back(len);
      }
      fBnetInfo = info;

      if (!fBNETProblem.empty() && (node_quality > 0.1)) {
         node_state = fBNETProblem;
         node_quality = 0.1;
      }

      if (node_state.empty()) {
         node_state = "Ready";
         node_quality = 1.;
         node_progress = 100;
      }

      fWorkerHierarchy.SetField("hubs", hubs);
      fWorkerHierarchy.SetField("hubs_info", hubs_info);
      fWorkerHierarchy.SetField("ports", ports);
      fWorkerHierarchy.SetField("calibr", calibr);
      fWorkerHierarchy.SetField("state", node_state);
      fWorkerHierarchy.SetField("quality", node_quality);
      fWorkerHierarchy.SetField("progress", node_progress);
      fWorkerHierarchy.SetField("nbuilders", NumOutputs());
      fWorkerHierarchy.SetField("queues", qsz);
      fWorkerHierarchy.SetField("hubs_dropev",hubs_dropev);
      fWorkerHierarchy.SetField("hubs_lostev",hubs_lostev);
      fWorkerHierarchy.SetField("hubs_state", hubs_state);
      fWorkerHierarchy.SetField("hubs_quality", hubs_quality);
      fWorkerHierarchy.SetField("hubs_progress", hubs_progress);
      fWorkerHierarchy.SetField("hubs_rates", hubs_rates);
      fWorkerHierarchy.SetField("recv_bufs", recv_bufs);
      fWorkerHierarchy.SetField("recv_sizes", recv_sizes);

      fWorkerHierarchy.GetHChild("State").SetField("value", node_state);
   }

   fBnetStat = fBldProfiler.Format();

   // fBnetStat = dabc::format("BldStat: calls:%ld inp:%ld out:%ld buf:%ld timer:%ld", fBldCalls, fInpCalls, fOutCalls, fBufCalls, fTimerCalls);

   fBldCalls = fInpCalls = fOutCalls = fBufCalls = fTimerCalls = 0;
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

   bool full_buffer_control = false;

   if (cfg.fResortIndx < 0) {
      // normal way to take next buffer
      if(!CanRecv(ninp))
         return false;
      buf = Recv(ninp);
      fNumReadBuffers++;
      full_buffer_control = true;
   } else {
      // do not try to look further than one more buffer
      if (cfg.fResortIndx > 1)
         return false;
      // when doing resort, try to access buffers from the input queue
      buf = RecvQueueItem(ninp, cfg.fResortIndx++);
   }

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      // Stop();
      return false;
   }

   return full_buffer_control ? iter.ResetOwner(buf) : iter.Reset(buf);
}

bool hadaq::CombinerModule::ShiftToNextHadTu(unsigned ninp)
{
   InputCfg &cfg = fCfg[ninp];
   ReadIterator &iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

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


int hadaq::CombinerModule::CalcTrigNumDiff(const uint32_t &prev, const uint32_t &next)
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

   // DOUT0("ninp %u Shift to event %x", ninp, cfg.fTrigNr);
   cfg.fLastTrigNr = cfg.fTrigNr;

   return true;
}


bool hadaq::CombinerModule::ShiftToNextSubEvent(unsigned ninp, bool fast, bool dropped)
{
   if (fBNETrecv) return ShiftToNextEvent(ninp, fast, dropped);

   DOUT5("CombinerModule::ShiftToNextSubEvent %d ", ninp);

   InputCfg &cfg = fCfg[ninp];

#ifdef HADAQ_DEBUG
   if (dropped && cfg.has_data)
      fprintf(stderr, "Input%u Trig:%6x Tag:%2x DROP\n", ninp, cfg.fTrigNr, cfg.fTrigTag);
#endif


   bool foundevent = false, doshift = true, tryresort = cfg.fResort;

   if (cfg.fResortIndx >= 0) {
      doshift = false; // do not shift event in main iterator
      if (cfg.subevnt) cfg.subevnt->SetTrigNr(kNoTrigger); // mark subevent as used
      cfg.fResortIndx = -1;
      cfg.fResortIter.Close();
   } else {
      // account when subevent exists but intentionally dropped
      if (dropped && cfg.has_data) cfg.fDroppedTrig++;
   }

   cfg.Reset(fast);

   // if (fast) DOUT0("FAST DROP on inp %d", ninp);

   while (!foundevent) {
      ReadIterator &iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

      bool res = true;
      if (doshift) res = iter.NextSubEvent();
      doshift = true;

      if (!res || !iter.subevnt()) {
         DOUT5("CombinerModule::ShiftToNextSubEvent %d with zero NextSubEvent()", ninp);

         // retry in next hadtu container
         if (ShiftToNextHadTu(ninp)) continue;

         if ((cfg.fResortIndx >= 0) && (NumCanRecv(ninp) > 1)) {
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
      if (fast)
         return true;

      bool ignore_resort = false;

      if (tryresort && (cfg.fLastTrigNr != kNoTrigger)) {
         uint32_t trignr = iter.subevnt()->GetTrigNr();

         if (trignr == kNoTrigger) continue; // this is processed trigger, exclude it

         trignr = (trignr >> 8) & fTriggerRangeMask;

         int diff = CalcTrigNumDiff(cfg.fLastTrigNr, trignr);

         uint32_t hubid = iter.subevnt()->GetId() & 0xffff;

         // hardcode MDC range
         bool is_mdc = fHadesTriggerType && (hubid >= 0x1100) && (hubid < 0x1200);

         if ((diff != 1) && is_mdc &&              // data belongs to MDC where such disorder allowed to repair
             ((trignr & 0xffff) == 0) &&           // lower two bytes in trigger id are 0 (from 0x2b0000)
             (fTriggerRangeMask > 0x100000) &&     // more than 4+16 bits used in trigger mask
             (cfg.fLastTrigNr != kNoTrigger) &&    // last trigger is not dummy
             ((cfg.fLastTrigNr & 0xffff) == 0xffff) &&  // lower byte of last trigger is 0xffff (from 0x2bffff)
             ((trignr & 0xffff0000) == (cfg.fLastTrigNr & 0xffff0000))) { // high bytes are same in last and now (0x2b == 0x2b)
                diff = 1;
                ignore_resort = true; // for MDC allows to repair also in the case of resorted data
                // if (cfg.fResortIndx >= 0)
                //   DOUT0("Potential fix for inp %u trignr %x resort index %d id %x", ninp, trignr, cfg.fResortIndx, hubid);
             }

         if (diff != 1) {

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

      // Trying to fix problem with old MDC readout
      // Produced sequence of trigger numbers are: 0x2bffff, 0x2b0000, 0x2c0001 and repeated every 64k events
      // In addition, packets order can be broken, therefore one can continue to search for such sequence

      if (((cfg.fTrigNr & 0xffff) == 0) &&       // lower two bytes in trigger id are 0 (from 0x2b0000)
           (fTriggerRangeMask > 0x100000) &&     // more than 4+16 bits used in trigger mask
           (ignore_resort || (cfg.fResortIndx < 0)) &&  // do not try to resort data, normally enabled for very special cases
           (cfg.fLastTrigNr != kNoTrigger) &&    // last trigger is not dummy
           ((cfg.fLastTrigNr & 0xffff) == 0xffff) &&  // lower byte of last trigger is 0xffff (from 0x2bffff)
           ((cfg.fTrigNr & 0xffff0000) == (cfg.fLastTrigNr & 0xffff0000))) // high bytes are same in last and now (0x2b == 0x2b)
         {
            // DOUT0("Repair trigger input %u detect: %x last: %x repaired: %x", ninp, cfg.fTrigNr, cfg.fLastTrigNr, (cfg.fLastTrigNr + 1) & fTriggerRangeMask);
            cfg.fTrigNr = (cfg.fLastTrigNr + 1) & fTriggerRangeMask;
         }

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

      uint32_t errorBits = cfg.subevnt->GetErrBits();

      if ((errorBits != 0) && (errorBits != 1))
         cfg.fErrorBitsCnt++;

      int diff = 1;
      if (cfg.fLastTrigNr != kNoTrigger)
         diff = CalcTrigNumDiff(cfg.fLastTrigNr, cfg.fTrigNr);

      if (diff > 1) {
         // DOUT0("******** LOST ninp %u last %x trignr %x lost %d", ninp, cfg.fLastTrigNr, cfg.fTrigNr, (diff-1));
         cfg.fLostTrig += (diff - 1);
      }

      cfg.fLastTrigNr = cfg.fTrigNr;

      // printf("Input%u Trig:%6x Tag:%2x diff:%d %s\n", ninp, cfg.fTrigNr, cfg.fTrigTag, diff, diff != 1 ? "ERROR" : "");
   }

   return true;
}

bool hadaq::CombinerModule::DropAllInputBuffers()
{
   DOUT0("DropAllInputBuffers()...");

   fLastDropTm.GetNow();
   fLastBuildTm = fLastDropTm;
   fAllFullDrops++;

   if (fBNETsend)
      fCheckBNETProblems = chkActive; // activate testing again

   unsigned maxnumsubev = 0, droppeddata = 0;

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
   fRunDiscEvents += maxnumsubev;
   fAllDiscEvents += maxnumsubev;
   fRunDroppedData += droppeddata;
   fAllDroppedData += droppeddata;

   return true;
}

int hadaq::CombinerModule::DestinationPort(uint32_t trignr)
{
   if (!fBNETsend || (NumOutputs()<2)) return -1;

   return (trignr/fBNETbunch) % NumOutputs();
}

bool hadaq::CombinerModule::CheckDestination(uint32_t trignr)
{
   if (!fBNETsend || (fLastTrigNr == kNoTrigger)) return true;

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


   PROFILER_GURAD(fBldProfiler, "bld", 0)

   fBldCalls++;

   auto currTm = dabc::TimeStamp::Now();

   if (fExtraDebug) {
      if (!fLastProcTm.null() && (currTm - fLastProcTm > fMaxProcDist))
         fMaxProcDist = currTm - fLastProcTm;
      fLastProcTm = currTm;
   }

   unsigned masterchannel = 0, min_inp = 0;
   uint32_t subeventssize = 0, mineventid = 0, maxeventid = 0, buildevid = 0;
   bool incomplete_data = false, any_data = false;
   int missing_inp = -1;

   // use fLastDebugTm to request used queue size only several times in seconds
   bool request_queue = false;
   if (fExtraDebug)
      request_queue = true;
   else if (fLastDebugTm.Expired(currTm, 0.2)) {
      request_queue = true;
      fLastDebugTm = currTm;
   }

   PROFILER_BLOCK("shft")

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      if (!fCfg[ninp].has_data)
         if (!ShiftToNextSubEvent(ninp)) {
            // could not get subevent data on any channel.
            // let framework do something before next try
            if (fExtraDebug && fLastDebugTm.Expired(currTm, 2.)) {
               DOUT1("Fail to build event while input %u is not ready numcanrecv %u maxtm = %5.3f ", ninp, NumCanRecv(ninp), fMaxProcDist);
               fLastDebugTm = currTm;
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
         min_inp = ninp;
      } else {
         if (CalcTrigNumDiff(evid, maxeventid) < 0)
            maxeventid = evid;

         if (CalcTrigNumDiff(mineventid, evid) < 0) {
            mineventid = evid;
            min_inp = ninp;
         }
      }
   } // for ninp

   PROFILER_BLOCK("drp")

   // we always build event with maximum trigger id = newest event, discard incomplete older events
   int diff0 = incomplete_data ? 0 : CalcTrigNumDiff(mineventid, maxeventid);

//   DOUT0("Min:%8u Max:%8u diff:%5d", mineventid, maxeventid, diff);

   // check potential error

   if (((fCheckBNETProblems == chkActive) || (fCheckBNETProblems == chkOk)) && (fEventBuildTimeout > 0.) && fLastBuildTm.Expired(currTm, fEventBuildTimeout*0.5)) {

      if (missing_inp >= 0)
         fBNETProblem = "no_data_" + std::to_string(missing_inp); // no data at input
      else
         fBNETProblem = dabc::format("blocked_") + std::to_string(min_inp); // input with minimal event id, show event diff

      fCheckBNETProblems = chkError; // detect error, next time will check after drop all buffers
   }

   ///////////////////////////////////////////////////////////////////////////////
   // check too large triggertag difference on input channels or very long delay in building,
   // to repair situation, try to flush all input buffers
   if (fLastDropTm.Expired(currTm, (fEventBuildTimeout > 0) ? 1.5*fEventBuildTimeout : 5.))
     if (((fTriggerNrTolerance > 0) && (diff0 > fTriggerNrTolerance)) || ((fEventBuildTimeout > 0) && fLastBuildTm.Expired(currTm, fEventBuildTimeout) && any_data && (fCfg.size() > 1))) {

        std::string msg;
        if ((fTriggerNrTolerance > 0) && (diff0 > fTriggerNrTolerance)) {
          msg = dabc::format(
              "Event id difference %d exceeding tolerance window %d (min input %u),",
              diff0, fTriggerNrTolerance, min_inp);
        } else {
           msg = dabc::format("No events were build since at least %.1f seconds,", fEventBuildTimeout);
        }

        if (missing_inp >= 0)
           msg += dabc::format(" missing data on input %d url: %s,", missing_inp, FindPort(InputName(missing_inp)).Cfg("url").AsStr().c_str());

        msg += " drop all!";

        SetInfo(msg, true);
        DOUT0("%s", msg.c_str());

#ifdef HADAQ_DEBUG
      fprintf(stderr, "DROP ALL\n");
#endif

        DropAllInputBuffers();

        if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
           DOUT1("Drop all buffers");
           fLastDebugTm = currTm;
        }

        return false; // retry on next set of buffers
     }


   PROFILER_BLOCK("chkcomp")

   if (incomplete_data)
      return false;

   uint32_t buildtag = fCfg[masterchannel].fTrigTag;

   // printf("build evid = %u\n", buildevid);

   ////////////////////////////////////////////////////////////////////////
   // second input loop: skip all subevents until we reach current trignum
   // select inputs which will be used for building
   //bool eventIsBroken=false;
   bool dataError = false, tagError = false;

   bool hasCompleteEvent = true;

   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
      bool foundsubevent = false;
      while (!foundsubevent) {
         uint32_t trignr = fCfg[ninp].fTrigNr;
         uint32_t trigtag = fCfg[ninp].fTrigTag;
         bool isempty = fCfg[ninp].fEmpty;
         bool haserror = fCfg[ninp].fDataError;
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

            fDataDroppedRateCnt += droppedsize;

            // Par(fDataDroppedRateName).SetValue(droppedsize/1024./1024.);
            fRunDroppedData += droppedsize;
            fAllDroppedData += droppedsize;

            if(!ShiftToNextSubEvent(ninp, false, true)) {
               if (fExtraDebug && fLastDebugTm.Expired(currTm, 2.)) {
                  DOUT1("Cannot shift data from input %d", ninp);
                  fLastDebugTm = currTm;
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

   PROFILER_BLOCK("buf")

   // here all inputs should be aligned to buildevid

   // for sync sequence number, check first if we have error from cts:
   uint32_t sequencenumber = fRunBuildEvents + 1; // HADES convention: sequencenumber 0 is "start event" of file

   if (fBNETsend)
      sequencenumber = (fCfg[masterchannel].fTrigNr << 8) | fCfg[masterchannel].fTrigTag;

   if (hasCompleteEvent && fCheckTag && tagError) {
      hasCompleteEvent = false;

      if (fBNETrecv) DOUT0("TAG error");

      fRunTagErrors++;
   }

   // provide normal buffer

   if (hasCompleteEvent) {
      if (fOut.IsBuffer() && (!fOut.IsPlaceForEvent(subeventssize) || !CheckDestination(buildevid))) {
         // first we close current buffer
         if (!FlushOutputBuffer()) {
            if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
               std::string sendmask;
               for (unsigned n=0;n<NumOutputs();n++)
                  sendmask.append(CanSend(n) ? "o" : "x");

               DOUT0("FlushOutputBuffer can't send to all %u outputs sendmask = %s", NumOutputs(), sendmask.c_str());
               fLastDebugTm = currTm;
            }
            return false;
         }
      }
      // after flushing last buffer, take next one:
      if (!fOut.IsBuffer()) {
         dabc::Buffer buf = TakeBuffer();
         if (buf.null()) {

            if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
               DOUT0("did not have new buffer - wait for it");
               fLastDebugTm = currTm;
            }

            return false;
         }
         if (!fOut.Reset(buf)) {
            SetInfo("Cannot use buffer for output - hard error!!!!", true);
            buf.Release();
            dabc::mgr.StopApplication();
            if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
               DOUT0("Abort application completely");
               fLastDebugTm = currTm;
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

      PROFILER_BLOCK("compl")

      fOut.NewEvent(sequencenumber, fRunNumber); // like in hadaq, event sequence number is independent of trigger.
      fRunBuildEvents++;
      fAllBuildEvents++;

      fOut.evnt()->SetDataError((dataError || tagError));
      if (dataError) fRunDataErrors++;
      if (tagError) fRunTagErrors++;

      unsigned trigtyp = 0;
      for (unsigned ninp = 0; ninp < fCfg.size(); ninp++) {
         trigtyp = fCfg[ninp].fTrigType;
         if (trigtyp) break;
      }

      // here event id, always from "cts master channel" 0
      unsigned currentid = trigtyp | (2 << 12); // DAQVERSION=2 for dabc
      fOut.evnt()->SetId(currentid);

      PROFILER_BLOCK("main")

      // third input loop: build output event from all not empty subevents
      for (auto &cfg : fCfg) {
         if (cfg.fEmpty && fSkipEmpty)
            continue;
         if (fBNETrecv)
            fOut.AddAllSubevents(cfg.evnt);
         else
            fOut.AddSubevent(cfg.subevnt);

         // record current state of event tag and queue level for control system
         if (request_queue)
            cfg.fNumCanRecv = NumCanRecv(cfg.ninp);
         cfg.fLastEvtBuildTrigId = (cfg.fTrigNr << 8) | (cfg.fTrigTag & 0xff);
      }

      PROFILER_BLOCK("after")

      fOut.FinishEvent();

      int diff = (fLastTrigNr != kNoTrigger) ? CalcTrigNumDiff(fLastTrigNr, buildevid) : 1;

      //if (fBNETsend && (diff != 1))
      //   DOUT0("%s %x %x %d", GetName(), fLastTrigNr, buildevid, diff);
      // if (fBNETsend) DOUT0("%s trig %x size %u", GetName(), buildevid, subeventssize);

#ifdef HADAQ_DEBUG
      fprintf(stderr, "BUILD:%6x\n", buildevid);
#endif

      if (fBNETrecv && fEvnumDiffStatistics && (fBNETNumRecv > 1) && (diff > fBNETbunch)) {
         // check if we really lost these events
         // int diff0 = diff;

         long ncycles = diff / (fBNETbunch * fBNETNumRecv);

         // substract big cycles
         diff -= ncycles * (fBNETbunch * fBNETNumRecv);

         // substract expected gap to previous cycle
         diff -= fBNETbunch * (fBNETNumRecv - 1);
         if (diff <= 0) diff = 1;

         // add lost events from big cycles
         diff += ncycles * fBNETbunch;

         // if (diff != 1) {
         //   DOUT0("Large EVENT difference %d bunch %ld ncycles %ld final %d", diff0, fBNETbunch, ncycles, diff);
         //}
      }

      fLastTrigNr = buildevid;

      fEventRateCnt++;
      // Par(fEventRateName).SetValue(1);

      if (fEvnumDiffStatistics && (diff > 1)) {

         if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
            DOUT1("Events gap %d", diff-1);
            fLastDebugTm = currTm;
         }

         fLostEventRateCnt += (diff-1);
         //Par(fLostEventRateName).SetValue(diff-1);
         fRunDiscEvents += (diff-1);
         fAllDiscEvents += (diff-1);
      }

      // if (subeventssize == 0) EOUT("ZERO EVENT");

      unsigned currentbytes = subeventssize + sizeof(hadaq::RawEvent);
      fRunRecvBytes += currentbytes;
      fAllRecvBytes += currentbytes;
      fDataRateCnt += currentbytes;
      // Par(fDataRateName).SetValue(currentbytes / 1024. / 1024.);

      if ((fCheckBNETProblems == chkActive) || (fCheckBNETProblems == chkError)) {
         fBNETProblem.clear();
         fCheckBNETProblems = chkOk; // no problems, event build normally, now wait for error, timeout relative to build time
      }

      fLastBuildTm = currTm;
   } else {
      PROFILER_BLOCKN("lostl", 14)
      fLostEventRateCnt += 1;
      // Par(fLostEventRateName).SetValue(1);
      fRunDiscEvents += 1;
      fAllDiscEvents += 1;
   } // ensure outputbuffer

   std::string debugmask;
   debugmask.resize(fCfg.size(), ' ');

   PROFILER_BLOCKN("shift", 15)

   // FINAL loop: proceed to next subevents
   for (unsigned ninp = 0; ninp < fCfg.size(); ninp++)
      if (fCfg[ninp].fTrigNr == buildevid) {
         debugmask[ninp] = 'o';
         ShiftToNextSubEvent(ninp, false, !hasCompleteEvent);
      } else {
         debugmask[ninp] = 'x';
      }

   if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
      DOUT1("Did building as usual mask %s complete = %5s maxdist = %5.3f s", debugmask.c_str(), DBOOL(hasCompleteEvent), fMaxProcDist);
      fLastDebugTm = currTm;
      fMaxProcDist = 0;
      // put here update of tid
      // fPID= syscall(SYS_gettid);
   }

   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true; // event is build successfully. try next one
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
      if (NumOutputs() < 2)
         return dabc::cmd_false;
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

         FlushOutputBuffer(); // need to ensure that all output data are moved to outputs

         // submit dummy buffer to the HLD outputs to stop current file
         for (unsigned k=1;k<NumOutputs();++k) {
            if (CanSend(k)) {
               dabc::Buffer eolbuf = TakeBuffer();
               if (eolbuf.null()) {
                  EOUT("FAIL to SEND EOL buffer to OUTPUT %d", k);
               } else {
                  DOUT2("SEND EOL to OUTPUT %d %d", k, eolbuf.GetTotalSize());
                  eolbuf.SetTypeId(hadaq::mbt_HadaqStopRun);
                  Send(k, eolbuf);
               }
            }
         }
      }

      return dabc::cmd_true;

   } else if (cmd.IsName("HCMD_DropAllBuffers")) {

      DropAllInputBuffers();

      if (fBNETsend && !fIsTerminating) {
         for (unsigned n = 0; n < NumInputs(); n++) {
            fCfg[n].fErrorBitsCnt = 0;
            fCfg[n].fDroppedTrig = 0;
            fCfg[n].fLostTrig = 0;
            fCfg[n].fHubSizeTmCnt = 0;
            fCfg[n].fHubLastSize = 0;
            fCfg[n].fHubPrevSize = 0;
            dabc::Command subcmd("ResetTransportStat");
            SubmitCommandToTransport(InputName(n), subcmd);
         }
      }

      cmd.SetStrRawData("true");
      return dabc::cmd_true;

   } else if (cmd.IsName("BnetCalibrControl")) {

      if (!fBNETsend || fIsTerminating || (NumInputs() == 0))
         return dabc::cmd_true;

      if (!fBnetCalibrCmd.null()) {
         EOUT("Still calibration command running");
         fBnetCalibrCmd.Reply(dabc::cmd_false);
      }

      fBnetCalibrCmd = cmd;
      fBnetCalibrCmd.SetInt("#replies", NumInputs());
      fBnetCalibrCmd.SetDouble("quality", 1.0);

      std::string rundir = "";
      unsigned runid = cmd.GetUInt("runid");

      if ((cmd.GetStr("mode") != "start") && !fBNETCalibrDir.empty() && (runid != 0)) {
         rundir = fBNETCalibrDir;
         rundir.append("/");
         rundir.append(dabc::HadaqFileSuffix(runid));
         std::string mkdir = "mkdir -p ";
         mkdir.append(rundir);
         auto res = std::system(mkdir.c_str());
         (void) res; // avoid compiler warnings
         fBnetCalibrCmd.SetStr("#rundir", rundir);
         rundir.append("/");
      }

      DOUT0("Combiner get BnetCalibrControl mode %s rundir %s", cmd.GetStr("mode").c_str(), rundir.c_str());

      for (unsigned n = 0; n < NumInputs(); n++) {
         dabc::Command subcmd("TdcCalibrations");
         subcmd.SetStr("mode", cmd.GetStr("mode"));
         subcmd.SetStr("rundir", rundir);
         SubmitCommandToTransport(InputName(n), Assign(subcmd));
      }

      return dabc::cmd_postponed;
   } else if (cmd.IsName("BnetCalibrRefresh")) {

      if (!fBNETsend || fIsTerminating || (NumInputs() == 0))
         return dabc::cmd_true;

      if (!fBnetRefreshCmd.null()) {
         EOUT("Still calibration command running");
         fBnetRefreshCmd.Reply(dabc::cmd_false);
      }

      fBnetRefreshCmd = cmd;
      fBnetRefreshCmd.SetInt("#replies", NumInputs());
      fBnetRefreshCmd.SetDouble("quality", 1.0);

      for (unsigned n = 0; n < NumInputs(); n++) {
         dabc::Command subcmd("CalibrRefresh");
         SubmitCommandToTransport(InputName(n), Assign(subcmd));
      }

      return dabc::cmd_postponed;

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


void hadaq::CombinerModule::StoreRunInfoStart()
{
   /* open ascii file eb_runinfo2ora.txt to store simple information for
      the started RUN. The format: start <run_id> <filename> <date> <time>
      where "start" is a key word which defines START RUN info. -S.Y.
    */
   if(!fRunToOracle || fRunNumber == 0) return;
   time_t t = fRunNumber + dabc::GetHadaqTimeOffset(); // new run number defines start time
   char ltime[20];            /* local time */
   struct tm tm_res;
   strftime(ltime, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &tm_res));
   std::string filename = GenerateFileName(fRunNumber); // new run number defines filename
   FILE *fp = fopen(fRunInfoToOraFilename.c_str(), "a+");
   if (fp) {
      fprintf(fp, "start %u %d %s %s\n", fRunNumber, fEBId, filename.c_str(), ltime);
      fclose(fp);
   }
   DOUT1("Write run info to %s - start: %lu %d %s %s ", fRunInfoToOraFilename.c_str(), (long unsigned) fRunNumber, fEBId, filename.c_str(), ltime);

}

void hadaq::CombinerModule::StoreRunInfoStop(bool onexit, unsigned newrunid)
{
   /* open ascii file eb_runinfo2ora.txt to store simple information for
      the stoped RUN. The format: stop <run_id> <date> <time> <events> <bytes>
      where "stop" is a key word which defines STOP RUN info. -S.Y.
    */

   if(!fRunToOracle || fRunNumber == 0) return; // suppress void output at beginning
   // JAM we do not use our own time, but time of next run given by epics master
   // otherwise mismatch between run start time that comes before run stop time!
   // note that this problem also occured with old EBs
   // only exception: when eventbuilder is discarded we use termination time!
   time_t t;
   if(onexit || (newrunid == 0))
      t = time(nullptr);
   else
      t = newrunid + dabc::GetHadaqTimeOffset(); // new run number defines stop time
   char ltime[20];            /* local time */
   struct tm tm_res;
   strftime(ltime, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&t, &tm_res));
   std::string filename = GenerateFileName(fRunNumber); // old run number defines old filename
   FILE *fp = fopen(fRunInfoToOraFilename.c_str(), "a+");
   if (fp) {
      fprintf(fp, "stop %u %d %s %s %s ", fRunNumber, fEBId, filename.c_str(), ltime, Unit(fRunBuildEvents));
      fprintf(fp, "%s\n", Unit(fRunRecvBytes));
      fclose(fp);
   }
   DOUT1("Write run info to %s - stop: %lu %d %s %s %s %s", fRunInfoToOraFilename.c_str(), (long unsigned) fRunNumber, fEBId, filename.c_str(), ltime, Unit(fRunBuildEvents),Unit(fRunRecvBytes));

}

void hadaq::CombinerModule::ResetInfoCounters()
{
   // DO NOT RESET COUNTERS IN BNET MODE
   fRunRecvBytes = 0;
   fRunBuildEvents = 0;
   fRunDiscEvents = 0;
   fRunDroppedData = 0;
   fRunTagErrors = 0;
   fRunDataErrors = 0;

   if (!fBNETrecv && !fIsTerminating)
      for (unsigned n = 0; n < NumInputs(); n++) {
         SubmitCommandToTransport(InputName(n), dabc::Command("ResetTransportStat"));

         fCfg[n].fLastEvtBuildTrigId = 0;
      }
}

const char *hadaq::CombinerModule::Unit(unsigned long v)
{
  // JAM stolen from old hadaq eventbuilders to keep precisely same format
   static char retVal[32];
   static char u[] = " kM";
   unsigned int i;

   for (i = 0; v >= 10000 && i < sizeof(u) - 2; v /= 1000, i++) {
   }
   snprintf(retVal, sizeof(retVal), "%4lu%c", v, u[i]);

   return retVal;
}

std::string hadaq::CombinerModule::GenerateFileName(unsigned /* runid */)
{
   return fPrefix + dabc::HadaqFileSuffix(fRunNumber, fEBId) + std::string(".hld");
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
      if ((Par(fEventRateName).Value().AsDouble() == 0) &&  (quality > 0.55)) { state = "NoData"; quality = 0.55; }
      if ((runid == 0) && runname.empty() && (quality > 0.5)) { state = "NoFile"; quality = 0.5; }

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
         // SetEvtbuildPar("prefix",hadaq::Observer::Args_prefixCode(fPrefix.c_str())); // also export changed prefix to EPICS
         fRunNumber = newrunid;
         ResetInfoCounters();
         StoreRunInfoStart();
         fBnetFileCmd.Reply(dabc::cmd_true);
      } else {
         fBnetFileCmd.SetInt("#replies", num-1);
      }
      return true;
   } else if (cmd.IsName("TdcCalibrations")) {
      int num = fBnetCalibrCmd.GetInt("#replies");
      double q = cmd.GetDouble("quality");
      if (q < fBnetCalibrCmd.GetDouble("quality"))
         fBnetCalibrCmd.SetDouble("quality", q);

      if (num == 1) {

         std::string rundir = fBnetCalibrCmd.GetStr("#rundir");
         DOUT0("COMBINER COMPLETE CALIBR PROCESSING quality %5.3f  dir %s", fBnetCalibrCmd.GetDouble("quality"), rundir.c_str());

         if (!fBNETCalibrPackScript.empty() && !rundir.empty() && (fBnetCalibrCmd.GetStr("mode") == "stop")) {
            std::string exec = fBNETCalibrPackScript;
            exec.append(" ");
            exec.append(rundir);
            int res = std::system(exec.c_str());
            DOUT0("EXEC %s res = %d", exec.c_str(), res);
         }

         fBnetCalibrCmd.Reply(dabc::cmd_true);
      } else {
         fBnetCalibrCmd.SetInt("#replies", num-1);
      }
      return true;
   } else if (cmd.IsName("CalibrRefresh")) {
      int num = fBnetCalibrCmd.GetInt("#replies");
      double q = cmd.GetDouble("quality");
      if (q < fBnetRefreshCmd.GetDouble("quality"))
         fBnetRefreshCmd.SetDouble("quality", q);
      fBnetRefreshCmd.SetInt("#replies", num-1);
      if (num == 1)
         fBnetRefreshCmd.Reply(dabc::cmd_true);
   }

   return dabc::ModuleAsync::ReplyCommand(cmd);
}

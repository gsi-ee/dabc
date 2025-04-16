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

#include "dogma/CombinerModule.h"

#include <cmath>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

#include "dabc/Manager.h"

#include "dogma/UdpTransport.h"

const unsigned kNoTrigger = 0xffffffff;


dogma::CombinerModule::CombinerModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fCfg(),
   fOut(),
   fFlushCounter(0),
   fIsTerminating(false),
   fRunToOracle(false),
   fFlushTimeout(1.),
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
   if (fEBId < 0) fEBId = dabc::mgr.NodeId() + 1; // hades eb ids start with 1

   fBNETsend = Cfg("BNETsend", cmd).AsBool(false);
   fBNETrecv = Cfg("BNETrecv", cmd).AsBool(false);
   fBNETbunch = Cfg("EB_EVENTS", cmd).AsInt(16);
   fBNETNumRecv = Cfg("BNET_NUMRECEIVERS", cmd).AsInt(1);
   fBNETNumSend = Cfg("BNET_NUMSENDERS", cmd).AsInt(1);

   fExtraDebug = Cfg("ExtraDebug", cmd).AsBool(true);

   fCheckTag = Cfg("CheckTag", cmd).AsBool(true);

   fSkipEmpty = Cfg("SkipEmpty", cmd).AsBool(true);

   fAllowDropBuffers = Cfg("AllowDropBuffers", cmd).AsBool(false);

   fBNETCalibrDir = Cfg("CalibrDir", cmd).AsStr();
   fBNETCalibrPackScript = Cfg("CalibrPack", cmd).AsStr();

   fEpicsRunNumber = 0;

   fLastTrigNr = kNoTrigger;
   fMaxDogmaTrigger = 0;
   fTriggerRangeMask = 0;

   if (fBNETrecv || fBNETsend)
      fRunNumber = 0; // ignore data without valid run id at beginning!
   else
      fRunNumber = 1; // runid from configuration time.

   fMaxDogmaTrigger = Cfg("TriggerNumRange", cmd).AsUInt(0x1000000);
   fTriggerNumStep = Cfg("TriggerNumStep", cmd).AsInt(1);
   if (fTriggerNumStep < 1) fTriggerNumStep = 1;
   fTriggerRangeMask = fMaxDogmaTrigger - 1;
   DOUT1("DOGMA %s module using maxtrigger 0x%x, rangemask:0x%x, triggerstep:%d", GetName(), fMaxDogmaTrigger, fTriggerRangeMask, fTriggerNumStep);
   fEvnumDiffStatistics = Cfg("AccountLostEventDiff", cmd).AsBool(true);

   fTriggerNrTolerance = Cfg("TriggerTollerance", cmd).AsInt(-1);
   if (fTriggerNrTolerance == -1) fTriggerNrTolerance = fMaxDogmaTrigger / 4;
   fEventBuildTimeout = Cfg("BuildDropTimeout", cmd).AsDouble(20.0); // 20 seconds configure this optionally from xml later
   fAllBuildEventsLimit = Cfg("MaxNumBuildEvt", cmd).AsUInt(0);

   for (unsigned n = 0; n < NumInputs(); n++) {
      fCfg.emplace_back();

      auto &cfg = fCfg[n];

      cfg.ninp = n;

      cfg.Reset(true);
      cfg.fResort = FindPort(InputName(n)).Cfg("resort").AsBool(false);
      cfg.fOptional = FindPort(InputName(n)).Cfg("optional").AsBool(false);
      if (cfg.fResort || cfg.fOptional)
         DOUT0("%s resort %s isoptional %s", InputName(n).c_str(), DBOOL(cfg.fResort), DBOOL(cfg.fOptional));
   }

   fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);

   // provide timeout with period/2, but trigger flushing after 3 counts
   // this will lead to effective flush time between FlushTimeout and FlushTimeout*1.5
   CreateTimer("FlushTimer", (fFlushTimeout > 0) ? fFlushTimeout/2. : 1.);

   //CreatePar("RunId");
   //Par("RunId").SetValue(fRunNumber); // to communicate with file components

   // TODO: optionally set this name
   fPrefix = Cfg("FilePrefix", cmd).AsStr("no");
   fRunToOracle = Cfg("Runinfo2ora", cmd).AsBool(false);


   fDataRateName = "DogmaData";
   fEventRateName = "DogmaEvents";
   fLostEventRateName = "DogmaLostEvents";
   fDataDroppedRateName = "DogmaDroppedData";
   fInfoName = "DogmaInfo";
   fCheckBNETProblems = chkNone;
   fBNETProblem = "";

   CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar(fEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar(fLostEventRateName).SetRatemeter(false, 3.).SetUnits("Ev");
   CreatePar(fDataDroppedRateName).SetRatemeter(false, 3.).SetUnits("MB");

   fDataRateCnt = fEventRateCnt = fDataDroppedRateCnt = 0;
   fLostEventRateCnt = 0.;

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
      PublishPars("$CONTEXT$/DogmaCombiner");
   else
      PublishPars(dabc::format("$CONTEXT$/%s", GetName()));

   fWorkerHierarchy.SetField("_player", "DABC.DogmaDAQControl");

   if (fBNETsend)
      fWorkerHierarchy.SetField("_bnet", "sender");
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


dogma::CombinerModule::~CombinerModule()
{
   DOUT3("dogma::CombinerModule::DTOR..does nothing now!.");
   //fOut.Close().Release();
   //fCfg.clear();
}

void dogma::CombinerModule::ModuleCleanup()
{
   DOUT0("dogma::CombinerModule::ModuleCleanup()");
   fIsTerminating = true;
   fOut.Close().Release();

   for (unsigned n=0;n<fCfg.size();n++)
      fCfg[n].Reset();

   DOUT5("dogma::CombinerModule::ModuleCleanup() after  fCfg[n].Reset()");

//   DOUT0("First %06x Last %06x Num %u Time %5.2f", firstsync, lastsync, numsync, tm2-tm1);
//   if (numsync>0)
//      DOUT0("Step %5.2f rate %5.2f sync/s", (lastsync-firstsync + 0.) / numsync, (numsync + 0.) / (tm2-tm1));
}


void dogma::CombinerModule::SetInfo(const std::string &info, bool forceinfo)
{
//   DOUT0("SET INFO: %s", info.c_str());

   dabc::InfoParameter par;

   if (!fInfoName.empty()) par = Par(fInfoName);

   par.SetValue(info);
   if (forceinfo)
      par.FireModified();
}

void dogma::CombinerModule::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) == "BnetTimer") {
      UpdateBnetInfo();
      return;
   }

   if ((fFlushTimeout > 0) && (++fFlushCounter > 2)) {
      fFlushCounter = 0;
      dabc::ProfilerGuard grd(fBldProfiler, "flush", 30);
      FlushOutputBuffer();
   }

   fTimerCalls++;

   Par(fDataRateName).SetValue(fDataRateCnt/1024./1024.);
   Par(fEventRateName).SetValue(fEventRateCnt);
   Par(fLostEventRateName).SetValue(fLostEventRateCnt);
   Par(fDataDroppedRateName).SetValue(fDataDroppedRateCnt/1024./1024.);

   fDataRateCnt = fEventRateCnt = fDataDroppedRateCnt = 0;
   fLostEventRateCnt = 0.;

   fLastEventRate = Par(fEventRateName).Value().AsDouble();

   // invoke event building, if necessary - reinjects events
   StartEventsBuilding();

   if ((fAllBuildEventsLimit > 0) && (fAllBuildEvents >= fAllBuildEventsLimit)) {
      FlushOutputBuffer();
      fAllBuildEventsLimit = 0; // invoke only once
      dabc::mgr.StopApplication();
   }
}

void dogma::CombinerModule::AccountDroppedData(unsigned sz, unsigned lost_events)
{
   fDataDroppedRateCnt += sz;
   fRunDroppedData += sz;
   fAllDroppedData += sz;

   fRunDiscEvents += lost_events;
   fAllDiscEvents += lost_events;

   fLostEventRateCnt += lost_events > 0 ? 1. * lost_events : 1. / fCfg.size();
}

void dogma::CombinerModule::StartEventsBuilding()
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

void dogma::CombinerModule::ProcessUserEvent(unsigned item)
{
   if (fSpecialItemId == item) {
      // DOUT0("Get user event");
      fSpecialFired = false;
   } else {
      EOUT("Get wrong user event");
   }

   StartEventsBuilding();
}

void dogma::CombinerModule::BeforeModuleStart()
{
   auto info = dabc::format("DOGMA %s starts. numinp:%u, numout:%u flush:%3.1f", GetName(),
                            NumInputs(), NumOutputs(), fFlushTimeout);

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
      fCfg[ninp].fQueueCapacity = InputQueueCapacity(ninp);
      if (fBNETrecv)
         continue;
      dabc::Command cmd("GetDogmaTransportInfo");
      cmd.SetInt("id", ninp);
      SubmitCommandToTransport(InputName(ninp), Assign(cmd));
   }
}

void dogma::CombinerModule::AfterModuleStop()
{
   auto info = dabc::format("DOGMA %s stopped. CompleteEvents:%d, BrokenEvents:%d, DroppedData:%d, "
                            "RecvBytes:%d, data errors:%d, tag errors:%d",
                            GetName(), (int)fAllBuildEvents, (int)fAllDiscEvents, (int)fAllDroppedData,
                            (int)fAllRecvBytes, (int)fRunDataErrors, (int)fRunTagErrors);

   SetInfo(info, true);
   DOUT0("%s", info.c_str());

   // when BNET receiver module stopped, lead to application stop
   if (fBNETrecv) dabc::mgr.StopApplication();
}


bool dogma::CombinerModule::FlushOutputBuffer()
{
   if (fOut.IsEmpty() || !fOut.IsBuffer()) {
      DOUT3("FlushOutputBuffer has no buffer to flush");
      return false;
   }

   int dest = DestinationPort(fLastTrigNr);

   bool drop_buffer = false;

   if (dest < 0) {
      if (!CanSendToAllOutputs()) {
         if (fAllowDropBuffers)
            drop_buffer = true;
         else
            return false;
      }
   } else {
      if (!CanSend(dest))
         return false;
   }

   dabc::Buffer buf = fOut.Close();

   // if (fBNETsend) DOUT0("%s FLUSH buffer", GetName());

   if (drop_buffer)
      AccountDroppedData(buf.GetTotalSize(), dogma::ReadIterator::NumEvents(buf));
   else if (dest < 0)
      SendToAllOutputs(buf);
   else
      Send(dest, buf);

   fFlushCounter = 0; // indicate that next flush timeout one not need to send buffer

   return true;
}

void dogma::CombinerModule::UpdateBnetInfo()
{
   fBldProfiler.MakeStatistic();

   dabc::ProfilerGuard grd(fBldProfiler, "info", 20);

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
         auto info = (dogma::TransportInfo *) inp.fInfo;
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
            sinfo = dabc::format("port:%d %5.3f MB/s data:%s pkts:%s buf:%s disc:%s magic:%s drop:%s lost:%s errbits:%s ",
                       info->fNPort,
                       rate,
                       dabc::size_to_str(info->fTotalRecvBytes).c_str(),
                       dabc::number_to_str(info->fTotalRecvPacket,1).c_str(),
                       dabc::number_to_str(info->fTotalProducedBuffers).c_str(),
                       info->GetDiscardString().c_str(),
                       info->GetDiscardMagicString().c_str(),
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
      for (unsigned n = 0; n < NumOutputs(); ++n) {
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

   fBldCalls = fInpCalls = fOutCalls = fBufCalls = fTimerCalls = 0;
}


void dogma::CombinerModule::ProcessConnectEvent(const std::string &name, bool on)
{
   printf("ProcessConnectEvent %s ninp %u on %s\n", name.c_str(), NumInputs(), DBOOL(on));

   if (on) return;

   for (unsigned n = 0; n < NumInputs(); ++n)
      if (InputName(n) == name) {
         ProcessEOF(n);
         return;
      }
}


void dogma::CombinerModule::ProcessEOF(unsigned ninp)
{
   if (ninp >= fCfg.size())
      return;

   fCfg[ninp].has_eof = true;

   bool all_eof = true;
   for (auto &cc : fCfg)
      if (!cc.has_eof)
         all_eof = false;

   if (all_eof)
      Stop();
}


///////////////////////////////////////////////////////////////
//////// INPUT BUFFER METHODS:


bool dogma::CombinerModule::ShiftToNextBuffer(unsigned ninp)
{
   auto &cfg = fCfg[ninp];

   ReadIterator& iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

   iter.Close();

   dabc::Buffer buf;

   if (cfg.fResortIndx < 0) {
      // normal way to take next buffer
      if(!CanRecv(ninp))
         return false;
      buf = Recv(ninp);
      fNumReadBuffers++;
   } else {
      // do not try to look further than one more buffer
      if (cfg.fResortIndx > 1)
         return false;
      // when doing resort, try to access buffers from the input queue
      buf = RecvQueueItem(ninp, cfg.fResortIndx++);
   }

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      printf("SEE EOF %u\n", ninp);
      if (cfg.fResortIndx < 0)
         ProcessEOF(ninp);
      return false;
   }

   DOUT5("CombinerModule::ShiftToNextBuffer %d type %u size %u", ninp, buf.GetTypeId(), buf.GetTotalSize());

   return iter.Reset(buf);
}

bool dogma::CombinerModule::ShiftToNextTu(unsigned ninp)
{
   auto &cfg = fCfg[ninp];
   ReadIterator &iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

   while (true) {

      bool res = false;
      if (iter.IsData())
         res = iter.NextSubeventsBlock();

      if (res && iter.IsData()) {
         // DOUT0("CombinerModule::ShiftToNextTu %u OK", ninp);
         return true;
      }

      if(!ShiftToNextBuffer(ninp))
         return false;
   }

   return false;
}

int dogma::CombinerModule::CalcTrigNumDiff(const uint32_t &prev, const uint32_t &next)
{
   int res = (int)next - prev;
   if (res > (int)fMaxDogmaTrigger / 2)
      res -= fMaxDogmaTrigger;
   else if (res < (int)fMaxDogmaTrigger / -2)
      res += fMaxDogmaTrigger;
   return res;
}

bool dogma::CombinerModule::ShiftToNextEvent(unsigned ninp, bool fast, bool dropped)
{
   // function used to shift to next event - used in BNET builder mode

   InputCfg &cfg = fCfg[ninp];

   if (dropped && cfg.has_data) cfg.fDroppedTrig++;

   cfg.Reset(fast);

   ReadIterator& iter = cfg.fIter;

   if (!iter.NextEvent())
      // retry in next hadtu container
      if (!ShiftToNextTu(ninp)) return false;

   // no need to analyze data
   if (fast) return true;

   // this is selected event
   cfg.evnt = iter.evnt();
   cfg.has_data = true;
   cfg.data_size = cfg.evnt->GetPayloadLen();

   cfg.fTrigNr = cfg.evnt->GetTrigNumber();
   cfg.fTrigType = cfg.evnt->GetTrigType();

   cfg.fTrigTag = 0;

   cfg.fTrigNumRing[cfg.fRingCnt] = cfg.fTrigNr;
   cfg.fRingCnt = (cfg.fRingCnt+1) % DOGMA_RINGSIZE;

   cfg.fEmpty = (cfg.data_size == 0);
   cfg.fDataError = 0;

   // int diff = CalcTrigNumDiff(cfg.fLastTrigNr,cfg.fTrigNr);
   // if (diff != 1)
   //   DOUT0("Inp%u Diff%d %x %x distance: %u", ninp, diff, cfg.fLastTrigNr, cfg.fTrigNr, iter.OnlyDebug());

   cfg.fLastTrigNr = cfg.fTrigNr;

   return true;
}


bool dogma::CombinerModule::ShiftToNextSubEvent(unsigned ninp, bool fast, bool dropped)
{
   if (fBNETrecv) return ShiftToNextEvent(ninp, fast, dropped);

   // DOUT0("CombinerModule::ShiftToNextSubEvent %d ", ninp);

   auto &cfg = fCfg[ninp];

   bool foundevent = false, doshift = true, tryresort = cfg.fResort;

   if (cfg.fResortIndx >= 0) {
      doshift = false; // do not shift event in main iterator
      if (cfg.subevnt) cfg.subevnt->SetTrigTypeNumber(0xffffffff); // mark subevent as used
      cfg.fResortIndx = -1;
      cfg.fResortIter.Close();
   } else {
      // account when subevent exists but intentionally dropped
      if (dropped && cfg.has_data) cfg.fDroppedTrig++;
   }

   cfg.Reset(fast);

   // if (fast) DOUT0("FAST DROP on inp %d", ninp);

   while (!foundevent) {
      auto &iter = (cfg.fResortIndx < 0) ? cfg.fIter : cfg.fResortIter;

      bool res = true;
      if (doshift) res = iter.NextSubEvent();
      doshift = true;

      // DOUT0("CombinerModule::ShiftToNextSubEvent %d res %s", ninp, DBOOL(res));

      if (!res || !iter.subevnt()) {
         DOUT5("CombinerModule::ShiftToNextSubEvent %d with zero NextSubEvent()", ninp);

         // retry in next hadtu container
         if (ShiftToNextTu(ninp)) continue;

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
      if (fast) return true;

      if (tryresort && (cfg.fLastTrigNr != kNoTrigger)) {
         uint32_t trignr = iter.subevnt()->GetTrigNumber();
         if (trignr == kNoTrigger) continue; // this is processed trigger, exclude it

         int diff = CalcTrigNumDiff(cfg.fLastTrigNr, trignr & fTriggerRangeMask);

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
      cfg.data_size = cfg.subevnt->GetSize();

      cfg.fTrigNr = cfg.subevnt->GetTrigNumber() & fTriggerRangeMask;
      cfg.fTrigType = cfg.subevnt->GetTrigType();
      cfg.fHubId = cfg.subevnt->GetAddr();
      cfg.fTrigTag = 0;

      DOUT5("inp %u event nr %u type %u", ninp, cfg.fTrigNr, cfg.fTrigType);

      cfg.fTrigNumRing[cfg.fRingCnt] = cfg.fTrigNr;
      cfg.fRingCnt = (cfg.fRingCnt+1) % DOGMA_RINGSIZE;

      cfg.fEmpty = cfg.subevnt->GetPayloadLen() == 0;
      cfg.fDataError = 0;

      uint32_t errorBits = 0;

      if ((errorBits != 0) && (errorBits != 1))
         cfg.fErrorBitsCnt++;

      int diff = fTriggerNumStep;
      if (cfg.fLastTrigNr != kNoTrigger)
         diff = CalcTrigNumDiff(cfg.fLastTrigNr, cfg.fTrigNr);
      cfg.fLastTrigNr = cfg.fTrigNr;

      if (diff >= 2*fTriggerNumStep)
         cfg.fLostTrig += diff / fTriggerNumStep - 1;

      // printf("Input%u Trig:%6x Tag:%2x diff:%d %s\n", ninp, cfg.fTrigNr, cfg.fTrigTag, diff, diff != 1 ? "ERROR" : "");
   }

   return true;
}

bool dogma::CombinerModule::DropAllInputBuffers()
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

      if (numsubev > maxnumsubev)
         maxnumsubev = numsubev;

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

int dogma::CombinerModule::DestinationPort(uint32_t trignr)
{
   if (!fBNETsend || (NumOutputs() < 2)) return -1;

   return (trignr/fBNETbunch) % NumOutputs();
}

bool dogma::CombinerModule::CheckDestination(uint32_t trignr)
{
   if (!fBNETsend || (fLastTrigNr == kNoTrigger)) return true;

   return DestinationPort(fLastTrigNr) == DestinationPort(trignr);
}

bool dogma::CombinerModule::BuildEvent()
{
   // RETURN VALUE: true - event is successfully build, recall immediately
   //               false - leave event loop for framework (other modules input is required!)

   // eventbuilding on hadtu streams here:

   // this is daq_evtbuild logic:
   // first check eventnumber of master channel
   // here loop over all channels: skip subevts with too old eventnumbers
   // if event is not complete, discard this and try next master channel index

   // adjust run number that might have changed by file output
   // note: file outout will overwrite this number in event header to be consistent with file name
   // for online monitor, we could live with different run numbers

   ///////////////////////////////////////////////////////////
   // alternative approach like a simplified mbs event building:
   //////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////////////
   // first input loop: find out maximum trignum of all inputs = current event trignumber


   // dabc::ProfilerGuard grd(fBldProfiler, "bld", 0);

   fBldCalls++;

   if (fExtraDebug) {
      double tm = fLastProcTm.SpentTillNow(true);
      if (tm > fMaxProcDist) fMaxProcDist = tm;
   }

   // DOUT0("dogma::CombinerModule::BuildEvent() starts");

   auto currTm = dabc::TimeStamp::Now();

   unsigned min_inp = 0, mast_have_max_inp = 0;
   uint32_t subeventssize = 0, mineventid = 0, maxeventid = 0, mineventid_must_have = 0, maxeventid_must_have = 0;
   bool incomplete_data = false, any_data = false, must_have_data = false;
   int missing_inp = -1;

   // grd.Next("shft");

   for (auto &cfg : fCfg) {

      bool miss_data = false;

      if (!cfg.has_data && !ShiftToNextSubEvent(cfg.ninp))
         miss_data = true;

      // skip data in optional inputs if they arrived AFTER event was already build with such id
      if (!miss_data && cfg.fOptional && (fLastTrigNr != kNoTrigger)) {
         while (cfg.has_data && (CalcTrigNumDiff(fLastTrigNr, cfg.fTrigNr) <= 0)) {
            AccountDroppedData(cfg.data_size);
            if (!ShiftToNextSubEvent(cfg.ninp))
               miss_data = true;
         }
      }

      if (miss_data) {
         // could not get subevent data on the channel.
         // let framework do something before next try
         if (fExtraDebug && fLastDebugTm.Expired(currTm, 2.) && !cfg.fOptional) {
            DOUT1("Fail to build event while input %u is not ready numcanrecv %u maxtm = %5.3f ", cfg.ninp, NumCanRecv(cfg.ninp), fMaxProcDist);
            fLastDebugTm = currTm;
            fMaxProcDist = 0;
         }

         // data incomplete when input must be there or optional input did not provide data for long time
         if (!cfg.fOptional || !cfg.fLastDataTm.Expired(currTm, fFlushTimeout > 0 ? fFlushTimeout : 0.5)) {
            missing_inp = cfg.ninp;
            incomplete_data = true;
         }

         continue;
      }

      uint32_t evid = cfg.fTrigNr;

      if (!cfg.fOptional) {
         if (!must_have_data) {
            must_have_data = true;
            mineventid_must_have = maxeventid_must_have = evid;
            mast_have_max_inp = cfg.ninp;
         } else {
            if (CalcTrigNumDiff(evid, maxeventid_must_have) < 0) {
               maxeventid_must_have = evid;
               mast_have_max_inp = cfg.ninp;
            }

            if (CalcTrigNumDiff(mineventid_must_have, evid) < 0)
               mineventid_must_have = evid;
         }
      }

      if (!any_data) {
         any_data = true;
         mineventid = maxeventid = evid;
         min_inp = cfg.ninp;
      } else {
         if (CalcTrigNumDiff(evid, maxeventid) < 0)
            maxeventid = evid;

         if (CalcTrigNumDiff(mineventid, evid) < 0) {
            mineventid = evid;
            min_inp = cfg.ninp;
         }
      }
   } // for ninp

   // grd.Next("drp");

   // for must_have channels we always build event with maximum trigger id = newest event, discarding incomplete older events
   int diff0 = (incomplete_data || !must_have_data) ? 0 : CalcTrigNumDiff(mineventid_must_have, maxeventid_must_have);

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

        DropAllInputBuffers();

        if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
           DOUT1("Drop all buffers");
           fLastDebugTm = currTm;
        }

        return false; // retry on next set of buffers
     }

   // grd.Next("chkcomp");

   if (incomplete_data || !any_data) {
      if (fExtraDebug && fLastDebugTm.Expired(currTm, 0.5)) {
         DOUT1("Do not build - %s data", !any_data ? "no any" : "incomplete");
         for (auto &cfg : fCfg)
            DOUT1("   ninp %u optional %s has_data %s Last data tm expired %s", cfg.ninp, DBOOL(cfg.fOptional), DBOOL(cfg.has_data), DBOOL(cfg.fLastDataTm.Expired(currTm, 0.5)));
         fLastDebugTm = currTm;
      }
      return false;
   }

   // which channel is definitely in the data
   unsigned masterchannel = must_have_data ? mast_have_max_inp : min_inp;

   uint32_t buildevid = fCfg[masterchannel].fTrigNr,
            buildtag = fCfg[masterchannel].fTrigTag,
            buildtype = fCfg[masterchannel].fTrigType;


   // printf("build evid = %u\n", buildevid);

   ////////////////////////////////////////////////////////////////////////
   // second input loop: skip all subevents until we reach current trignum
   // select inputs which will be used for building
   //bool eventIsBroken=false;
   bool dataError = false, tagError = false, canBuildEvent = true;

   for (auto &cfg : fCfg) {

      while (cfg.has_data) {
         if (cfg.fTrigNr == buildevid) {

            if (!cfg.fEmpty || !fSkipEmpty) {
               // check also trigtag:
               if (cfg.fTrigTag != buildtag) tagError = true;
               if (cfg.fDataError) dataError = true;
               subeventssize += cfg.data_size;
            }
            break;
         }

         if (CalcTrigNumDiff(cfg.fTrigNr, buildevid) < 0) {
            // we want to build event with id, defined by input 0
            // but subevent in this input has number bigger than buildevid
            // it will not be possible to build buildevid, therefore mark it as incomplete
            if (!cfg.fOptional)
               canBuildEvent = false;

            // let also verify all other channels
            break;
         }

         AccountDroppedData(cfg.data_size);

         //if (!cfg.fOptional)
         //   EOUT("Skip data in must_have channel %u", cfg.ninp);

         // try with next subevt until reaching buildevid
         ShiftToNextSubEvent(cfg.ninp, false, true);
      } // while (cfg.has_data)

      // can build event only if miss data on optional channel
      if (!cfg.has_data && !cfg.fOptional)
         canBuildEvent = false;

   } // for fCfg

   // grd.Next("buf");

   // here all inputs should be aligned to buildevid

   // for sync sequence number, check first if we have error from cts:
   uint32_t sequencenumber = fRunBuildEvents + 1; // HADES convention: sequencenumber 0 is "start event" of file

   if (fBNETsend)
      sequencenumber = (fCfg[masterchannel].fTrigNr << 8) | fCfg[masterchannel].fTrigTag;

   if (canBuildEvent && fCheckTag && tagError) {
      canBuildEvent = false;

      if (fBNETrecv) DOUT0("TAG error");

      fRunTagErrors++;
   }

   // provide normal buffer

   if (canBuildEvent) {
      if (fOut.IsBuffer() && (!fOut.IsPlaceForEvent(subeventssize) || !CheckDestination(buildevid))) {
         // first we close current buffer
         if (!FlushOutputBuffer()) {
            if (fExtraDebug && fLastDebugTm.Expired(1.)) {
               std::string sendmask;
               for (unsigned n = 0; n < NumOutputs(); n++)
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
         canBuildEvent = false;
      }
   }

   int buildevid_diff = 0;

   // now we should be able to build event
   if (canBuildEvent) {
      // EVENT BUILDING STARTS HERE

      // grd.Next("compl");

      fOut.NewEvent(sequencenumber, buildtype, buildevid);

      DOUT5("Building event seq:%u typ:%u id %u", sequencenumber, buildtype, buildevid);

      fRunBuildEvents++;
      fAllBuildEvents++;

      // fOut.evnt()->SetDataError((dataError || tagError));
      if (dataError) fRunDataErrors++;
      if (tagError) fRunTagErrors++;

      // grd.Next("main");

      // third input loop: build output event from all not empty subevents
      for (auto &cfg : fCfg) {
         if (!cfg.has_data)
            continue;
         if (cfg.fEmpty && fSkipEmpty)
            continue;
         if (cfg.fTrigNr != buildevid)
            continue;

         if (fBNETrecv)
            fOut.AddAllSubevents(cfg.evnt);
         else
            fOut.AddSubevent(cfg.subevnt);

         // DoInputSnapshot(ninp);
         // tag all information about input when using it
         cfg.fLastDataTm = currTm;
         cfg.fNumCanRecv = NumCanRecv(cfg.ninp);
         cfg.fQueueLevel = (cfg.fQueueCapacity > 0) ? 1. * cfg.fNumCanRecv / cfg.fQueueCapacity : 0.;
         cfg.fLastEvtBuildTrigId = (cfg.fTrigNr << 8) | (cfg.fTrigTag & 0xff);
      } // for ninp

      // grd.Next("after");

      fOut.FinishEvent();

      buildevid_diff = 1;
      if (fLastTrigNr != kNoTrigger)
         buildevid_diff = CalcTrigNumDiff(fLastTrigNr, buildevid);

      //if (fBNETsend && (buildevid_diff != 1))
      //   DOUT0("%s %x %x %d", GetName(), fLastTrigNr, buildevid, buildevid_diff);
      // if (fBNETsend) DOUT0("%s trig %x size %u", GetName(), buildevid, subeventssize);

      if (fBNETrecv && fEvnumDiffStatistics && (fBNETNumRecv > 1) && (buildevid_diff > fBNETbunch)) {
         // check if we really lost these events
         // int diff0 = diff;

         long ncycles = buildevid_diff / (fBNETbunch * fBNETNumRecv);

         // substract big cycles
         buildevid_diff -= ncycles * (fBNETbunch * fBNETNumRecv);

         // substract expected gap to previous cycle
         buildevid_diff -= fBNETbunch * (fBNETNumRecv - 1);
         if (buildevid_diff <= 0) buildevid_diff = 1;

         // add lost events from big cycles
         buildevid_diff += ncycles * fBNETbunch;

         // if (buildevid_diff != 1) {
         //   DOUT0("Large EVENT difference %d bunch %ld ncycles %ld final %d", diff0, fBNETbunch, ncycles, buildevid_diff);
         //}
      }

      fLastTrigNr = buildevid;

      fEventRateCnt++;
      // Par(fEventRateName).SetValue(1);

      if (fEvnumDiffStatistics && (buildevid_diff > fTriggerNumStep)) {

         if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
            DOUT1("Events gap %d", buildevid_diff-1);
            fLastDebugTm = currTm;
         }

         fLostEventRateCnt += buildevid_diff / fTriggerNumStep - 1;
         //Par(fLostEventRateName).SetValue(diff-1);
         fRunDiscEvents += buildevid_diff / fTriggerNumStep - 1;
         fAllDiscEvents += buildevid_diff / fTriggerNumStep - 1;
      }

      // if (subeventssize == 0) EOUT("ZERO EVENT");

      unsigned currentbytes = subeventssize + sizeof(dogma::DogmaEvent);
      fRunRecvBytes += currentbytes;
      fAllRecvBytes += currentbytes;
      fDataRateCnt += currentbytes;
      // Par(fDataRateName).SetValue(currentbytes / 1024. / 1024.);

      if ((fCheckBNETProblems == chkActive) || (fCheckBNETProblems == chkError)) {
         fBNETProblem.clear();
         fCheckBNETProblems = chkOk; // no problems, event build normally, now wait for error, timeout relative to build time
      }

      fLastBuildTm.GetNow();
   } else {

      // grd.Next("lostl", 14);
      fLostEventRateCnt += 1;
      // Par(fLostEventRateName).SetValue(1);
      fRunDiscEvents += 1;
      fAllDiscEvents += 1;
   }

   std::string debugmask;
   debugmask.resize(fCfg.size(), ' ');

   // grd.Next("shift", 15);

   // bool fatal = !fCfg[1].has_data || (fCfg[1].fTrigNr != buildevid);

   // FINAL loop: proceed to next subevents
   for (auto &cfg : fCfg)
      if (cfg.has_data && (cfg.fTrigNr == buildevid)) {
         debugmask[cfg.ninp] = 'o';
         ShiftToNextSubEvent(cfg.ninp, false, !canBuildEvent);
      } else {
         debugmask[cfg.ninp] = 'x';
      }

   if (fExtraDebug && fLastDebugTm.Expired(currTm, 1.)) {
      DOUT1("Did building as usual mask %s canBuild = %5s maxdist = %5.3f s", debugmask.c_str(), DBOOL(canBuildEvent), fMaxProcDist);
      fLastDebugTm = currTm;
      fMaxProcDist = 0;
      // put here update of tid
      // fPID= syscall(SYS_gettid);
   }

   //if (fatal) {
   //   printf("Event %6u diff %2d mask %s %s\n", buildevid, buildevid_diff, debugmask.c_str(), buildevid_diff == 1 && (debugmask == "oo")  ? "" : "?????????");
   //   printf("MISMATCH!!!!\n");
   //   dabc::mgr.StopApplication();
   //}

   // if (debug++ > 20000)
   //    dabc::mgr.StopApplication();

   // return true means that method can be called again immediately
   // in all places one requires while loop
   return true; // event is build successfully. try next one
}

int dogma::CombinerModule::ExecuteCommand(dabc::Command cmd)
{
   bool do_start = false, do_stop = false;

   if (cmd.IsName("StartDogmaFile")) {
      do_start = do_stop = true;
      SetInfo("Execute StartDogmaFile");

      DOUT0("******************* START DOGMA FILE *************");
   } else if (cmd.IsName("StopDogmaFile")) {
      do_stop = true;
      SetInfo("Execute StopDogmaFile");
      DOUT0("******************* STOP DOGMA FILE *************");

   } else if (cmd.IsName("RestartDogmaFile")) {
      if (NumOutputs() < 2) return dabc::cmd_false;
      SetInfo("Execute RestartDogmaFile");
      cmd.ChangeName("RestartTransport");
      SubmitCommandToTransport(OutputName(1), cmd);
      return dabc::cmd_postponed;
   } else if (cmd.IsName("BnetFileControl")) {
      if (NumOutputs() < 2)
         return dabc::cmd_false;
      if (!fBnetFileCmd.null())
         fBnetFileCmd.Reply(dabc::cmd_false);

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
         // reset runid
         fRunNumber = 0;

         FlushOutputBuffer(); // need to ensure that all output data are moved to outputs

         // submit dummy buffer to the HLD outputs to stop current file
         for (unsigned k = 1; k < NumOutputs(); ++k) {
            if (CanSend(k)) {
               dabc::Buffer eolbuf = TakeBuffer();
               if (eolbuf.null()) {
                  EOUT("FAIL to SEND EOL buffer to OUTPUT %d", k);
               } else {
                  DOUT2("SEND EOL to OUTPUT %d %d", k, eolbuf.GetTotalSize());
                  eolbuf.SetTypeId(dogma::mbt_DogmaStopRun);
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
      if (NumOutputs() > 1)
         res = DisconnectPort(OutputName(1));

      DOUT0("Stop HLD file res = %s", DBOOL(res));
   }

   if (do_start && res) {
      std::string fname = cmd.GetStr("filename", "file.dogma");
      int maxsize = cmd.GetInt(dabc::xml_maxsize, 1500);

      std::string url = dabc::format("dogma://%s?%s=%d", fname.c_str(), dabc::xml_maxsize, maxsize);

      // we guarantee, that at least two ports will be created
      EnsurePorts(0, 2);

      res = dabc::mgr.CreateTransport(OutputName(1, true), url);

      DOUT0("Start HLD file %s res = %s", fname.c_str(), DBOOL(res));
   }

   return cmd_bool(res);
}


void dogma::CombinerModule::ResetInfoCounters()
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

bool dogma::CombinerModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetDogmaTransportInfo")) {
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
         std::string newprefix = fBnetFileCmd.GetStr("prefix");
         if(!newprefix.empty()) fPrefix = newprefix; // need to reset prefix here for run info JAM2018
         fRunNumber = newrunid;
         ResetInfoCounters();
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

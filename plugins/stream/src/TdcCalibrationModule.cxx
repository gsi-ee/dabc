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

#include "stream/TdcCalibrationModule.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"

#include "dabc/timing.h"

#include <math.h>
#include <stdlib.h>

#include "hadaq/TdcProcessor.h"


stream::TdcCalibrationModule::TdcCalibrationModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProcMgr(0),
   fOwnProcMgr(false),
   fTrbProc(0),
   fDummy(false),
   fReplace(true),
   fAutoCalibr(1000),
   fDummyCounter(0),
   fLastCalibr(),
   fTRB(0),
   fProgress(0),
   fState(),
   fQuality(0)
{
   fLastCalibr.GetNow();

   fDummy = Cfg("Dummy", cmd).AsBool(false);
   fDebug = Cfg("Debug", cmd).AsInt(0);
   fReplace = Cfg("Replace", cmd).AsBool(true);

   fFineMin = Cfg("FineMin", cmd).AsInt();
   fFineMax = Cfg("FineMax", cmd).AsInt();
   if ((fFineMin > 0) && (fFineMax > fFineMin))
      hadaq::TdcMessage::SetFineLimits(fFineMin, fFineMax);

   fNumCh = Cfg("NumChannels", cmd).AsInt(65);
   fEdges = Cfg("EdgeMask", cmd).AsUInt(1);
   fTdcMin = Cfg("TdcMin", cmd).AsUIntVect();
   fTdcMax = Cfg("TdcMax", cmd).AsUIntVect();

   std::vector<uint64_t> caltr = Cfg("CalibrTrigger", cmd).AsUIntVect();

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(fNumCh, fEdges);

   fWorkerHierarchy.Create("Worker");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("Status");
   fState = "Init";

   fTRB = Cfg("TRB", cmd).AsUInt(0x0);
   int portid = cmd.GetInt("portid", 0); // this is portid parameter from hadaq::Factory
   if (fTRB==0) fTRB = 0x8000 | portid;

   item.SetField("trb", fTRB);

   fProcMgr = (stream::DabcProcMgr*) cmd.GetPtr("ProcMgr");
   hadaq::HldProcessor* hld = (hadaq::HldProcessor*) cmd.GetPtr("HLDProc");

   if (!fProcMgr) {
      fOwnProcMgr = true;
      fProcMgr = new stream::DabcProcMgr();
      fProcMgr->SetTop(fWorkerHierarchy);
   }

   int hfill = Cfg("HistFilling", cmd).AsInt(1);

   fTrbProc = new hadaq::TrbProcessor(fTRB, hld, hfill);
   std::vector<uint64_t> hubid = Cfg("HUB", cmd).AsUIntVect();
   for (unsigned n=0;n<hubid.size();n++)
      fTrbProc->AddHadaqHUBId(hubid[n]);
   unsigned hubmin = Cfg("HUBmin", cmd).AsUInt();
   unsigned hubmax = Cfg("HUBmax", cmd).AsUInt();
   for (unsigned n=hubmin;n<hubmax;n++)
      fTrbProc->AddHadaqHUBId(hubid[n]);

   fAutoTdcMode = Cfg("Mode", cmd).AsInt(-1);

   if ((fAutoTdcMode < 0) || fDummy) {
      DOUT0("TRB 0x%04x  creates TDCs %s", (unsigned) fTRB, Cfg("TDC", cmd).AsStr().c_str());
      fTDCs = Cfg("TDC", cmd).AsUIntVect();
      for(unsigned n=0;n<fTDCs.size();n++) {
         fTrbProc->CreateTDC(fTDCs[n]);

         hadaq::TdcProcessor *tdc = fTrbProc->GetTDC(fTDCs[n], true);
         if (fAutoTdcMode == 1) tdc->SetUseLinear(); // force linear
         if (fAutoToTRange > 0) tdc->SetToTRange(10., 30., 60.);
         tdc->UseExplicitCalibration();
      }
      item.SetField("tdc", fTDCs);
   } else {
      if (fAutoTdcMode>=10) {
         fAutoToTRange = fAutoTdcMode / 10;
         fAutoTdcMode = fAutoTdcMode % 10;
      }

      DOUT0("TRB 0x%04x configured in auto mode %d ToT range %d NumCh %d Edges %d", (unsigned) fTRB, fAutoTdcMode, fAutoToTRange, fNumCh, fEdges);

      for (unsigned n=0;n<fTdcMin.size();++n)
         DOUT0("   TDC range 0x%04x - 0x%04x", (unsigned) fTdcMin[n], (unsigned) fTdcMax[n]);
   }

   fCalibrMask = 0xffff;
   if ((caltr.size() > 0) && (caltr[0] != 0xffff)) {
      fCalibrMask = 0;
      for (unsigned n=0;n<caltr.size();n++)
         fCalibrMask |= (1 << caltr[n]);
   }

   fTrbProc->SetCalibrTriggerMask(fCalibrMask);

   fDisabledCh = Cfg("DisableCalibrationFor", cmd).AsIntVect();
   for (unsigned n=0;n<fDisabledCh.size();n++)
      fTrbProc->DisableCalibrationFor(fDisabledCh[n]);

   fAutoCalibr = Cfg("Auto", cmd).AsInt(0);
   if (fDummy && (fAutoCalibr>0)) fAutoCalibr = 1000;
   fTrbProc->SetAutoCalibrations(fAutoCalibr);

   fCountLinear = Cfg("CountLinear", cmd).AsInt(10000);
   fCountNormal = Cfg("CountNormal", cmd).AsInt(100000);

   fCalibrFile = Cfg("CalibrFile", cmd).AsStr();
   if (!fCalibrFile.empty()) {
      if ((fAutoTdcMode < 0) && (fAutoCalibr > 0))
         fTrbProc->SetWriteCalibrations(fCalibrFile.c_str(), true);
      fTrbProc->LoadCalibrations(fCalibrFile.c_str());
   }

   item.SetField("value", fState);

   // set ids and create more histograms
   if (fOwnProcMgr) {
      DOUT0("%s USER PRELLOP NUMCHILDS %u", GetName(), fWorkerHierarchy.NumChilds());
      fProcMgr->UserPreLoop();
      Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));
      // remove pointer, let other modules to create and use it
      base::ProcMgr::ClearInstancePointer();
   }

   // in AutoTDCMode==0 no data is changed, but also no new buffer are required
   if ((fAutoTdcMode==0) && !fReplace) fReplace = true;

   // one need additional buffers
   if (!fReplace) CreatePoolHandle(dabc::xmlWorkPool);

   if (fDebug) CreatePar("DataRate").SetRatemeter(false, 3.).SetUnits("MB");

   DOUT0("TdcCalibrationModule dummy %s autotdc %d histfill %d replace %s", DBOOL(fDummy), fAutoCalibr, hfill, DBOOL(fReplace));
}

stream::TdcCalibrationModule::~TdcCalibrationModule()
{
//   if (fOwnProcMgr && fProcMgr) {
//      fProcMgr->UserPostLoop();
//   }

   fTrbProc = nullptr;
   if (fOwnProcMgr) delete fProcMgr;
   fOwnProcMgr = false;
   fProcMgr = nullptr;
}

void stream::TdcCalibrationModule::SetTRBStatus(dabc::Hierarchy& item, hadaq::TrbProcessor* trb, int *res_progress, double *res_quality, std::string *res_state)
{
   if (item.null() || (trb==0)) return;

   item.SetField("trb", trb->GetID());

   std::vector<int64_t> tdcs;
   std::vector<int64_t> tdc_progr;
   std::vector<double> tdc_quality;
   std::vector<std::string> status;

   double p0(0), p1(1), worse_quality(1), worse_progress(1e10);
   bool ready(true), explicitmode(true), is_any_progress(false);
   std::string worse_status = "Ready"; // overall status from all TDCs

   for (unsigned n=0;n<trb->NumberOfTDC();n++) {
      hadaq::TdcProcessor* tdc = trb->GetTDCWithIndex(n);

      if (tdc!=0) {

         double progr = tdc->GetCalibrProgress();
         std::string sname = tdc->GetCalibrStatus();
         double quality = tdc->GetCalibrQuality();
         int mode = tdc->GetExplicitCalibrationMode();

         // DOUT0("TDC 0x%04x mode %d Progress %5.4f Quality %5.4f state %s", tdc->GetID(), mode, progr, quality, sname.c_str());

         if (quality < worse_quality) {
            worse_quality = quality;
            worse_status = sname;
         }

         if (mode < 0) {
            explicitmode = false;
            // auto calibration
            if (sname.find("Ready")==0) {
               if (p1 > progr) p1 = progr;
            } else {
               if (p0 < progr) p0 = progr;
               ready = false;
            }
         } else {
            if ((progr > 0) && (progr < worse_progress)) {
               worse_progress = progr;
               is_any_progress = true;
            }
         }

         tdcs.push_back(tdc->GetID());
         tdc_progr.push_back((int) (progr*100.));
         status.push_back(sname);
         tdc_quality.push_back(quality);
      } else {
         tdcs.push_back(0);
         tdc_progr.push_back(0);
         status.push_back("Init");
         tdc_quality.push_back(0);
      }
   }

   if (!explicitmode) {
      worse_progress = ready ? p1 : -p0;
      is_any_progress = true;
      // at the end check if auto-calibration can be done
      if (worse_progress > 0) {
         worse_status = "Ready";
      } else {
         worse_status = "Init";
      }
   }

   if (!is_any_progress) worse_progress = 0;

   item.SetField("value", worse_status);
   item.SetField("progress", (int)(fabs(worse_progress)*100));
   item.SetField("quality", worse_quality);
   item.SetField("tdc", tdcs);
   item.SetField("tdc_progr", tdc_progr);
   item.SetField("tdc_status", status);
   item.SetField("tdc_quality", tdc_quality);

//   DOUT0("Calibr Quality %5.4f Progress %5.4f", progress, worse_quality);

   if (res_progress) *res_progress = (int) (fabs(worse_progress)*100);
   if (res_quality) *res_quality = worse_quality;
   if (res_state) *res_state = worse_status;
}


bool stream::TdcCalibrationModule::retransmit()
{
   // method reads one buffer, calibrate it and send further

   // dabc::ProfilerGuard grd(fProfiler, "checks", 0);

   // nothing to do
   if (CanSend() && CanRecv()) {

      if (!fReplace && !CanTakeBuffer()) return false;

      dabc::Buffer buf = Recv();

      if (fDebug) Par("DataRate").SetValue(buf.GetTotalSize()/1024./1024.);

      if (fDummy && false) {

         dabc::Hierarchy item = fWorkerHierarchy.GetHChild("Status");

         fDummyCounter++;

         fProgress = 0;
         if (fAutoCalibr>0) fProgress = (int) (100*fDummyCounter/fAutoCalibr); else
         if (fDoingTdcCalibr) fProgress = (int) (100*fDummyCounter/1000);
         item.SetField("progress", fProgress);

         fQuality = 0;
         if (fProgress > 0) fQuality = 0.7 + fProgress*1e-3;

         if (fProgress >= 100) {
            fQuality = 1;
            if (fAutoCalibr>0) fDummyCounter = 0;
            fState = "Ready";
            item.SetField("value", fState);
            item.SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
         }

         std::vector<int64_t> progr;
         progr.assign(fTDCs.size(), fProgress);
         item.SetField("tdc_progr", progr);

         std::vector<std::string> status;
         status.assign(fTDCs.size(), item.GetField("value").AsStr());
         item.SetField("tdc_status", status);

         std::vector<double> tdc_quality;
         tdc_quality.assign(fTDCs.size(), fQuality);
         item.SetField("tdc_quality", tdc_quality);

         hadaq::ReadIterator iter(buf);
         while (iter.NextSubeventsBlock()) {
            while (iter.NextSubEvent()) {
               iter.subevnt()->SetId(fTRB);
            }
         }

         item.SetField("quality", fQuality);

      } else if (fTrbProc) {

         if ((buf.GetTypeId() == hadaq::mbt_HadaqEvents) ||  // this is debug mode when processing events from the file
             (buf.GetTypeId() == hadaq::mbt_HadaqTransportUnit) || // this is normal operation mode
             (buf.GetTypeId() == hadaq::mbt_HadaqSubevents) || fDummy) { // this could be data after sorting

            // grd.Next("auto");

            // this is special case when TDC should be created

            fDummyCounter++;

            bool auto_create = (fAutoTdcMode > 0) && (fTDCs.size() == 0) && (fTdcMin.size() > 0) && !fDummy;

            if (auto_create) {
               // special loop over data to create missing TDCs

               hadaq::ReadIterator iter0(buf);
               // take only first event - all other ignored
               if (iter0.NextSubeventsBlock()) {
                  while (iter0.NextSubEvent()) {
                     if (iter0.subevnt()->GetPaddedSize() > iter0.rawdata_maxsize()) {
                        EOUT("Creating TDCs HUB %u Wrong subevent header len %u maximial %u", fTrbProc->GetID(), iter0.subevnt()->GetPaddedSize(), iter0.rawdata_maxsize());
                        break;
                     }
                     fTrbProc->CreateMissingTDC((hadaqs::RawSubevent*)iter0.subevnt(), fTdcMin, fTdcMax, fNumCh, fEdges);
                  }
               }

               fTrbProc->SetCalibrTriggerMask(fCalibrMask);

               for (unsigned n=0;n<fDisabledCh.size();n++)
                  fTrbProc->DisableCalibrationFor(fDisabledCh[n]);

               fTrbProc->SetAutoCalibrations(fAutoCalibr);

               if (!fCalibrFile.empty()) {
                  if (fAutoCalibr > 0)
                      fTrbProc->SetWriteCalibrations(fCalibrFile.c_str(), true);
                  fTrbProc->LoadCalibrations(fCalibrFile.c_str());
               }

               unsigned num = fTrbProc->NumberOfTDC();
               for (unsigned indx=0;indx<num;++indx) {
                  hadaq::TdcProcessor *tdc = fTrbProc->GetTDCWithIndex(indx);

                  if (fAutoTdcMode==1) tdc->SetUseLinear(); // force linear
                  if (fAutoToTRange==1) tdc->SetToTRange(20., 30., 60.); // special mode for DiRICH

                  tdc->UseExplicitCalibration();

                  fTDCs.emplace_back(tdc->GetID());
                  DOUT0("TRB 0x%04x created TDC 0x%04x", (unsigned) fTRB, tdc->GetID());
               }

               if (num > 0) fTrbProc->CreatePerTDCHistos();

               // set field with TDCs
               fWorkerHierarchy.GetHChild("Status").SetField("tdc", fTDCs);

               if (num==0) EOUT("No any TDC found");

               if (fDebug == 2) {
                  // just start explicit calculations
                  dabc::Command cmd("TdcCalibrations");
                  cmd.SetStr("mode", "start");
                  ExecuteCommand(cmd);
               }
            }

            // grd.Next("buf");

            // from here starts transformation of the data

            unsigned char* tgt = nullptr;
            unsigned tgtlen(0), reslen(0);
            dabc::Buffer resbuf;
            if (!fReplace) {
               resbuf = TakeBuffer();
               tgt = (unsigned char*) resbuf.SegmentPtr();
               tgtlen = resbuf.SegmentSize();
            }

            // grd.Next("main");

            hadaq::ReadIterator iter(buf);
            while (iter.NextSubeventsBlock()) {
               while (iter.NextSubEvent()) {

                  if (iter.subevnt()->GetPaddedSize() > iter.rawdata_maxsize()) {
                     EOUT("TransferData HUB %u Wrong subevent header len %u maximial %u", fTrbProc->GetID(), iter.subevnt()->GetPaddedSize(), iter.rawdata_maxsize());
                     break;
                  }

                  if (tgt && (tgtlen - reslen < iter.subevnt()->GetPaddedSize())) {
                     EOUT("Not enough space for subevent sz %u in output buffer sz %u seg0 %u filled %u remains in src %u", iter.subevnt()->GetPaddedSize(), resbuf.GetTotalSize(), tgtlen, reslen, iter.remained_size());
                     exit(4);
                     return false;
                  }

                  unsigned sublen = 0;

                  // sublen = iter.subevnt()->GetPaddedSize();

                  if (fDummy) {
                     iter.subevnt()->SetId(fTRB);
                     sublen = fTrbProc->EmulateTransform((hadaqs::RawSubevent*)iter.subevnt(), fDummyCounter);
                  } else {
                     sublen = fTrbProc->TransformSubEvent((hadaqs::RawSubevent*)iter.subevnt(), tgt, tgtlen - reslen, (fAutoTdcMode==0));
                  }

                  if (tgt) {
                     tgt += sublen;
                     reslen += sublen;
                  }
               }
            }

            // grd.Next("finish");

            if (!fReplace) {
               resbuf.SetTotalSize(reslen);
               resbuf.SetTypeId(hadaq::mbt_HadaqSubevents);
               buf = resbuf;
            }

            if (fLastCalibr.Expired(1.)) {
               fLastCalibr.GetNow();

               dabc::Hierarchy item = fWorkerHierarchy.GetHChild("Status");

               SetTRBStatus(item, fTrbProc, &fProgress, &fQuality, &fState);

               // DOUT0("%s PROGR %d QUALITY %5.3f STATE %s", GetName(), fProgress, fQuality, fState.c_str());

               // if (fProgress>0) fState = "Ready";
            }
         } else {
            EOUT("Error buffer type!!! %d", buf.GetTypeId());
         }
      }

      Send(buf);

      return true;
   }

   return false;
}


int stream::TdcCalibrationModule::ExecuteCommand(dabc::Command cmd)
{
   if (fOwnProcMgr && fProcMgr && fProcMgr->ExecuteHCommand(cmd)) return dabc::cmd_true;

   if (cmd.IsName("ResetExportedCounters") || cmd.IsName("ResetTransportStat")) {
      // redirect command to real transport
      if (SubmitCommandToTransport(InputName(), cmd)) return dabc::cmd_postponed;
      return dabc::cmd_false;
   }

   if (cmd.IsName("GetCalibrState")) {
      cmd.SetUInt("trb", fTRB);
      cmd.SetField("tdcs", fTDCs);
      cmd.SetInt("progress", fProgress);
      cmd.SetDouble("quality", fQuality);
      cmd.SetStr("state", fState);
      return dabc::cmd_true;
   }

   if (cmd.IsName("GetHadaqTransportInfo")) {
      // redirect command to real transport
      cmd.SetStr("CalibrModule", ItemName());
      if (SubmitCommandToTransport(InputName(), cmd)) return dabc::cmd_postponed;
      return dabc::cmd_true;
   }

   if (cmd.IsName("TdcCalibrations")) {
      fDummyCounter = 0; // only for debugging

      fDoingTdcCalibr = (cmd.GetStr("mode") == "start");

      std::string subdir = cmd.GetStr("rundir");

      DOUT0("%s ENTER CALIBRATION Mode %s subdir %s\n", GetName(), cmd.GetStr("mode").c_str(), subdir.c_str());
      if (!subdir.empty()) subdir.append(fCalibrFile);

      unsigned numtdc = fTrbProc->NumberOfTDC();

      if (cmd.GetStr("mode") == "start")
         DOUT0("%s START CALIBRATIONS autotdc %d NumTDC %u", GetName(), fAutoTdcMode, numtdc);
      else
         DOUT0("%s STORE CALIBRATIONS IN %s %s NumTDC %u", GetName(), fCalibrFile.c_str(), subdir.c_str(), numtdc);

      for (unsigned indx=0;indx<numtdc;++indx) {
         hadaq::TdcProcessor *tdc = fTrbProc->GetTDCWithIndex(indx);

         if (cmd.GetStr("mode") == "start") {
            tdc->BeginCalibration(fAutoTdcMode==1 ? fCountLinear : fCountNormal);
         } else {

            std::string s1 = dabc::format("BEFORE mode %d Progress %5.4f Quality %5.4f state %s", tdc->GetExplicitCalibrationMode(), tdc->GetCalibrProgress(), tdc->GetCalibrQuality(), tdc->GetCalibrStatus().c_str());

            tdc->CompleteCalibration(fDummy, fCalibrFile, subdir);

            std::string s2 = dabc::format("AFTER mode %d Progress %5.4f Quality %5.4f state %s", tdc->GetExplicitCalibrationMode(), tdc->GetCalibrProgress(), tdc->GetCalibrQuality(), tdc->GetCalibrStatus().c_str());

            DOUT0("TDC %04x edges %u %s %s", tdc->GetID(), tdc->GetEdgeMask(), s1.c_str(), s2.c_str());
         }
      }

      fLastCalibr.GetNow();
      dabc::Hierarchy item = fWorkerHierarchy.GetHChild("Status");
      SetTRBStatus(item, fTrbProc, &fProgress, &fQuality, &fState);

      cmd.SetDouble("quality", fQuality);

      DOUT0("RESULT!!! %s PROGR %d QUALITY %5.3f STATE %s", GetName(), fProgress, fQuality, fState.c_str());

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


void stream::TdcCalibrationModule::BeforeModuleStart()
{
   // fProfiler.Reserve(50);
   // fProfiler.MakeStatistic();
}


void stream::TdcCalibrationModule::AfterModuleStop()
{
   //fProfiler.MakeStatistic();
   //DOUT0("PROFILER %s", fProfiler.Format().c_str());

   if (fDebug == 2) {
      // just start explicit calculations
      dabc::Command cmd("TdcCalibrations");
      cmd.SetStr("mode", "stop");
      ExecuteCommand(cmd);
   }

}

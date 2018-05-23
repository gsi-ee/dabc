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

#include "hadaq/TdcProcessor.h"


stream::TdcCalibrationModule::TdcCalibrationModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProcMgr(0),
   fOwnProcMgr(false),
   fTrbProc(0),
   fDummy(true),
   fReplace(true),
   fAutoCalibr(1000),
   fDummyCounter(0),
   fLastCalibr(),
   fTRB(0),
   fProgress(0),
   fState()
{
   fLastCalibr.GetNow();

   fDummy = Cfg("Dummy", cmd).AsBool(false);
   fReplace = Cfg("Replace", cmd).AsBool(true);

   fFineMin = Cfg("FineMin", cmd).AsInt();
   fFineMax = Cfg("FineMax", cmd).AsInt();
   if ((fFineMin > 0) && (fFineMax > fFineMin))
      hadaq::TdcMessage::SetFineLimits(fFineMin, fFineMax);

   fNumCh = Cfg("NumChannels", cmd).AsInt(65);
   fEdges = Cfg("EdgeMask", cmd).AsInt(1);
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

   if (fAutoTdcMode < 0) {
      DOUT0("TRB 0x%04x  creates TDCs %s", (unsigned) fTRB, Cfg("TDC", cmd).AsStr().c_str());
      fTDCs = Cfg("TDC", cmd).AsUIntVect();
      for(unsigned n=0;n<fTDCs.size();n++)
         fTrbProc->CreateTDC(fTDCs[n]);
      item.SetField("tdc", fTDCs);
   } else {
      DOUT0("TRB 0x%04x configured in auto mode %d", (unsigned) fTRB, fAutoTdcMode);
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
   if (fDummy) fAutoCalibr = 1000;
   fTrbProc->SetAutoCalibrations(fAutoCalibr);

   fCalibrFile = Cfg("CalibrFile", cmd).AsStr();
   if (!fCalibrFile.empty()) {
      fTrbProc->SetWriteCalibrations(fCalibrFile.c_str(), true);
      if (fTrbProc->LoadCalibrations(fCalibrFile.c_str()) || fDummy) {
         fState = "File";
         item.SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
      }
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


   DOUT0("TdcCalibrationModule dummy %s autotdc %d histfill %d replace %s", DBOOL(fDummy), fAutoCalibr, hfill, DBOOL(fReplace));
}

stream::TdcCalibrationModule::~TdcCalibrationModule()
{
   fTrbProc = 0;
   if (fOwnProcMgr) delete fProcMgr;
   fOwnProcMgr = false;
   fProcMgr = 0;
}

double stream::TdcCalibrationModule::SetTRBStatus(dabc::Hierarchy& item, hadaq::TrbProcessor* trb)
{
   if (item.null() || (trb==0)) return 0.;

   item.SetField("trb", trb->GetID());

   std::vector<int64_t> tdcs;
   std::vector<int64_t> tdc_progr;
   std::vector<std::string> status;

   double p0(0), p1(1);
   bool ready(true);

   for (unsigned n=0;n<trb->NumberOfTDC();n++) {
      hadaq::TdcProcessor* tdc = trb->GetTDCWithIndex(n);

      if (tdc!=0) {

         double progr = tdc->GetCalibrProgress();

         if (tdc->GetCalibrStatus().find("Ready")==0) {
            if (p1 > progr) p1 = progr;
         } else {
            if (p0 < progr) p0 = progr;
            ready = false;
         }

         tdcs.push_back(tdc->GetID());
         tdc_progr.push_back((int) (progr*100.));
         status.push_back(tdc->GetCalibrStatus());
      } else {
         tdcs.push_back(0);
         tdc_progr.push_back(0);
         status.push_back("Init");
      }
   }

   double progress = ready ? p1 : -p0;

   // at the end check if auto-calibration can be done
   if (progress>0) {
      item.SetField("value", "Ready");
      item.SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
   } else {
      item.SetField("value", "Init");
   }

   item.SetField("progress", (int)(fabs(progress)*100));
   item.SetField("tdc", tdcs);
   item.SetField("tdc_progr", tdc_progr);
   item.SetField("tdc_status", status);

   return progress;
}



bool stream::TdcCalibrationModule::retransmit()
{
   // method reads one buffer, calibrate it and send further

   // nothing to do
   if (CanSend() && CanRecv()) {

      if (!fReplace && !CanTakeBuffer()) return false;

      dabc::Buffer buf = Recv();

      if (fDummy) {

         dabc::Hierarchy item = fWorkerHierarchy.GetHChild("Status");

         fDummyCounter++;
         if ((fDummyCounter>fAutoCalibr) && (fAutoCalibr>0)) {
            fDummyCounter = 0;
            fState = "Ready";
            item.SetField("value", fState);
            item.SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
         }

         fProgress = 0;
         if (fAutoCalibr>0) fProgress = (int) (100*fDummyCounter/fAutoCalibr);

         item.SetField("progress", fProgress);

         std::vector<int64_t> progr;
         progr.assign(fTDCs.size(), fProgress);
         item.SetField("tdc_progr", progr);

         std::vector<std::string> status;
         status.assign(fTDCs.size(),  item.GetField("value").AsStr());
         item.SetField("tdc_status", status);

         hadaq::ReadIterator iter(buf);
         while (iter.NextSubeventsBlock()) {
            while (iter.NextSubEvent()) {
               iter.subevnt()->SetId(fTRB);
            }
         }

      } else if (fTrbProc) {

         if ((buf.GetTypeId() == hadaq::mbt_HadaqEvents) ||  // this is debug mode when processing events from the file
             (buf.GetTypeId() == hadaq::mbt_HadaqTransportUnit) || // this is normal operation mode
             (buf.GetTypeId() == hadaq::mbt_HadaqSubevents)) { // this could be data after sorting

            // this is special case when TDC should be created

            bool auto_create = (fAutoTdcMode > 0) && (fTDCs.size() == 0) && (fTdcMin.size() > 0);

            if (auto_create) {
               // special loop over data to create missing TDCs

               hadaq::ReadIterator iter0(buf);
               // take only first event - all other ignored
               if (iter0.NextSubeventsBlock()) {
                  while (iter0.NextSubEvent()) {
                     fTrbProc->CreateMissingTDC((hadaqs::RawSubevent*)iter0.subevnt(), fTdcMin, fTdcMax, fNumCh, fEdges);
                  }
               }

               fTrbProc->SetCalibrTriggerMask(fCalibrMask);

               for (unsigned n=0;n<fDisabledCh.size();n++)
                  fTrbProc->DisableCalibrationFor(fDisabledCh[n]);

               fTrbProc->SetAutoCalibrations(fAutoCalibr);

               if (!fCalibrFile.empty()) {
                  fTrbProc->SetWriteCalibrations(fCalibrFile.c_str(), true);
                  if (fTrbProc->LoadCalibrations(fCalibrFile.c_str())) {
                     fState = "File";
                     fWorkerHierarchy.GetHChild("Status").SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
                  }
               }

               unsigned num = fTrbProc->NumberOfTDC();
               for (unsigned indx=0;indx<num;++indx) {
                  hadaq::TdcProcessor *tdc = fTrbProc->GetTDCWithIndex(indx);

                  if (fAutoTdcMode==1) tdc->SetUseLinear(); // force linear

                  fTDCs.emplace_back(tdc->GetID());
                  DOUT0("TRB 0x%04x created TDC 0x%04x", (unsigned) fTRB, tdc->GetID());
               }

               fTrbProc->CreatePerTDCHistos();

               // set field with TDCs
               fWorkerHierarchy.GetHChild("Status").SetField("tdc", fTDCs);

               if (num==0) EOUT("No any TDC found");
            }


            // from here starts transformation of the data

            unsigned char* tgt = nullptr;
            unsigned tgtlen(0), reslen(0);
            dabc::Buffer resbuf;
            if (!fReplace) {
               resbuf = TakeBuffer();
               tgt = (unsigned char*) resbuf.SegmentPtr();
               tgtlen = resbuf.SegmentSize();
            }

            hadaq::ReadIterator iter(buf);
            while (iter.NextSubeventsBlock()) {
               while (iter.NextSubEvent()) {
                  if (tgt && (tgtlen - reslen < iter.subevnt()->GetPaddedSize())) {
                     EOUT("Not enough space for subevent in output buffer");
                     exit(4); return false;
                  }

                  unsigned sublen = fTrbProc->TransformSubEvent((hadaqs::RawSubevent*)iter.subevnt(), tgt, tgtlen - reslen, (fAutoTdcMode==0));
                  if (tgt) {
                     tgt += sublen;
                     reslen += sublen;
                  }
               }
            }

            if (!fReplace) {
               resbuf.SetTotalSize(reslen);
               resbuf.SetTypeId(hadaq::mbt_HadaqSubevents);
               buf = resbuf;
            }

            if (fLastCalibr.Expired(1.)) {
               fLastCalibr.GetNow();

               dabc::Hierarchy item = fWorkerHierarchy.GetHChild("Status");

               double p = SetTRBStatus(item, fTrbProc);
               fProgress = (int) (p*100);
               if (fProgress>0) fState = "Ready";
            }
         } else {
            EOUT("Error buffer type!!!");
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

   if (cmd.IsName("ResetExportedCounters") ) {
      // redirect command to real transport
      if (SubmitCommandToTransport(InputName(), cmd)) return dabc::cmd_postponed;
   }

   if (cmd.IsName("GetCalibrState")) {
      cmd.SetUInt("trb", fTRB);
      cmd.SetField("tdcs", fTDCs);
      cmd.SetInt("progress", fProgress);
      cmd.SetStr("state", fState);
      return dabc::cmd_true;
   }

   if (cmd.IsName("GetHadaqTransportInfo")) {
      // redirect command to real transport
      cmd.SetStr("CalibrModule", ItemName());
      if (SubmitCommandToTransport(InputName(), cmd)) return dabc::cmd_postponed;
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

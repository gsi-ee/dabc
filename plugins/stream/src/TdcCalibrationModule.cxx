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


stream::TdcCalibrationModule::TdcCalibrationModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProcMgr(0),
   fTrbProc(0),
   fDummy(true),
   fAutoCalibr(1000),
   fDummyCounter(0),
   fLastCalibr(),
   fTRB(0),
   fProgress(0),
   fState()
{
   // we need one input and one output port
   EnsurePorts(1, 1);

   fLastCalibr.GetNow();

   fDummy = Cfg("Dummy", cmd).AsBool(false);

   int finemin = Cfg("FineMin", cmd).AsInt(0);
   int finemax = Cfg("FineMax", cmd).AsInt(0);
   if ((finemin>0) && (finemax==0))
      hadaq::TdcMessage::SetFineLimits(finemin, finemax);

   int numch = Cfg("NumChannels", cmd).AsInt(65);
   int edges = Cfg("EdgeMask", cmd).AsInt(1);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(numch, edges);

   fWorkerHierarchy.Create("Worker");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("Status");
   fState = "Init";

   fTRB = Cfg("TRB", cmd).AsUInt(0x0);
   int portid = cmd.GetInt("portid", 0); // this is portid parameter from hadaq::Factory
   if (fTRB==0) fTRB = 0x8000 | portid;

   item.SetField("trb", fTRB);

   fProcMgr = new stream::DabcProcMgr();
   fProcMgr->SetTop(fWorkerHierarchy);

   fTrbProc = new hadaq::TrbProcessor(fTRB);
   std::vector<uint64_t> hubid = Cfg("HUB", cmd).AsUIntVect();
   for (unsigned n=0;n<hubid.size();n++)
      fTrbProc->AddHadaqHUBId(hubid[n]);

   int hfill = Cfg("HistFilling", cmd).AsInt(1);

   fTrbProc->SetHistFilling(hfill);

   DOUT2("Create TDCs %s", Cfg("TDC", cmd).AsStr().c_str());

   fTDCs = Cfg("TDC", cmd).AsUIntVect();
   for(unsigned n=0;n<fTDCs.size();n++) {
      DOUT2("Create TDC 0x%04x", (unsigned) fTDCs[n]);
      fTrbProc->CreateTDC(fTDCs[n]);
   }
   item.SetField("tdc", fTDCs);

   std::vector<int64_t> dis_ch = Cfg("DisableCalibrationFor", cmd).AsIntVect();
   for (unsigned n=0;n<dis_ch.size();n++)
      fTrbProc->DisableCalibrationFor(dis_ch[n]);

   fAutoCalibr = Cfg("Auto", cmd).AsInt(0);
   fTrbProc->SetAutoCalibrations(fAutoCalibr);

   std::string calibrfile = Cfg("CalibrFile", cmd).AsStr();
   if (!calibrfile.empty()) {
      fTrbProc->SetWriteCalibrations(calibrfile.c_str(), true);
      if (fTrbProc->LoadCalibrations(calibrfile.c_str()) || fDummy) {
         fState = "File";
         item.SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
      }
   }

   item.SetField("value", fState);

   // set ids and create more histograms
   fProcMgr->UserPreLoop();

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));

   // remove pointer, let other modules to create and use it
   base::ProcMgr::ClearInstancePointer();

   DOUT0("TdcCalibrationModule dummy %s auto %d", DBOOL(fDummy), fAutoCalibr);
}

stream::TdcCalibrationModule::~TdcCalibrationModule()
{
   fTrbProc = 0;
   if (fProcMgr) {
      delete fProcMgr;
      fProcMgr = 0;
   }
}

bool stream::TdcCalibrationModule::retransmit()
{
   // method reads one buffer, calibrate it and send further

   // nothing to do
   if (CanSend() && CanRecv()) {

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

      } else
      if (fTrbProc!=0) {

         if ((buf.GetTypeId() == hadaq::mbt_HadaqEvents) ||  // this is debug mode when processing events from the file
             (buf.GetTypeId() == hadaq::mbt_HadaqTransportUnit)) { // this is normal operation mode

            hadaq::ReadIterator iter(buf);
            while (iter.NextSubeventsBlock()) {
               while (iter.NextSubEvent()) {
                  fTrbProc->TransformSubEvent((hadaqs::RawSubevent*)iter.subevnt());
               }
            }

            if (fLastCalibr.Expired(1.)) {

               double progress = fTrbProc->CheckAutoCalibration();
               dabc::Hierarchy item = fWorkerHierarchy.GetHChild("Status");
               fProgress = (int) fabs(progress*100);
               item.SetField("progress", fProgress);

               // at the end check if autocalibration can be done
               if (progress>0) {
                  fState = "Ready";
                  item.SetField("value", fState);
                  item.SetField("time", dabc::DateTime().GetNow().OnlyTimeAsString());
               }

               std::vector<int64_t> progr;
               std::vector<std::string> status;

               for (unsigned n=0;n<fTDCs.size();n++) {
                  hadaq::TdcProcessor* tdc = fTrbProc->GetTDC(fTDCs[n]);

                  if (tdc!=0) {
                     progr.push_back((int) (tdc->GetCalibrProgress()*100.));
                     status.push_back(tdc->GetCalibrStatus());
                  } else {
                     progr.push_back(0);
                     status.push_back("Init");
                  }
               }

               item.SetField("tdc_progr", progr);
               item.SetField("tdc_status", status);

               fLastCalibr.GetNow();
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
   if (fProcMgr && fProcMgr->ExecuteHCommand(cmd)) return dabc::cmd_true;

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

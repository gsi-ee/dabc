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

#include "hadaq/TdcCalibrationModule.h"
#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"
#include "dabc/timing.h"

#include <math.h>


#ifdef WITH_STREAM
#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcProcessor.h"

DabcProcMgr::DabcProcMgr() :
   base::ProcMgr(),
   fTop()
{
}

DabcProcMgr::~DabcProcMgr()
{
}

base::H1handle DabcProcMgr::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   DOUT2("Create TH1 %s", name);

   dabc::Hierarchy h = fTop.CreateHChild(name);
   if (h.null()) return 0;

   h.SetField("_kind","ROOT.TH1D");
   h.SetField("_title", title);
   h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
   h.SetField("nbins", nbins);
   h.SetField("left", left);
   h.SetField("right", right);

   std::vector<double> bins;
   bins.resize(nbins+3, 0.);
   bins[0] = nbins;
   bins[1] = left;
   bins[2] = right;
   h.SetField("bins", bins);

   return h.GetFieldPtr("bins")->GetDoubleArr();
}

void DabcProcMgr::FillH1(base::H1handle h1, double x, double weight)
{
   if (h1==0) return;
   double* arr = (double*) h1;

   int nbin = (int) arr[0];
   int bin = (int) /*floor*/ (nbin * (x - arr[1]) / (arr[2] - arr[1]));

   if ((bin>=0) && (bin<nbin)) arr[bin+3]+=weight;
}

double DabcProcMgr::GetH1Content(base::H1handle h1, int nbin)
{
   if (h1==0) return 0.;
   double* arr = (double*) h1;
   return (nbin>=0) && (nbin<arr[0]) ? arr[nbin+3] : 0;
}

void DabcProcMgr::ClearH1(base::H1handle h1)
{
   if (h1==0) return;
   double* arr = (double*) h1;
   for (int n=0;n<arr[0];n++) arr[n+3] = 0.;
}

base::H2handle DabcProcMgr::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   DOUT2("Create TH2 %s", name);

   dabc::Hierarchy h = fTop.CreateHChild(name);
   if (h.null()) return 0;

   h.SetField("_kind","ROOT.TH2D");
   h.SetField("_title", title);
   h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
   h.SetField("nbins1", nbins1);
   h.SetField("left1", left1);
   h.SetField("right1", right1);
   h.SetField("nbins2", nbins2);
   h.SetField("left2", left2);
   h.SetField("right2", right2);

   std::vector<double> bins;
   bins.resize(nbins1*nbins2+6, 0.);
   bins[0] = nbins1;
   bins[1] = left1;
   bins[2] = right1;
   bins[3] = nbins2;
   bins[4] = left2;
   bins[5] = right2;
   h.SetField("bins", bins);

   return h.GetFieldPtr("bins")->GetDoubleArr();
}

void DabcProcMgr::FillH2(base::H1handle h2, double x, double y, double weight)
{
   if (h2==0) return;
   double* arr = (double*) h2;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];

   int bin1 = (int) /*floor*/ (nbin1 * (x - arr[1]) / (arr[2] - arr[1]));
   int bin2 = (int) /*floor*/(nbin2 * (y - arr[4]) / (arr[5] - arr[4]));

   if ((bin1>=0) && (bin1<nbin1) && (bin2>=0) && (bin2<nbin2))
      arr[bin1 + bin2*nbin1 + 6]+=weight;
}

void DabcProcMgr::ClearH2(base::H2handle h2)
{
   if (h2==0) return;
   double* arr = (double*) h2;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];
   for (int n=0;n<nbin1*nbin2;n++) arr[n+6] = 0.;
}


#endif


hadaq::TdcCalibrationModule::TdcCalibrationModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProcMgr(0),
   fTrbProc(0),
   fDummy(true),
   fAutoCalibr(1000),
   fDummyCounter(0),
   fLastCalibr(),
   fProgress(0),
   fState()
{
   // we need one input and one output port
   EnsurePorts(1, 1);

   fLastCalibr.GetNow();

   fDummy = Cfg("Dummy", cmd).AsBool(false);

#ifdef WITH_STREAM

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

   fProcMgr = new DabcProcMgr;

   fProcMgr->fTop = fWorkerHierarchy;

   fTrbProc = new hadaq::TrbProcessor(fTRB);
   int hubid = Cfg("HUB", cmd).AsInt(0x0);
   if (hubid>0) fTrbProc->SetHadaqHUBId(hubid);

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
#endif

   DOUT0("TdcCalibrationModule dummy %s", DBOOL(fDummy));

}

hadaq::TdcCalibrationModule::~TdcCalibrationModule()
{
#ifdef WITH_STREAM
   fTrbProc = 0;
   delete fProcMgr;
   fProcMgr = 0;
#endif
}

bool hadaq::TdcCalibrationModule::retransmit()
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

         if ((buf.GetTypeId() == hadaq::mbt_HadaqEvents) || (buf.GetTypeId() == hadaq::mbt_HadaqTransportUnit)) {
            // this is debug mode when processing events from the file
#ifdef WITH_STREAM

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
#endif
         } else {
            EOUT("Error buffer type!!!");
         }
      }
      Send(buf);

      return true;
   }

   return false;
}


int hadaq::TdcCalibrationModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ResetExportedCounters") ) {
      // redirect command to real transport
      if (SubmitCommandToTransport(InputName(), cmd)) return dabc::cmd_postponed;
   }

   if (cmd.IsName("GetHadaqTransportInfo")) {
      // redirect command to real transport
      cmd.SetPtr("CalibrModule", this);
      if (SubmitCommandToTransport(InputName(), cmd)) return dabc::cmd_postponed;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

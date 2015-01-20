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
   DOUT0("Create TH1 %s", name);

   dabc::Hierarchy h = fTop.CreateHChild(name);
   if (h.null()) return 0;

   h.SetField("_kind","ROOT.TH1D");
   h.SetField("_title", title);
   h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
   h.SetField("nbins", nbins);
   h.SetField("left", left);
   h.SetField("right", right);

   std::vector<double> bins;
   bins.resize(nbins+3, 0);
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

   double bin = (x - arr[1]) / (arr[2] - arr[1]) * arr[0];

   if ((bin>=0.) && (bin<arr[0]-0.75)) arr[(int)(bin+3)]+=weight;
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
   for (int n=0;n<arr[0];n++) arr[n+3] = 0;
}

#endif


hadaq::TdcCalibrationModule::TdcCalibrationModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProcMgr(0),
   fTrbProc(0)
{
   // we need one input and one output port
   EnsurePorts(1, 1);


#ifdef WITH_STREAM

   fWorkerHierarchy.Create("Worker");

   fProcMgr = new DabcProcMgr;

   fProcMgr->fTop = fWorkerHierarchy;

   fTrbProc = new hadaq::TrbProcessor(0x0);

   std::vector<int64_t> tdcs = Cfg("TDC", cmd).AsIntVect();
   for(unsigned n=0;n<tdcs.size();n++)
      fTrbProc->CreateTDC(tdcs[n]);

   fTrbProc->SetTDCCorrectionMode(1);

   int autocal = Cfg("Auto", cmd).AsInt();
   if (autocal>1000)
      fTrbProc->SetAutoCalibrations(autocal);

   std::string calfile = Cfg("CalibrFile", cmd).AsStr();
   if (!calfile.empty()) {
      fTrbProc->LoadCalibrations(calfile.c_str());
      fTrbProc->SetWriteCalibrations(calfile.c_str(), true);
   }

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));

#endif

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

      if (fTrbProc!=0) {

         if (buf.GetTypeId() == hadaq::mbt_HadaqEvents) {
            // this is debug mode when processing events from the file
#ifdef WITH_STREAM

            hadaq::ReadIterator iter(buf);
            while (iter.NextEvent()) {
               while (iter.NextSubEvent()) {
                  fTrbProc->TransformSubEvent((hadaqs::RawSubevent*)iter.subevnt());
               }
            }
            // at the end check if autocalibration can be done
            if (fTrbProc->CheckAutoCalibration())
               DOUT0("MADE CALIBRATION!!!");
#endif

         } else
         if (buf.GetTypeId() == hadaq::mbt_HadaqTransportUnit) {

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
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

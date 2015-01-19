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


#ifdef WITH_STREAM
#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcProcessor.h"

DabcProcMgr::DabcProcMgr() :
   base::ProcMgr()
{
}

DabcProcMgr::~DabcProcMgr()
{
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
   fProcMgr = new DabcProcMgr;

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

            hadaq::ReadIterator iter(buf);
            while (iter.NextEvent()) {
               while (iter.NextSubEvent()) {
#ifdef WITH_STREAM
                  fTrbProc->CalibrateSubEvent((hadaqs::RawSubevent*)iter.subevnt());
#endif
               }
            }
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

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

#ifndef HADAQ_TdcCalibrationModule
#define HADAQ_TdcCalibrationModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifdef WITH_STREAM

#include "base/ProcMgr.h"

class DabcProcMgr : public base::ProcMgr {
   protected:

   public:
      dabc::Hierarchy fTop;

      DabcProcMgr();
      virtual ~DabcProcMgr();

      virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);
      virtual void FillH1(base::H1handle h1, double x, double weight = 1.);
      virtual double GetH1Content(base::H1handle h1, int nbin);
      virtual void ClearH1(base::H1handle h1);

      virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);
      virtual void FillH2(base::H1handle h2, double x, double y, double weight = 1.);
      virtual void ClearH2(base::H2handle h2);

};

#endif


class DabcProcMgr;

namespace hadaq {

   class TrbProcessor;

/** \brief Perform calibration of FPGA TDC data
 *
 * Module extracts TDC hits and replace fine counter with calibration values.
 */

   class TdcCalibrationModule : public dabc::ModuleAsync {

   protected:

      DabcProcMgr* fProcMgr;
      hadaq::TrbProcessor* fTrbProc;
      bool fDummy;  //! module creates all TDCs but do not perform any transformation
      int  fAutoCalibr;  //! amount of statistic for the auto calibration in channels
      int  fProgress;    //! used in dummy

      bool retransmit();

      virtual int ExecuteCommand(dabc::Command cmd);

   public:
      TdcCalibrationModule(const std::string& name, dabc::Command cmd = 0);
      virtual ~TdcCalibrationModule();

      virtual bool ProcessBuffer(unsigned pool) { return retransmit(); }

      virtual bool ProcessRecv(unsigned port) { return retransmit(); }

      virtual bool ProcessSend(unsigned port) { return retransmit(); }

      virtual void BeforeModuleStart() { DOUT2("START CALIBR MODULE"); }

   };

}


#endif

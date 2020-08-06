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

#ifndef STREAM_TdcCalibrationModule
#define STREAM_TdcCalibrationModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef STREAM_DabcProcMgr
#include "stream/DabcProcMgr.h"
#endif

//#ifndef DABC_Profiler
//#include "dabc/Profiler.h"
//#endif


#include "hadaq/TrbProcessor.h"


namespace stream {


/** \brief Perform calibration of FPGA TDC data
 *
 * Module extracts TDC hits and replace fine counter with calibration values.
 */

   class TdcCalibrationModule : public dabc::ModuleAsync {

   protected:
      DabcProcMgr *fProcMgr{nullptr};         ///< stream process manager
      bool fOwnProcMgr;                       ///< if created in the module
      hadaq::TrbProcessor *fTrbProc{nullptr}; ///< TRB processor
      bool fDummy;                            ///< module creates all TDCs but do not perform any transformation
      int fDebug{0};                          ///< when specified, provides more debug output and special mode
      bool fReplace;                          ///< replace hit messages (true) or add calibration messages (false)
      int fAutoCalibr;                        ///< amount of statistic for the auto calibration in channels
      int fCountLinear;                       ///< number of count for liner calibrations when starting explicitly
      int fCountNormal;                       ///< number of counts for normal calibrations when starting explicitly
      std::string fCalibrFile;                ///< names to load/store calibrations
      unsigned fCalibrMask{0};                ///< mask to used for triggers
      bool fDoingTdcCalibr{false};            ///< indicates if module doing explicit TDC calibrations
      std::vector<int64_t> fDisabledCh;       ///< disabled for calibrations channels
      int fDummyCounter{0};                   ///< used in dummy
      dabc::TimeStamp fLastCalibr;            ///< use not to check for calibration very often
      int fAutoTdcMode{0};                    ///< automatic mode of TDC creation 0 - off, 1 - with linear calibr, 2 - with normal calibr
      int fAutoToTRange{0};                   ///< ToT range, 0 - normal, 1 - DiRICH
      int fLinearNumPoints{2};                ///< number of points used in linear calibration
      std::vector<uint64_t> fTDCs;            ///< remember TDCs in the beginning
      unsigned fTRB{0};                       ///< remember TRB id, used in module name
      int fProgress;                          ///< total calibration progress
      std::string fState;                     ///< current state
      double fQuality;                        ///< current calibration quality
      int fFineMin{0};                        ///< configure min value
      int fFineMax{0};                        ///< configure max value
      std::vector<uint64_t> fTdcMin;          ///< configured min TDC id
      std::vector<uint64_t> fTdcMax;          ///< configured max TDC id
      int fNumCh{33};                         ///< configured number of channel
      int fEdges{1};                          ///< configured edges
      bool fRecheckTdcs{false};               ///< check for missing tdcs, done once 5 sec

      // dabc::Profiler fProfiler;               ///< profiler of build event performance

      bool retransmit();

      int ExecuteCommand(dabc::Command cmd) override;

      void ConfigureNewTDC(hadaq::TdcProcessor *tdc);

      bool MatchTdcId(uint32_t tdcid);

   public:

      TdcCalibrationModule(const std::string &name, dabc::Command cmd = nullptr);
      virtual ~TdcCalibrationModule();

      void ProcessTimerEvent(unsigned) override;

      bool ProcessBuffer(unsigned) override { return retransmit(); }

      bool ProcessRecv(unsigned) override { return retransmit(); }

      bool ProcessSend(unsigned) override { return retransmit(); }

      void BeforeModuleStart() override;
      void AfterModuleStop() override;

      static void SetTRBStatus(dabc::Hierarchy &item, hadaq::TrbProcessor *trb, int *res_progress = nullptr, double *res_quality = nullptr, std::string *res_state = nullptr);

   };

}

#endif

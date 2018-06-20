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
      bool fDebug{false};                     ///< when specified, provides more debug output
      bool fReplace;                          ///< replace hit messages (true) or add calibration messages (false)
      int fAutoCalibr;                        ///< amount of statistic for the auto calibration in channels
      int fCountLinear;                       ///< number of count for liner calibrations when starting explicitly
      int fCountNormal;                       ///< number of counts for normal calibrations when starting explicitly
      std::string fCalibrFile;                ///< names to load calibrations
      unsigned fCalibrMask{0};                ///< mask to used for triggers
      bool fDoingTdcCalibr{false};            ///< indicates if module doing explicit TDC calibrations
      std::vector<int64_t> fDisabledCh;       ///< disabled for calibrations channels
      int fDummyCounter{0};                   ///< used in dummy
      dabc::TimeStamp fLastCalibr;            ///< use not to check for calibration very often
      int fAutoTdcMode{0};                    ///< automatic mode of TDC creation
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

      bool retransmit();

      virtual int ExecuteCommand(dabc::Command cmd);

   public:

      TdcCalibrationModule(const std::string &name, dabc::Command cmd = nullptr);
      virtual ~TdcCalibrationModule();

      virtual bool ProcessBuffer(unsigned) { return retransmit(); }

      virtual bool ProcessRecv(unsigned) { return retransmit(); }

      virtual bool ProcessSend(unsigned) { return retransmit(); }

      virtual void BeforeModuleStart() { DOUT2("START CALIBR MODULE"); }

      static void SetTRBStatus(dabc::Hierarchy& item, hadaq::TrbProcessor* trb, int *res_progress = 0, double *res_quality = 0, std::string *res_state = 0);

   };

}

#endif

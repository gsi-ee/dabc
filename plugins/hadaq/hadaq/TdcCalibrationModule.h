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

#ifndef DABC_timing
#include "dabc/timing.h"
#endif


namespace dabc {
   class ProcMgr;
}

namespace hadaq {

   class TrbProcessor;
   class TerminalModule;

/** \brief Perform calibration of FPGA TDC data
 *
 * Module extracts TDC hits and replace fine counter with calibration values.
 */

   class TdcCalibrationModule : public dabc::ModuleAsync {

   friend class TerminalModule;

   protected:

      dabc::ProcMgr* fProcMgr;
      hadaq::TrbProcessor* fTrbProc;
      bool fDummy;  //! module creates all TDCs but do not perform any transformation
      int  fAutoCalibr;  //! amount of statistic for the auto calibration in channels
      int  fDummyCounter;  //! used in dummy
      dabc::TimeStamp fLastCalibr; //! use not to check for calibration very often
      std::vector<uint64_t> fTDCs; //! remember TDCs in the beginning
      unsigned fTRB;               //! remember TRB id
      int fProgress;               //! total calibration progress
      std::string fState;          //! current state

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

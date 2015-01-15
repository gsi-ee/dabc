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

namespace hadaq {

/** \brief Perform calibration of FPGA TDC data
 *
 * Module extracts TDC hits and replace fine counter with calibration values.
 */

   class TdcCalibrationModule : public dabc::ModuleAsync {

   protected:

      bool retransmit();

      virtual int ExecuteCommand(dabc::Command cmd);

   public:
      TdcCalibrationModule(const std::string& name, dabc::Command cmd = 0);

      virtual bool ProcessBuffer(unsigned pool) { return retransmit(); }

      virtual bool ProcessRecv(unsigned port) { return retransmit(); }

      virtual bool ProcessSend(unsigned port) { return retransmit(); }

      virtual void BeforeModuleStart() { DOUT2("START CALIBR MODULE"); }

   };

}


#endif

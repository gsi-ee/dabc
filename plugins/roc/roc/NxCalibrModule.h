/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#ifndef ROC_NXCALIBRMODULE_H
#define ROC_NXCALIBRMODULE_H

#include "dabc/ModuleAsync.h"

#include "dabc/Device.h"

namespace roc {

   class Board;

   class NxCalibrModule : public dabc::ModuleAsync {

      public:

         NxCalibrModule(const std::string& name, dabc::Command cmd = 0, roc::Board* brd = 0);
         virtual ~NxCalibrModule();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

         int switchCalibration(bool on);

      protected:

         //dabc::Device*  fDev;

         roc::Board*    fBrd;
         double         fWorkPeriod;   // time for normal working
         double         fCalibrPeriod; // time for calibration
         int            fLoopCounts;   // number of calibration loops (-1 - infinite)
         int            fState; // 0 - off, 1 - normal, 2 - calibr
   };
}

#endif

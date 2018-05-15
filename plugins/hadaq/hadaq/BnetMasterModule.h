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

#ifndef HADAQ_BnetMasterModule
#define HADAQ_BnetMasterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif


namespace hadaq {

   /** \brief Master monitor for BNet components
    *
    * Provides statistic for clients
    */

   class BnetMasterModule : public dabc::ModuleAsync {
      protected:

         bool          fControl;  ///< when true, master actively controls BNET nodes and switches to new RUNs
         unsigned      fMaxRunSize; ///< maximal run size in MB
         dabc::Command fCurrentFileCmd; ///< currently running cmd to switch files
         int           fCmdCnt;     ///< just counter to avoid mismatch
         int           fCtrlId;     ///< counter for control requests
         dabc::TimeStamp fCtrlTm;   ///< time when last control count was send
         int           fCtrlCnt;    ///< how many control replies are awaited
         int           fCtrlState;  ///< 0-ok (green), 1-partially (yellow), 2-red
         std::string   fCtrlStateName; ///< current name
         bool          fCtrlSzLimit; ///< when true, size limit was exceed
         double        fCtrlData;   ///< accumulated data rate
         double        fCtrlEvents;   ///< accumulated events rate

         virtual bool ReplyCommand(dabc::Command cmd);

      public:

         BnetMasterModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual void BeforeModuleStart();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

   };
}


#endif

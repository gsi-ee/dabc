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
         uint64_t      fMaxRunSize; ///< maximal run size
         dabc::Command fCurrentCmd; ///< currently running cmd
         int           fCmdCnt;    ///< just counter to avoid mismatch

         virtual bool ReplyCommand(dabc::Command cmd);

      public:

         BnetMasterModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual void BeforeModuleStart();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

   };
}


#endif

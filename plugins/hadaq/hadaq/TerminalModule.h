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

#ifndef HADAQ_TerminalModule
#define HADAQ_TerminalModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include <vector>

namespace hadaq {

   /** \brief Terminal for HADAQ event builder
    *
    * Provides text display of different counters.
    */

   class TerminalModule : public dabc::ModuleAsync {
      protected:

         uint64_t        fTotalBuildEvents;
         uint64_t        fTotalRecvBytes;
         uint64_t        fTotalDiscEvents;
         uint64_t        fTotalDroppedData;

         bool            fDoClear;
         dabc::TimeStamp fLastTm;

         std::string rate_to_str(double r);

      public:

         TerminalModule(const std::string& name, dabc::Command cmd = 0);

         virtual void BeforeModuleStart();

         virtual void ProcessTimerEvent(unsigned timer);
   };
}


#endif

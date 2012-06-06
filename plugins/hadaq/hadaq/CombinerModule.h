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

#ifndef HADAQ_CombinerModule
#define HADAQ_CombinerModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

/*
 * This module will combine the hadtu data streams into full hadaq events
 * Use functionality as in daq_evtbuild here.
 */

namespace hadaq {




   class CombinerModule : public dabc::ModuleAsync {

      public:
         CombinerModule(const char* name, dabc::Command cmd = 0);

         virtual void ProcessInputEvent(dabc::Port* port);
   };


}


#endif

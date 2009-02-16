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
#ifndef ABB_ReadoutModule
#define ABB_ReadoutModule

#include "dabc/ModuleAsync.h"

#include "dabc/statistic.h"

namespace abb {

   class ReadoutModule : public dabc::ModuleAsync {

      public:

         ReadoutModule(const char* name, dabc::Command* cmd);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();

         virtual void ProcessUserEvent(dabc::ModuleItem* , uint16_t );


      protected:

         /** standalonetest flag switches to simple readout test
           * without forwarding buffers to bnet */

         dabc::PoolHandle*    fPool;
         dabc::Ratemeter      fRecvRate;
         bool                 fStandalone;

   };
}

#endif

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
#ifndef ABB_WriterModule
#define ABB_WriterModule

#include "dabc/ModuleSync.h"

#include "dabc/statistic.h"


namespace abb {

   class WriterModule : public dabc::ModuleSync {

      public:

         /** standalonetest flag switches to simple readout test
           * without forwarding buffers to bnet */
         WriterModule(const char* name, dabc::Command* cmd);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();

         void MainLoop();


      protected:
         dabc::PoolHandle*    fPool;
         dabc::Ratemeter      fWriteRate;
         bool                 fStandalone;
         int     		         fBufferSize;

   };
}

#endif

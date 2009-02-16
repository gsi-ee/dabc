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
#ifndef BNET_CombinerModule
#define BNET_CombinerModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class CombinerModule : public dabc::ModuleSync {
      protected:
         int                    fNumReadouts;
         int                    fModus;
         dabc::PoolHandle*      fInpPool;
         dabc::PoolHandle*      fOutPool;
         dabc::Port*            fOutPort;

         CombinerModule(const char* name, dabc::Command* cmd = 0);

      public:

         int NumReadouts() const { return fNumReadouts; }
   };
}

#endif

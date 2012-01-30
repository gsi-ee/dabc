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
#ifndef BNET_FilterModule
#define BNET_FilterModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class FilterModule : public dabc::ModuleSync {
      protected:
         dabc::PoolHandle*  fPool;

         virtual bool TestBuffer(dabc::Buffer*) { return true; }

         // constructor is hidden while filter module is incomplete
         FilterModule(const char* name, dabc::Command cmd = 0);

      public:

         virtual void MainLoop();
   };
}

#endif

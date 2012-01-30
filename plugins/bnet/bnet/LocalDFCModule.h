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
#ifndef BNET_LocalDFCModule
#define BNET_LocalDFCModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class LocalDFCModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*   fPool;

      public:
         LocalDFCModule(const char* name, dabc::Command cmd = 0);
   };
}

#endif

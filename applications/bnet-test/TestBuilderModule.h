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
#ifndef BNET_TestBuilderModule
#define BNET_TestBuilderModule

#include "dabc/ModuleAsync.h"

#include <vector>

namespace bnet {

   class TestBuilderModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*  fInpPool;
         dabc::PoolHandle*  fOutPool;
         int                fNumSenders;
         int                fOutBufferSize;

         std::vector<dabc::Buffer*> fBuffers;

      public:
         TestBuilderModule(const char* name, dabc::Command cmd = 0);
         virtual ~TestBuilderModule();

         virtual void ProcessItemEvent(dabc::ModuleItem* item, uint16_t evid);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
   };
}

#endif

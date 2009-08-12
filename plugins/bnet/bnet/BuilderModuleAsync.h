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
#ifndef BNET_BuilderModuleAsync
#define BNET_BuilderModuleAsync

#include "dabc/ModuleAsync.h"

#include "dabc/Buffer.h"

#include <vector>

namespace bnet {

   class BuilderModuleAsync : public dabc::ModuleAsync {

      protected:
         dabc::PoolHandle*    fInpPool;
         dabc::PoolHandle*    fOutPool;
         unsigned             fNumSenders;
         dabc::BufferSize_t   fOutBufferSize;

         std::vector<dabc::Buffer*> fBuffers;

         BuilderModuleAsync(const char* name, dabc::Command* cmd = 0);

      public:
         virtual ~BuilderModuleAsync();

         virtual void BeforeModuleStart();

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

         virtual void DoBuildEvent(std::vector<dabc::Buffer*>& bufs) {}
   };
}

#endif

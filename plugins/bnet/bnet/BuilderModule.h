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
#ifndef BNET_BuilderModule
#define BNET_BuilderModule

#include "dabc/ModuleSync.h"

#include "dabc/Buffer.h"

#include <vector>

namespace bnet {

   class BuilderModule : public dabc::ModuleSync {

      protected:
         dabc::PoolHandle*  fInpPool;
         dabc::PoolHandle*  fOutPool;
         int                  fNumSenders;
         dabc::BufferSize_t   fOutBufferSize;

         std::vector<dabc::Buffer*> fBuffers;

         BuilderModule(const char* name, dabc::Command* cmd = 0);

      public:
         virtual ~BuilderModule();

         virtual void MainLoop();

         virtual void DoBuildEvent(std::vector<dabc::Buffer*>& bufs) {}
   };
}

#endif

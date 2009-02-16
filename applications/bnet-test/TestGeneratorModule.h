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
#ifndef BNET_TestGeneratorModule
#define BNET_TestGeneratorModule

#include "dabc/ModuleAsync.h"

#include "dabc/logging.h"

#include "bnet/common.h"

namespace bnet {

   class TestGeneratorModule : public dabc::ModuleAsync {
      protected:
         uint64_t              fEventCnt;
         int                   fBufferSize;
         uint64_t              fUniquieId;

      public:
         TestGeneratorModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessOutputEvent(dabc::Port* port);
   };
}

#endif

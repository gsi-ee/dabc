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
#ifndef BNET_TestCombinerModule
#define BNET_TestCombinerModule

#include "dabc/ModuleAsync.h"

#include "bnet/common.h"

#include <vector>

namespace bnet {

   class TestCombinerModule : public dabc::ModuleAsync {
      protected:
         int                      fNumReadout;
         int                      fModus;
         dabc::PoolHandle*        fInpPool;
         dabc::PoolHandle*        fOutPool;
         uint64_t                 fOutBufferSize;
         dabc::Port*              fOutPort;
         std::vector<dabc::Buffer*> fBuffers;
         int                      fLastInput; // indicate id of the port where next packet must be read
         dabc::Buffer*            fOutBuffer;
         uint64_t                 fLastEvent;


         dabc::Buffer* MakeSegmenetedBuf(uint64_t& evid);
         dabc::Buffer* MakeMemCopyBuf(uint64_t& evid);

      public:
         TestCombinerModule(const char* name, dabc::Command* cmd = 0);

         virtual ~TestCombinerModule();

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);
   };
}

#endif

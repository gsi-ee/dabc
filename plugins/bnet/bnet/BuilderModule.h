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

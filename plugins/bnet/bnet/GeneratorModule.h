#ifndef BNET_GeneratorModule
#define BNET_GeneratorModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class GeneratorModule : public dabc::ModuleSync {
      protected:
         dabc::PoolHandle*       fPool;
         uint64_t                  fEventCnt;
         int                       fBufferSize;
         uint64_t                  fUniqueId;

         GeneratorModule(const char* name, dabc::Command* cmd = 0);

      public:

         virtual void MainLoop();

         uint64_t UniqueId() const { return fUniqueId; }

      protected:

         virtual void GeneratePacket(dabc::Buffer* buf) = 0;
   };
}

#endif

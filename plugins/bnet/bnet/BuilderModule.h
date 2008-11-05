#ifndef BNET_BuilderModule
#define BNET_BuilderModule

#include "dabc/ModuleSync.h"

#include "dabc/Buffer.h"

#include <vector>

namespace bnet {

   class WorkerApplication;

   class BuilderModule : public dabc::ModuleSync {

      protected:
         dabc::PoolHandle*  fInpPool;
         dabc::PoolHandle*  fOutPool;
         int                  fNumSenders;
         dabc::BufferSize_t   fOutBufferSize;

         std::vector<dabc::Buffer*> fBuffers;

      public:
         BuilderModule(const char* name, WorkerApplication* factory);
         virtual ~BuilderModule();

         virtual void MainLoop();

         virtual void DoBuildEvent(std::vector<dabc::Buffer*>& bufs) {}
   };
}

#endif

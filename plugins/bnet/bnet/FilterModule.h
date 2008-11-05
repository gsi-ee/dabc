#ifndef BNET_FilterModule
#define BNET_FilterModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class WorkerApplication;

   class FilterModule : public dabc::ModuleSync {
      protected:
         dabc::PoolHandle*  fPool;

         virtual bool TestBuffer(dabc::Buffer*) { return true; }

      public:
         FilterModule(const char* name, WorkerApplication* factory);

         virtual void MainLoop();
   };
}

#endif

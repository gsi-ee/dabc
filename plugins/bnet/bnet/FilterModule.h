#ifndef BNET_FilterModule
#define BNET_FilterModule

#include "dabc/ModuleSync.h"

namespace bnet {
   
   class WorkerPlugin;
   
   class FilterModule : public dabc::ModuleSync {
      protected:
         dabc::PoolHandle*  fPool;
         
         virtual bool TestBuffer(dabc::Buffer*) { return true; }
       
      public:
         FilterModule(dabc::Manager* m, const char* name, WorkerPlugin* factory);
         
         virtual void MainLoop();
   };   
}

#endif

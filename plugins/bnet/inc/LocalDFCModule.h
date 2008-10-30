#ifndef BNET_LocalDFCModule
#define BNET_LocalDFCModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class WorkerPlugin;

   class LocalDFCModule : public dabc::ModuleAsync {
      protected:  
         dabc::PoolHandle*   fPool;

      public:
         LocalDFCModule(dabc::Manager* m, const char* name, WorkerPlugin* factory);
   };
}

#endif

#ifndef BNET_LocalDFCModule
#define BNET_LocalDFCModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class LocalDFCModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*   fPool;

      public:
         LocalDFCModule(const char* name, dabc::Command* cmd = 0);
   };
}

#endif

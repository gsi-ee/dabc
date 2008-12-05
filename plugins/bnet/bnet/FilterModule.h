#ifndef BNET_FilterModule
#define BNET_FilterModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class FilterModule : public dabc::ModuleSync {
      protected:
         dabc::PoolHandle*  fPool;

         virtual bool TestBuffer(dabc::Buffer*) { return true; }

         // constructor is hidden while filter module is incomplete
         FilterModule(const char* name, dabc::Command* cmd);

      public:

         virtual void MainLoop();
   };
}

#endif

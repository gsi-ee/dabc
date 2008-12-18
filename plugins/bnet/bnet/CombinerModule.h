#ifndef BNET_CombinerModule
#define BNET_CombinerModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class CombinerModule : public dabc::ModuleSync {
      protected:
         int                    fNumReadouts;
         int                    fModus;
         dabc::PoolHandle*      fInpPool;
         dabc::PoolHandle*      fOutPool;
         dabc::Port*            fOutPort;

         CombinerModule(const char* name, dabc::Command* cmd = 0);

      public:

         int NumReadouts() const { return fNumReadouts; }
   };
}

#endif

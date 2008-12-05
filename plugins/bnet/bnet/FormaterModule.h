#ifndef BNET_FormaterModule
#define BNET_FormaterModule

#include "dabc/ModuleSync.h"

namespace bnet {

   class FormaterModule : public dabc::ModuleSync {
      protected:
         int                    fNumReadouts;
         int                    fModus;
         dabc::PoolHandle*      fInpPool;
         dabc::PoolHandle*      fOutPool;
         dabc::Port*            fOutPort;

         FormaterModule(const char* name, dabc::Command* cmd = 0);

      public:

         int NumReadouts() const { return fNumReadouts; }
   };
}

#endif

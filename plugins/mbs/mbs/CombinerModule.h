#ifndef MBS_CombinerModule
#define MBS_CombinerModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace mbs {

   class CombinerModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle* fPool;
         unsigned          fBufferSize;

      public:
         CombinerModule(const char* name, dabc::Command* cmd);
   };

}


#endif

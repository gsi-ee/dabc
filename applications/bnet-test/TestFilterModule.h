#ifndef BNET_TestFilterModule
#define BNET_TestFilterModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class TestFilterModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*  fPool;

      public:
         TestFilterModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);
   };
}

#endif

#ifndef BNET_TestFilterModule
#define BNET_TestFilterModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class WorkerApplication;

   class TestFilterModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*  fPool;

      public:
         TestFilterModule(const char* name, WorkerApplication* factory);

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);
   };
}

#endif

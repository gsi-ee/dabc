#ifndef BNET_TestFilterModule
#define BNET_TestFilterModule

#include "dabc/ModuleAsync.h"

namespace bnet {
   
   class WorkerPlugin;
   
   class TestFilterModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*  fPool;
       
      public:
         TestFilterModule(dabc::Manager* m, const char* name, WorkerPlugin* factory);
         
         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);
   };   
}

#endif

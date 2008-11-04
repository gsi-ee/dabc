#ifndef BNET_TestGeneratorModule
#define BNET_TestGeneratorModule

#include "dabc/ModuleAsync.h"

#include "dabc/logging.h"

#include "bnet/common.h"

namespace bnet {
    
   class WorkerApplication;
   
   class TestGeneratorModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*       fPool; 
         uint64_t                  fEventCnt;
         int                       fBufferSize;
         uint64_t                  fUniquieId;
       
      public:
         TestGeneratorModule(dabc::Manager* mgr, 
                             const char* name, 
                             WorkerApplication* factory);
         
         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
         
         void ProcessOutputEvent(dabc::Port* port);
         void ProcessInputEvent(dabc::Port* port);
         void ProcessPoolEvent(dabc::PoolHandle* pool);

      protected:  
      
         bool GeneratePacket();
   };   
}

#endif

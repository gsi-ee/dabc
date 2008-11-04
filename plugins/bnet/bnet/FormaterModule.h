#ifndef BNET_FormaterModule
#define BNET_FormaterModule

#include "dabc/ModuleSync.h"

namespace bnet {
    
   class WorkerApplication;
    
   class FormaterModule : public dabc::ModuleSync {
      protected:
         int                      fNumReadout; 
         int                      fModus;
         dabc::PoolHandle*      fInpPool;
         dabc::PoolHandle*      fOutPool;
         dabc::Port*              fOutPort; 
         
      public:
         FormaterModule(dabc::Manager* mgr, const char* name, 
                        WorkerApplication* factory);
                        
         virtual ~FormaterModule();

         int NumReadouts() const { return fNumReadout; }                     
   };   
}

#endif

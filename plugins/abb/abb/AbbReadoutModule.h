#ifndef DABC_AbbReadoutModule
#define DABC_AbbReadoutModule

#include "dabc/ModuleAsync.h"
#include <string>

namespace dabc {
    
   class AbbReadoutModule : public dabc::ModuleAsync {
     
      public:

         /** standalonetest flag switches to simple readout test
           * without forwarding buffers to bnet */   
         AbbReadoutModule(dabc::Manager* mgr, 
                             const char* name, 
                             dabc::Command* cmd);
         
         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
         
         void ProcessUserEvent(ModuleItem* , uint16_t );


      protected:
         dabc::PoolHandle*    fPool; 
         dabc::Ratemeter      fRecvRate;
         bool                 fStandalone;
         
   };   
}

#endif

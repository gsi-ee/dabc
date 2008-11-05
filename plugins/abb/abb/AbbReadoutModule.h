#ifndef DABC_AbbReadoutModule
#define DABC_AbbReadoutModule

#include "dabc/ModuleAsync.h"
#include <string>

namespace dabc {

   class AbbReadoutModule : public dabc::ModuleAsync {

      public:

         AbbReadoutModule(const char* name, dabc::Command* cmd);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();

         void ProcessUserEvent(ModuleItem* , uint16_t );


      protected:

         /** standalonetest flag switches to simple readout test
           * without forwarding buffers to bnet */

         dabc::PoolHandle*    fPool;
         dabc::Ratemeter      fRecvRate;
         bool                 fStandalone;

   };
}

#endif

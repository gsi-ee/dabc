#ifndef ABB_ReadoutModule
#define ABB_ReadoutModule

#include "dabc/ModuleAsync.h"

#include "dabc/statistic.h"

namespace abb {

   class ReadoutModule : public dabc::ModuleAsync {

      public:

         ReadoutModule(const char* name, dabc::Command* cmd);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();

         virtual void ProcessUserEvent(dabc::ModuleItem* , uint16_t );


      protected:

         /** standalonetest flag switches to simple readout test
           * without forwarding buffers to bnet */

         dabc::PoolHandle*    fPool;
         dabc::Ratemeter      fRecvRate;
         bool                 fStandalone;

   };
}

#endif

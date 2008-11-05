#ifndef DABC_AbbWriterModule
#define DABC_AbbWriterModule

#include "dabc/ModuleSync.h"
#include <string>

namespace dabc {

   class AbbWriterModule : public dabc::ModuleSync {

      public:

         /** standalonetest flag switches to simple readout test
           * without forwarding buffers to bnet */
         AbbWriterModule(const char* name,
                         dabc::Command* cmd);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();

         void MainLoop();


      protected:
         dabc::PoolHandle*    fPool;
         dabc::Ratemeter      fWriteRate;
         bool                 fStandalone;
         int     		      fBufferSize;

   };
}

#endif

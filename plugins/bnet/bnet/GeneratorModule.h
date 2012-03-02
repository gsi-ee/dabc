#ifndef BNET_GeneratorModule
#define BNET_GeneratorModule

#include "dabc/ModuleAsync.h"

#include "bnet/defines.h"

namespace bnet {

   class GeneratorModule : public dabc::ModuleAsync {
      protected:
         EventId             fEventCnt;
         int                 fUniqueId;
         int                 fNumIds;

         EventHandlingRef    fEventHandling;

      public:
         GeneratorModule(const char* name, dabc::Command cmd = 0);

         virtual void BeforeModuleStart();

         virtual void ProcessOutputEvent(dabc::Port* port);
   };
}

#endif

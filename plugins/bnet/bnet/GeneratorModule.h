#ifndef BNET_GeneratorModule
#define BNET_GeneratorModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class GeneratorModule : public dabc::ModuleAsync {
      protected:
         uint64_t                fEventCnt;
         uint64_t                fUniqueId;

      public:
         GeneratorModule(const char* name, dabc::Command cmd = 0);

         virtual void BeforeModuleStart();

         virtual void ProcessOutputEvent(dabc::Port* port);

      protected:

         bool FillPacket(dabc::Buffer& buf);
   };
}

#endif

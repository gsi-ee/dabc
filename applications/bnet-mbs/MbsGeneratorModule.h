#ifndef BNET_MbsGeneratorModule
#define BNET_MbsGeneratorModule

#include "bnet/GeneratorModule.h"

namespace bnet {

   class MbsGeneratorModule : public GeneratorModule {
      public:
         MbsGeneratorModule(const char* name,
                            WorkerApplication* factory) :
           GeneratorModule(name, factory) {}

      protected:

         virtual void GeneratePacket(dabc::Buffer* buf);
   };

}

#endif

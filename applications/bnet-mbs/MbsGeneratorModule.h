#ifndef BNET_MbsGeneratorModule
#define BNET_MbsGeneratorModule

#include "bnet/GeneratorModule.h"

namespace bnet {

   class MbsGeneratorModule : public GeneratorModule {
      public:
         MbsGeneratorModule(const char* name, dabc::Command* cmd = 0) :
           GeneratorModule(name, cmd) {}

      protected:

         virtual void GeneratePacket(dabc::Buffer* buf);
   };


   extern bool GenerateMbsPacket(dabc::Buffer* buf,
                                 int procid,
                                 int &evid,
                                 int SingleSubEventDataSize = 120,
                                 int MaxNumSubEvents = 8,
                                 int startacqevent = -1,
                                 int stopacqevent = -1);

}

#endif

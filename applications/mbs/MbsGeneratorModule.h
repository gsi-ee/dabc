#ifndef BNET_MbsGeneratorModule
#define BNET_MbsGeneratorModule

#include "bnet/GeneratorModule.h"

namespace bnet {
    
   class MbsGeneratorModule : public GeneratorModule {
      public:
         MbsGeneratorModule(dabc::Manager* mgr, 
                            const char* name, 
                            WorkerPlugin* factory) :
           GeneratorModule(mgr, name, factory) {}
                            
      protected:  
      
         virtual void GeneratePacket(dabc::Buffer* buf);
   };   
   
}

#endif

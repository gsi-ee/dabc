#ifndef BNET_MbsFactory
#define BNET_MbsFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   extern const char* xmlMbsWorkerClass;

   class MbsFactory : public dabc::Factory {
   public:
      MbsFactory(const char* name) : dabc::Factory(name) { DfltAppClass(xmlMbsWorkerClass); }

      virtual dabc::Application* CreateApplication(const char* classname, dabc::Command* cmd);

      virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd);
   };
}



#endif

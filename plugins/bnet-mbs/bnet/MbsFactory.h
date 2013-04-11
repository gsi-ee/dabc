#ifndef BNET_MbsFactory
#define BNET_MbsFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   extern const char* xmlMbsWorkerClass;

   class MbsFactory : public dabc::Factory {
   public:
      MbsFactory(const std::string& name) : dabc::Factory(name) {}

      virtual dabc::Application* CreateApplication(const std::string& classname, dabc::Command cmd);

      virtual dabc::Module* CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd);
   };
}



#endif

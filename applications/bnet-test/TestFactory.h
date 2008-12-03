#ifndef BNET_TestFactory
#define BNET_TestFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   extern const char* xmlTestWrokerClass;

   class TestFactory : public dabc::Factory {
   public:
      TestFactory(const char* name) : dabc::Factory(name) { DfltAppClass(xmlTestWrokerClass); }

      virtual dabc::Application* CreateApplication(const char* classname, dabc::Command* cmd);

   };
}

#endif

#ifndef BNET_TestFactory
#define BNET_TestFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   class TestFactory : public dabc::Factory {
   public:
      TestFactory(const char* name) : dabc::Factory(name) { DfltAppClass("TestBnetWorker"); }

      virtual dabc::Application* CreateApplication(dabc::Basic* parent, const char* classname, const char* appname, dabc::Command* cmd);

   };
}

#endif

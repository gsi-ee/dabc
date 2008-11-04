#ifndef BNET_MbsFactory
#define BNET_MbsFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   class MbsFactory : public dabc::Factory {
   public:
      MbsFactory(const char* name) : dabc::Factory(name) { DfltAppClass("MbsBnetWorker"); }

      virtual dabc::Application* CreateApplication(dabc::Basic* parent, const char* classname, const char* appname, dabc::Command* cmd);

   };
}



#endif

#ifndef BNET_Factory
#define BNET_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name) : dabc::Factory(name) { SetDfltAppClass("BnetCluster"); }

         virtual dabc::Application* CreateApplication(dabc::Basic* parent, const char* classname, const char* appname, dabc::Command* cmd);
   };

}




#endif

#ifndef BNET_Factory
#define BNET_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name);

         virtual dabc::Reference CreateObject(const char* classname, const char* objname, dabc::Command cmd);

         virtual dabc::Application* CreateApplication(const char* classname, dabc::Command cmd);

         virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command cmd);
   };

}


#endif

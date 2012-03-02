#ifndef VERBS_Factory
#define VERBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace verbs {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name) : dabc::Factory(name) {}

         virtual dabc::Reference CreateObject(const char* classname, const char* objname, dabc::Command cmd);

         virtual dabc::Device* CreateDevice(const char* classname,
                                            const char* devname,
                                            dabc::Command cmd);

         virtual dabc::Reference CreateThread(dabc::Reference parent, const char* classname, const char* thrdname, const char* thrddev, dabc::Command cmd);
   };
}

#endif

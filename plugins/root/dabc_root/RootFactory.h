#ifndef DABC_RootFactory
#define DABC_RootFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace dabc_root {

   class RootFactory : public dabc::Factory {
      public:
         RootFactory(const char* name) : dabc::Factory(name) {}

         virtual dabc::DataInput* CreateDataInput(const char* typ, const char* name, dabc::Command* cmd = 0);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd = 0);
   };

}

#endif

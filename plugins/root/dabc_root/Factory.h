#ifndef DABC_Factory
#define DABC_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace dabc_root {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name) : dabc::Factory(name) {}

         virtual dabc::DataInput* CreateDataInput(const char* typ);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ);
   };

}

#endif

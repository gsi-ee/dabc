#ifndef MBS_Factory
#define MBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace mbs {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name) : dabc::Factory(name) {}

         static const char* LmdFileType() { return "LmdFile"; }

         virtual dabc::Device* CreateDevice(const char* classname, const char* devname, dabc::Command*);

         virtual dabc::DataInput* CreateDataInput(const char* typ, const char* name, dabc::Command* cmd = 0);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd = 0);

      protected:

   };

}

#endif

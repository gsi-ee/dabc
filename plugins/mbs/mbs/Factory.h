#ifndef MBS_Factory
#define MBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace mbs {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name) : dabc::Factory(name) {}

         virtual dabc::Transport* CreateTransport(dabc::Port* port, const char* typ, const char* thrdname, dabc::Command*);

         virtual dabc::DataInput* CreateDataInput(const char* typ);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ);

      protected:

   };

}

#endif

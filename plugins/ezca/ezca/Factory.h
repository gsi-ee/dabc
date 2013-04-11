#ifndef EZCA_FACTORY_H
#define EZCA_FACTORY_H

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace ezca {

   class Factory: public dabc::Factory {

      public:

         Factory(const std::string& name);

         virtual dabc::DataInput* CreateDataInput(const std::string& typ);
   };

}

#endif


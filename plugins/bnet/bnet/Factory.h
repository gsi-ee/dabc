#ifndef BNET_Factory
#define BNET_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name);

         dabc::Reference CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd) override;

         dabc::Module *CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd) override;
   };

}

#endif

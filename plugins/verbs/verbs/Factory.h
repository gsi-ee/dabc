#ifndef VERBS_Factory
#define VERBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support of InfiniBand %verbs */

namespace verbs {

   /** \brief %Factory for VERBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         virtual dabc::Reference CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd);

         virtual dabc::Device* CreateDevice(const std::string &classname,
                                            const std::string &devname,
                                            dabc::Command cmd);

         virtual dabc::Reference CreateThread(dabc::Reference parent, const std::string &classname, const std::string &thrdname, const std::string &thrddev, dabc::Command cmd);
   };
}

#endif

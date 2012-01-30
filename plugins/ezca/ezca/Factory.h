#ifndef EZCA_FACTORY_H
#define EZCA_FACTORY_H

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace ezca {

   class Factory: public dabc::Factory {

      public:

         Factory(const char* name);

         virtual dabc::Application* CreateApplication(const char* classname, dabc::Command cmd);

         virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command cmd);

         virtual dabc::DataInput* CreateDataInput(const char* typ);

         virtual dabc::Device* CreateDevice(const char* classname, const char* devname, dabc::Command cmd);
   };

}

#endif


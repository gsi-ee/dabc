#ifndef ROC_Factory
#define ROC_Factory

#include "dabc/Factory.h"

namespace roc {

   class Factory: public dabc::Factory  {
      public:

         Factory(const char* name) : dabc::Factory(name) { DfltAppClass("RocReadoutApp"); }

         virtual dabc::Application* CreateApplication(const char* classname, const char* appname, dabc::Command* cmd);

         virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd);

         virtual dabc::Device* CreateDevice(const char* classname, const char* devname, dabc::Command* com);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ);
   };

}

#endif


#ifndef BNET_Application
#define BNET_Application

#include "dabc/Application.h"

namespace bnet {

   class Application : public dabc::Application {
      public:

         Application();
         virtual ~Application();

         virtual bool CreateAppModules();

         virtual bool BeforeAppModulesStarted();

         virtual int SMCommandTimeout() const;
   };
}

#endif

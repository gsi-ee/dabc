#include "TestFactory.h"

#include "TestWorkerApplication.h"

bnet::TestFactory bnettestfactory("bnet-test");

dabc::Application* bnet::TestFactory::CreateApplication(dabc::Basic* parent, const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname, "TestBnetWorker")!=0)
      return new bnet::TestWorkerApplication(parent, appname);

   return dabc::Factory::CreateApplication(parent, classname, appname, cmd);
}


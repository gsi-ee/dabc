#include "TestFactory.h"

#include "TestWorkerApplication.h"

bnet::TestFactory bnettestfactory("bnet-test");

dabc::Application* bnet::TestFactory::CreateApplication(const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname, "TestBnetWorker")==0)
      return new bnet::TestWorkerApplication(appname);

   return dabc::Factory::CreateApplication(classname, appname, cmd);
}


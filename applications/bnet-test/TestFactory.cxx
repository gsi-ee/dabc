#include "TestFactory.h"

#include "TestWorkerApplication.h"

bnet::TestFactory bnettestfactory("bnet-test");

const char* bnet::xmlTestWrokerClass = "bnet::TestWorker";

dabc::Application* bnet::TestFactory::CreateApplication(const char* classname, dabc::Command* cmd)
{
   if (strcmp(classname, xmlTestWrokerClass)==0)
      return new bnet::TestWorkerApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}


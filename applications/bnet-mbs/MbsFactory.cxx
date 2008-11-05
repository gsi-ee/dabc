#include "MbsFactory.h"

#include "MbsWorkerApplication.h"

bnet::MbsFactory bnetmbsfactory("bnet-mbs");

dabc::Application* bnet::MbsFactory::CreateApplication(const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname, "MbsBnetWorker")==0)
      return new bnet::MbsWorkerApplication(appname);

   return dabc::Factory::CreateApplication(classname, appname, cmd);
}

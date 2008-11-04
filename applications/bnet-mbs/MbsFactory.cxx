#include "MbsFactory.h"

#include "MbsWorkerApplication.h"

bnet::MbsFactory bnetmbsfactory("bnet-mbs");

dabc::Application* bnet::MbsFactory::CreateApplication(dabc::Basic* parent, const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname, "MbsBnetWorker")!=0)
      return new bnet::MbsWorkerApplication(parent, appname);

   return dabc::Factory::CreateApplication(parent, classname, appname, cmd);
}

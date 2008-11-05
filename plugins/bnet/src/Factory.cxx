#include "bnet/Factory.h"

#include "bnet/ClusterApplication.h"

bnet::Factory bnetfactory("bnet");

dabc::Application* bnet::Factory::CreateApplication(const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname, "BnetCluster")==0)
      return new bnet::ClusterApplication(appname);

   return dabc::Factory::CreateApplication(classname, appname, cmd);
}


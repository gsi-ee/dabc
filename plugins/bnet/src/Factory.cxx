#include "bnet/Factory.h"

#include "bnet/ClusterApplication.h"

bnet::Factory bnetfactory("bnet");

dabc::Application* bnet::Factory::CreateApplication(dabc::Basic* parent, const char* classname, const char* appname, dabc::Command* cmd)
{
   if (strcmp(classname, "BnetCluster")==0)
      return new bnet::ClusterApplication(parent, appname);

   return dabc::Factory::CreateApplication(parent, classname, appname, cmd);
}


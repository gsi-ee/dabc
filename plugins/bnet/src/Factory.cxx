#include "bnet/Factory.h"

#include "bnet/common.h"
#include "bnet/ClusterApplication.h"

bnet::Factory bnetfactory("bnet");

bnet::Factory::Factory(const char* name) :
   dabc::Factory(name)
{
   DfltAppClass(xmlClusterClass);
}


dabc::Application* bnet::Factory::CreateApplication(const char* classname, dabc::Command* cmd)
{
   if (strcmp(classname, xmlClusterClass)==0)
      return new bnet::ClusterApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}


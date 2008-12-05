#include "bnet/Factory.h"

#include "bnet/common.h"
#include "bnet/ClusterApplication.h"
#include "bnet/GlobalDFCModule.h"
#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

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

dabc::Module* bnet::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{

   if (strcmp(classname, "bnet::GlobalDFCModule")==0)
      return new bnet::GlobalDFCModule(modulename, cmd);
   else
   if (strcmp(classname, "bnet::SenderModule")==0)
      return new bnet::SenderModule(modulename, cmd);
   else
   if (strcmp(classname, "bnet::ReceiverModule")==0)
      return new bnet::ReceiverModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

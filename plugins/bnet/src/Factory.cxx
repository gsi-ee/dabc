#include "bnet/Factory.h"

#include "bnet/Application.h"
#include "bnet/GeneratorModule.h"
#include "bnet/TransportModule.h"

bnet::Factory bnetfactory("bnet");

bnet::Factory::Factory(const char* name) :
   dabc::Factory(name)
{
}

dabc::Application* bnet::Factory::CreateApplication(const char* classname, dabc::Command cmd)
{
   if (strcmp(classname, "bnet::Application")==0)
      return new bnet::Application();
   return dabc::Factory::CreateApplication(classname, cmd);
}


dabc::Module* bnet::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
{
   if (strcmp(classname,"bnet::TransportModule")==0)
      return new bnet::TransportModule(modulename, cmd);

   if (strcmp(classname,"bnet::GeneratorModule")==0)
      return new bnet::GeneratorModule(modulename, cmd);

   return 0;
}

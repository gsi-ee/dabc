#include "bnet/Factory.h"

#include "bnet/defines.h"
#include "bnet/Application.h"
#include "bnet/GeneratorModule.h"
#include "bnet/TransportModule.h"

dabc::FactoryPlugin bnetfactory(new bnet::Factory("bnet"));

bnet::Factory::Factory(const std::string& name) :
   dabc::Factory(name)
{
}
dabc::Reference bnet::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname == "TestEventHandling")
      return new bnet::TestEventHandling(objname);

   return 0;
}


dabc::Application* bnet::Factory::CreateApplication(const std::string& classname, dabc::Command cmd)
{
   if (classname == "bnet::Application")
      return new bnet::Application();
   return dabc::Factory::CreateApplication(classname, cmd);
}


dabc::Module* bnet::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "bnet::TransportModule")
      return new bnet::TransportModule(modulename, cmd);

   if (classname == "bnet::GeneratorModule")
      return new bnet::GeneratorModule(modulename, cmd);

   return 0;
}

#include "bnet/Factory.h"

#include "bnet/testing.h"
#include "bnet/GeneratorModule.h"
#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

dabc::FactoryPlugin bnetfactory(new bnet::Factory("bnet"));

bnet::Factory::Factory(const std::string &name) :
   dabc::Factory(name)
{
}

dabc::Reference bnet::Factory::CreateObject(const std::string &classname, const std::string &objname, dabc::Command)
{
   if (classname == "TestEventsProducer")
      return new bnet::TestEventsProducer(objname);

   if (classname == "TestEventsIterator")
      return new bnet::TestEventsIterator(objname);

   return nullptr;
}

dabc::Module* bnet::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "bnet::GeneratorModule")
      return new bnet::GeneratorModule(modulename, cmd);

   if (classname == "bnet::SenderModule")
      return new bnet::SenderModule(modulename, cmd);

   if (classname == "bnet::ReceiverModule")
      return new bnet::ReceiverModule(modulename, cmd);

   return nullptr;
}

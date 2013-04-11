#include "bnet/MbsFactory.h"

#include "bnet/MbsWorkerApplication.h"
#include "bnet/MbsCombinerModule.h"
#include "bnet/MbsFilterModule.h"
#include "bnet/MbsBuilderModule.h"
#include "bnet/SplitterModule.h"

dabc::FactoryPlugin bnetmbsfactory(new bnet::MbsFactory("bnet-mbs"));

const char* bnet::xmlMbsWorkerClass = "bnet::MbsWorker";


dabc::Application* bnet::MbsFactory::CreateApplication(const std::string& classname, dabc::Command cmd)
{
   if (classname == xmlMbsWorkerClass)
      return new bnet::MbsWorkerApplication;

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* bnet::MbsFactory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "bnet::MbsCombinerModule")
      return new bnet::MbsCombinerModule(modulename, cmd);

   if (classname == "bnet::MbsFilterModule")
      return new bnet::MbsFilterModule(modulename, cmd);

   if (classname == "bnet::MbsBuilderModule")
      return new bnet::MbsBuilderModule(modulename, cmd);

   if (classname == "dabc::SplitterModule")
      return new dabc::SplitterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

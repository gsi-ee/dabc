#include "bnet/MbsFactory.h"

#include "bnet/MbsWorkerApplication.h"
#include "bnet/MbsCombinerModule.h"
#include "bnet/MbsFilterModule.h"
#include "bnet/MbsBuilderModule.h"
#include "bnet/SplitterModule.h"

dabc::FactoryPlugin bnetmbsfactory(new bnet::MbsFactory("bnet-mbs"));

const char* bnet::xmlMbsWorkerClass = "bnet::MbsWorker";


dabc::Application* bnet::MbsFactory::CreateApplication(const char* classname, dabc::Command cmd)
{
   if (strcmp(classname, xmlMbsWorkerClass)==0)
      return new bnet::MbsWorkerApplication;

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* bnet::MbsFactory::CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
{
   if (strcmp(classname, "bnet::MbsCombinerModule")==0)
      return new bnet::MbsCombinerModule(modulename, cmd);
   else
   if (strcmp(classname, "bnet::MbsFilterModule")==0)
      return new bnet::MbsFilterModule(modulename, cmd);
   else
   if (strcmp(classname, "bnet::MbsBuilderModule")==0)
      return new bnet::MbsBuilderModule(modulename, cmd);
   else
   if (strcmp(classname, "dabc::SplitterModule")==0)
      return new dabc::SplitterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

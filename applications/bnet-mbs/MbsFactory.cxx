#include "MbsFactory.h"

#include "MbsWorkerApplication.h"

bnet::MbsFactory bnetmbsfactory("bnet-mbs");

const char* bnet::xmlMbsWorkerClass = "bnet::MbsWorker";


dabc::Application* bnet::MbsFactory::CreateApplication(const char* classname, dabc::Command* cmd)
{
   if (strcmp(classname, xmlMbsWorkerClass)==0)
      return new bnet::MbsWorkerApplication;

   return dabc::Factory::CreateApplication(classname, cmd);
}

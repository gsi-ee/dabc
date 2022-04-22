#include "bnet/SenderModule.h"

#include "dabc/Manager.h"


bnet::SenderModule::SenderModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name,cmd)
{
   DOUT0("%s num outputs %d", GetName(), NumOutputs());

   fIter = dabc::mgr.CreateObject("TestEventsIterator", "bnet-evnt-iter");

   fOut.resize(NumOutputs());

   for (unsigned n=0;n<NumOutputs();n++) {
      fOut[n].iter = dabc::mgr.CreateObject("TestEventsProducer", "bnet-evnt-gener");
   }

}

bnet::SenderModule::~SenderModule()
{
}

bool bnet::SenderModule::ProcessData()
{
   return false;
}

void bnet::SenderModule::ProcessTimerEvent(unsigned)
{
}


int bnet::SenderModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void bnet::SenderModule::BeforeModuleStart()
{
   DOUT0("%s BeforeModuleStart", GetName());
}

void bnet::SenderModule::AfterModuleStop()
{
}

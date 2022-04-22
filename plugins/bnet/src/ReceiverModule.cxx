#include "bnet/ReceiverModule.h"

bnet::ReceiverModule::ReceiverModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name,cmd)
{
   DOUT0("%s num inputs %d", GetName(), NumInputs());
}

bnet::ReceiverModule::~ReceiverModule()
{
}


bool bnet::ReceiverModule::ProcessRecv(unsigned)
{
   return false;
}

bool bnet::ReceiverModule::ProcessSend(unsigned)
{
   return false;
}

void bnet::ReceiverModule::ProcessTimerEvent(unsigned)
{
}


int bnet::ReceiverModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void bnet::ReceiverModule::BeforeModuleStart()
{
   DOUT0("%s BeforeModuleStart", GetName());
}

void bnet::ReceiverModule::AfterModuleStop()
{
}

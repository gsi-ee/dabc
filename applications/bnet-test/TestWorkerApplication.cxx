#include "TestWorkerApplication.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"

#include "bnet/GeneratorModule.h"

#include "TestGeneratorModule.h"
#include "TestCombinerModule.h"
#include "TestBuilderModule.h"
#include "TestFilterModule.h"

bnet::TestWorkerApplication::TestWorkerApplication(const char* name) :
   WorkerApplication(name) ,
   fABBActive(false)
{
// register application specific parameters here:

    DOUT0(("Set all pars to TestWorker"));

    SetPars(bnet::NetBidirectional, 4, 0);
}


dabc::Module* bnet::TestWorkerApplication::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)

{
   if (strcmp(classname,"TestGeneratorModule")==0)
      return new bnet::TestGeneratorModule(modulename, this);
   else
   if (strcmp(classname,"TestCombinerModule")==0)
      return new bnet::TestCombinerModule(modulename, this);
   else
   if (strcmp(classname,"TestBuilderModule")==0)
      return new bnet::TestBuilderModule(modulename, this);
   else
   if (strcmp(classname,"TestFilterModule")==0)
      return new bnet::TestFilterModule(modulename, this);
   else
      return bnet::WorkerApplication::CreateModule(classname, modulename, cmd);
}

bool bnet::TestWorkerApplication::CreateReadout(const char* portname, int portnumber)
{
   if (!IsGenerator()) return false;

   DOUT1(("CreateReadout buf = %d", ReadoutBufferSize()));

   bool res = false;
   std::string modulename;
   dabc::formats(modulename,"Readout%d", portnumber);
   std::string abbdevname;
   std::string modinputname = modulename+"/Ports/Input";
   // check parameter if we should use abb at this readout:
   if(ReadoutPar(portnumber) == "ABB") {
       abbdevname = "ABBDevice";
       fABBActive = dabc::mgr()->CreateDevice("abb::Device", abbdevname.c_str());
   } else {
      // create dummy event generator module:
      dabc::Module* m = new bnet::TestGeneratorModule(modulename.c_str(), this);
      dabc::mgr()->MakeThreadForModule(m, Thrd1Name().c_str());
      modulename += "/Ports/Output";
      dabc::mgr()->ConnectPorts(modulename.c_str(), portname);
      fABBActive = false;
   }

   dabc::mgr()->CreateMemoryPools(); // pools must exist before abb transport can connect to module
   if(fABBActive) {
      /// here creation of pci transport from abb device with connection to given input port:
       res = dabc::mgr()->CreateTransport(portname, abbdevname.c_str());
       if (!res) {
          EOUT(("Cannot create ABB transport"));
          return false;
       }
   }

   return true;
}

dabc::Module* bnet::TestWorkerApplication::CreateCombiner(const char* name)
{
   dabc::Module* m = new bnet::TestCombinerModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, Thrd1Name().c_str());
   return m;
}

dabc::Module* bnet::TestWorkerApplication::CreateBuilder(const char* name)
{
   dabc::Module* m = new bnet::TestBuilderModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, Thrd2Name().c_str());
   return m;
}

dabc::Module* bnet::TestWorkerApplication::CreateFilter(const char* name)
{
   dabc::Module* m = new bnet::TestFilterModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, Thrd2Name().c_str());
   return m;
}

bool bnet::TestWorkerApplication::CreateStorage(const char* portname)
{
   return bnet::WorkerApplication::CreateStorage(portname);
}


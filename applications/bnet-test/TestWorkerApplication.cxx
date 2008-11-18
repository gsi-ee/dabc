#include "TestWorkerApplication.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"

#include "bnet/GeneratorModule.h"

#include "TestGeneratorModule.h"
#include "TestCombinerModule.h"
#include "TestBuilderModule.h"
#include "TestFilterModule.h"

#ifdef __USE_ABB__
#include "abb/AbbFactory.h"
#endif

#include <iostream>

bnet::TestWorkerApplication::TestWorkerApplication(const char* name) :
   WorkerApplication(name) ,
   fABBActive(false)
{
// register application specific parameters here:
#ifdef __USE_ABB__
    CreateStrPar(ABB_PAR_DEVICE, "ABB_Device");
    CreateStrPar(ABB_PAR_MODULE, "ABB_Readout");
    CreateIntPar(ABB_PAR_BOARDNUM,0);
    CreateIntPar(ABB_PAR_BAR,1);
    CreateIntPar(ABB_PAR_ADDRESS,(0x8000 >> 2));
    CreateIntPar(ABB_PAR_LENGTH, 8*1024);
#endif

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
   dabc::String modulename;
   dabc::formats(modulename,"Readout%d", portnumber);
   dabc::String abbdevname;
   dabc::String modinputname = modulename+"/Ports/Input";
   // check parameter if we should use abb at this readout:
#ifdef __USE_ABB__
   if(strcmp(ReadoutPar(portnumber),"ABB")==0)
      {
         // create device by command:
         abbdevname=GetParCharStar(ABB_PAR_DEVICE, "ABBDevice");
         dabc::Command* dcom= new dabc::CmdCreateDevice("AbbDevice", abbdevname.c_str());
         // set additional parameters for abb device here:
         dcom->SetInt(ABB_PAR_BOARDNUM, GetParInt(ABB_PAR_BOARDNUM, 0));
         dcom->SetInt(ABB_PAR_BAR, GetParInt(ABB_PAR_BAR, 1));
         dcom->SetInt(ABB_PAR_ADDRESS, GetParInt(ABB_PAR_ADDRESS, (0x8000 >> 2)));
         dcom->SetInt(ABB_PAR_LENGTH, GetParInt(ABB_PAR_LENGTH, 8192));
         res=dabc::mgr()->CreateDevice("AbbDevice",abbdevname.c_str(),dcom);

         //dabc::mgr()->Execute(dcom);
         abbdevname=std::string("Devices/")+abbdevname;
         //fABBActive=true;
         fABBActive=res;
      }
   else
#endif
      {
         // create dummy event generator module:
         dabc::Module* m = new bnet::TestGeneratorModule(modulename.c_str(), this);
         dabc::mgr()->MakeThreadForModule(m, Thrd1Name());
         modulename += "/Ports/Output";
         dabc::mgr()->ConnectPorts(modulename.c_str(), portname);
         fABBActive=false;
      }

   dabc::mgr()->CreateMemoryPools(); // pools must exist before abb transport can connect to module
   if(fABBActive)
      {
      /// here creation of pci transport from abb device with connection to given input port:
        res=dabc::mgr()->CreateTransport(abbdevname.c_str(), portname);
//        dabc::Command* concom= new dabc::CmdCreateTransport(portname);
//        dabc::Device* abbdev=dabc::mgr()->FindDevice(abbdevname.c_str());
//        if(abbdev==0) return false;
//        bool res=abbdev->Execute(concom);
/////// following  to test failure:
//       DOUT1(("CreateReadout Sleeping 60s ..."));
//       ::sleep(60);
//       DOUT1(("done."));
      }

   return true;
}

dabc::Module* bnet::TestWorkerApplication::CreateCombiner(const char* name)
{
   dabc::Module* m = new bnet::TestCombinerModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, Thrd1Name());
   return m;
}

dabc::Module* bnet::TestWorkerApplication::CreateBuilder(const char* name)
{
   dabc::Module* m = new bnet::TestBuilderModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, Thrd2Name());
   return m;
}

dabc::Module* bnet::TestWorkerApplication::CreateFilter(const char* name)
{
   dabc::Module* m = new bnet::TestFilterModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, Thrd2Name());
   return m;
}

bool bnet::TestWorkerApplication::CreateStorage(const char* portname)
{
   return bnet::WorkerApplication::CreateStorage(portname);
}


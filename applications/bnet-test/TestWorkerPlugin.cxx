#include "TestWorkerPlugin.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"  

#include "bnet/GeneratorModule.h"

#include "TestGeneratorModule.h"
#include "TestCombinerModule.h"
#include "TestBuilderModule.h"
#include "TestFilterModule.h"

#ifdef __USE_ABB__
#include "dabc/AbbFactory.h"
#endif

#include <iostream>

bnet::TestWorkerPlugin::TestWorkerPlugin(dabc::Manager* m) 
: WorkerPlugin(m) , fABBActive(false)
{ 
// register application specific parameters here:   
#ifdef __USE_ABB__
    new dabc::StrParameter(this, ABB_PAR_DEVICE, "ABB_Device");
    new dabc::StrParameter(this, ABB_PAR_MODULE, "ABB_Readout");
    new dabc::IntParameter(this, ABB_PAR_BOARDNUM,0);
    new dabc::IntParameter(this, ABB_PAR_BAR,1);
    new dabc::IntParameter(this, ABB_PAR_ADDRESS,(0x8000 >> 2));
    new dabc::IntParameter(this, ABB_PAR_LENGTH, 8*1024);   
#endif   
}




dabc::Module* bnet::TestWorkerPlugin::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)

{
   if (strcmp(classname,"TestGeneratorModule")==0)
      return new bnet::TestGeneratorModule(GetManager(), modulename, this);
   else
   if (strcmp(classname,"TestCombinerModule")==0)
      return new bnet::TestCombinerModule(GetManager(), modulename, this);
   else
   if (strcmp(classname,"TestBuilderModule")==0)
      return new bnet::TestBuilderModule(GetManager(), modulename, this);
   else
   if (strcmp(classname,"TestFilterModule")==0)
      return new bnet::TestFilterModule(GetManager(), modulename, this);
   else
      return bnet::WorkerPlugin::CreateModule(classname, modulename, cmd);
}

bool bnet::TestWorkerPlugin::CreateReadout(const char* portname, int portnumber)
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
         res=GetManager()->CreateDevice("AbbDevice",abbdevname.c_str(),dcom);

         //GetManager()->Execute(dcom);
         abbdevname=std::string("Devices/")+abbdevname;
         //fABBActive=true;
         fABBActive=res;
      }
   else
#endif
      {   
         // create dummy event generator module:
         dabc::Module* m = new bnet::TestGeneratorModule(GetManager(), modulename.c_str(), this);
         GetManager()->MakeThreadForModule(m, Thrd1Name());
         modulename += "/Ports/Output";   
         GetManager()->ConnectPorts(modulename.c_str(), portname);      
         fABBActive=false;
      }   
      
   GetManager()->CreateMemoryPools(); // pools must exist before abb transport can connect to module   
   if(fABBActive)
      {
      /// here creation of pci transport from abb device with connection to given input port:
        res=GetManager()->CreateTransport(abbdevname.c_str(), portname);
//        dabc::Command* concom= new dabc::CmdCreateTransport(portname);
//        dabc::Device* abbdev=GetManager()->FindDevice(abbdevname.c_str());
//        if(abbdev==0) return false;
//        bool res=abbdev->Execute(concom);
/////// following  to test failure:        
//       DOUT1(("CreateReadout Sleeping 60s ..."));
//       ::sleep(60); 
//       DOUT1(("done.")); 
      }

   return true;
}

dabc::Module* bnet::TestWorkerPlugin::CreateCombiner(const char* name) 
{
   dabc::Module* m = new bnet::TestCombinerModule(GetManager(), name, this);
   GetManager()->MakeThreadForModule(m, Thrd1Name());
   return m;
}

dabc::Module* bnet::TestWorkerPlugin::CreateBuilder(const char* name)
{
   dabc::Module* m = new bnet::TestBuilderModule(GetManager(), name, this);
   GetManager()->MakeThreadForModule(m, Thrd2Name()); 
   return m;
}

dabc::Module* bnet::TestWorkerPlugin::CreateFilter(const char* name)
{
   dabc::Module* m = new bnet::TestFilterModule(GetManager(), name, this);
   GetManager()->MakeThreadForModule(m, Thrd2Name());
   return m;
}

bool bnet::TestWorkerPlugin::CreateStorage(const char* portname)
{
   return bnet::WorkerPlugin::CreateStorage(portname);
}





// _____________________________________________________________________

void InitUserPlugin(dabc::Manager* mgr)
{
   //std::cout<<"iiiiiiiii InitUserPlugin..."<<std::endl;
   
   bool is_all_to_all = bnet::NetBidirectional;
   int CombinerModus = 0;
   int NumReadouts = 4;

   bnet::TestWorkerPlugin* worker = new bnet::TestWorkerPlugin(mgr);

   worker->SetPars(is_all_to_all, NumReadouts, CombinerModus);


   
}

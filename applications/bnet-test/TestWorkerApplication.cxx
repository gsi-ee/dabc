#include "TestWorkerApplication.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"

#include "bnet/GeneratorModule.h"

#include "TestGeneratorModule.h"
#include "TestCombinerModule.h"
#include "TestBuilderModule.h"
#include "TestFilterModule.h"
#include "TestFactory.h"

bnet::TestWorkerApplication::TestWorkerApplication() :
   WorkerApplication(xmlTestWrokerClass) ,
   fABBActive(false)
{
// register application specific parameters here:

    DOUT0(("Set all pars to TestWorker"));

    int nodeid = dabc::mgr()->NodeId();
    int numnodes = dabc::mgr()->NumNodes();

    if ((nodeid<0) || (nodeid>=numnodes)) return;

    int numsrcnodes(1), numbldnodes(1);

    bool all_to_all = true;

    if (all_to_all) {
       numsrcnodes = numnodes - 1;
       numbldnodes = numsrcnodes;
    } else {
       numsrcnodes = (numnodes - 1) / 2;
       numbldnodes = numnodes - 1 - numsrcnodes;
    }

    // first, set common parameters
    bool issender(false), isreceiver(false);

    if (all_to_all) {
       issender = true;
       isreceiver = true;
    } else {
       issender = (nodeid <= numsrcnodes);
       isreceiver = !issender;
    }

    SetParBool(xmlIsGenerator, true);
    SetParBool(xmlIsSender, issender);
    SetParBool(xmlIsReceiever, isreceiver);
    SetParBool(xmlIsFilter, isreceiver);
    SetParInt(xmlCombinerModus, 0);
    SetParInt(xmlNumReadouts, 4);
}


bool bnet::TestWorkerApplication::CreateReadout(const char* portname, int portnumber)
{
   if (!IsGenerator()) return false;

   DOUT1(("CreateReadout buf = %d", GetParInt(xmlReadoutBuffer)));

   bool res = false;
   std::string modulename;
   dabc::formats(modulename,"Readout%d", portnumber);
   std::string abbdevname;
   std::string modinputname = modulename+"/Input";
   // check parameter if we should use abb at this readout:
   if(ReadoutPar(portnumber) == "ABB") {
       abbdevname = "ABBDevice";
       fABBActive = dabc::mgr()->CreateDevice("abb::Device", abbdevname.c_str());
   } else {
      // create dummy event generator module:

      dabc::mgr()->CreateModule("bnet::TestGeneratorModule", modulename.c_str(), SenderThreadName);
//      dabc::Module* m = new bnet::TestGeneratorModule(modulename.c_str());
//      dabc::mgr()->MakeThreadForModule(m, SenderThreadName);
      modulename += "/Output";
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

bool bnet::TestWorkerApplication::CreateCombiner(const char* name)
{
   return dabc::mgr()->CreateModule("bnet::TestCombinerModule", name, SenderThreadName);
}

bool bnet::TestWorkerApplication::CreateBuilder(const char* name)
{
   return dabc::mgr()->CreateModule("bnet::TestBuilderModule", name, ReceiverThreadName);
}

bool bnet::TestWorkerApplication::CreateFilter(const char* name)
{
   return dabc::mgr()->CreateModule("bnet::TestFilterModule", name, ReceiverThreadName);
}

bool bnet::TestWorkerApplication::CreateStorage(const char* portname)
{
   return bnet::WorkerApplication::CreateStorage(portname);
}

void bnet::TestWorkerApplication::DiscoverNodeConfig(dabc::Command* cmd)
{
   bnet::WorkerApplication::DiscoverNodeConfig(cmd);

   if (IsSender())
      SetParInt(xmlReadoutBuffer, GetParInt(xmlTransportBuffer) / NumReadouts());

   if (IsReceiver())
      SetParInt(xmlEventBuffer, GetParInt(xmlTransportBuffer) * (GetParInt(CfgNumNodes) - 1));

}



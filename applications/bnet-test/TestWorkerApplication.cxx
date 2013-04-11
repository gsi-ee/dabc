/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
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
   WorkerApplication(xmlTestWorkerClass) ,
   fABBActive(false)
{
   // register application specific parameters here:
}


bool bnet::TestWorkerApplication::CreateReadout(const char* portname, int portnumber)
{
   if (!IsGenerator()) return false;

   DOUT1("CreateReadout buf = %d", Par(xmlReadoutBuffer).AsInt());

   bool res = false;

   // check parameter if we should use abb at this readout:
   if(ReadoutPar(portnumber) == "ABB") {
       const char* abbdevname = "ABBDevice";
       fABBActive = dabc::mgr.CreateDevice("abb::Device", abbdevname);

       res = dabc::mgr.CreateTransport(portname, abbdevname);
       if (!res) EOUT("Cannot create ABB transport");
       dabc::mgr()->LogMessage(0,"created ABB readout");

   } else {
      // create dummy event generator module:

      std::string modulename = dabc::format("Readout%d", portnumber);
      dabc::mgr.CreateModule("bnet::TestGeneratorModule", modulename, SenderThreadName);
      res = dabc::mgr.Connect(modulename + "/Output", portname);
      fABBActive = false;
      dabc::mgr()->LogMessage(0,"created dummy event generator readout");
   }

   return res;
}

bool bnet::TestWorkerApplication::CreateCombiner(const char* name)
{
   dabc::mgr()->LogMessage(0,"Test of message");
   dabc::mgr()->LogMessage(1,"Test of warning");
   dabc::mgr()->LogMessage(2,"Test of error");
   return dabc::mgr.CreateModule("bnet::TestCombinerModule", name, SenderThreadName);
}

bool bnet::TestWorkerApplication::CreateBuilder(const char* name)
{
   return dabc::mgr.CreateModule("bnet::TestBuilderModule", name, ReceiverThreadName);
}

bool bnet::TestWorkerApplication::CreateFilter(const char* name)
{
   return dabc::mgr.CreateModule("bnet::TestFilterModule", name, ReceiverThreadName);
}

bool bnet::TestWorkerApplication::CreateStorage(const char* portname)
{
   return bnet::WorkerApplication::CreateStorage(portname);
}

void bnet::TestWorkerApplication::DiscoverNodeConfig(dabc::Command cmd)
{
   bnet::WorkerApplication::DiscoverNodeConfig(cmd);

   if (IsSender())
      Par(xmlReadoutBuffer).SetInt(Par(xmlTransportBuffer).AsInt() / NumReadouts());

   if (IsReceiver())
      Par(xmlEventBuffer).SetInt(Par(xmlTransportBuffer).AsInt() * (Par(CfgNumNodes).AsInt() - 1));
}

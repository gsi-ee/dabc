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
#include "dabc_root/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Url.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

#include "dabc_root/RootSniffer.h"

dabc::FactoryPlugin dabc_root_factory(new dabc_root::Factory("dabc_root"));

void dabc_root::Factory::Initialize()
{
   dabc_root::RootSniffer* serv = new dabc_root::RootSniffer("/ROOT");
   if (!serv->IsEnabled()) {
      delete serv;
   } else {
      DOUT0("Initialize ROOT sniffer");
      dabc::WorkerRef(serv).MakeThreadForWorker("RootThread");
   }
}

dabc::Reference dabc_root::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname=="dabc_root::RootSniffer")
      return new dabc_root::RootSniffer(objname, cmd);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}


dabc::DataInput* dabc_root::Factory::CreateDataInput(const std::string& typ)
{
   return 0;
}


dabc::DataOutput* dabc_root::Factory::CreateDataOutput(const std::string& typ)
{

   DOUT3("dabc_root::Factory::CreateDataOutput typ:%s", typ);

   dabc::Url url(typ);

   if (url.GetProtocol() == "root") {
      return 0;
   }

   return 0;
}



void dabc_root::StartHttpServer(int port)
{
   if (!dabc::mgr.null()) return;

   static dabc::Configuration dabc_root_cfg;

   new dabc::Manager("dabc", &dabc_root_cfg);

   dabc::mgr.SyncWorker();

   DOUT2("Create manager");

   // dabc::mgr.Execute("InitFactories");

   dabc::mgr.CreateThread("MainThread");


   dabc::CmdCreateObject cmd2("dabc_root::RootSniffer","/ROOT");
   cmd2.SetBool("enabled", true);
   cmd2.SetBool("batch", false);

   if (!dabc::mgr.Execute(cmd2)) return;

   dabc::WorkerRef w2 = cmd2.GetRef("Object");

   dabc_root::RootSniffer* sniff = dynamic_cast<dabc_root::RootSniffer*> (w2());
   if (sniff) sniff->InstallSniffTimer();

   w2.MakeThreadForWorker("MainThread");

   DOUT2("Create root sniffer");


   dabc::CmdCreateObject cmd1("http::Server","/http");
   cmd1.SetBool("enabled", true);
   cmd1.SetInt("port", port);
   cmd1.SetStr("top", "ROOT");
   cmd1.SetStr("select", "ROOT");

   if (!dabc::mgr.Execute(cmd1)) return;

   dabc::WorkerRef w1 = cmd1.GetRef("Object");
   w1.MakeThreadForWorker("MainThread");

   DOUT2("Create http server");
}

void dabc_root::ConnectMaster(const char* master_url)
{

}



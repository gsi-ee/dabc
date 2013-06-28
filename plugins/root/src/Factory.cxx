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

#include "dabc_root/functions.h"
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


bool dabc_root::StartHttpServer(int port)
{
   if (!dabc::mgr.null()) return false;

   static dabc::Configuration dabc_root_cfg;

   new dabc::Manager("dabc", &dabc_root_cfg);

   dabc::mgr.SyncWorker();

   DOUT2("Create manager");

   // dabc::mgr.Execute("InitFactories");

   dabc::mgr.CreateThread("MainThread");


   dabc::CmdCreateObject cmd2("dabc_root::RootSniffer","/ROOT");
   cmd2.SetBool("enabled", true);
   cmd2.SetBool("batch", false);

   if (!dabc::mgr.Execute(cmd2)) return false;

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

   if (!dabc::mgr.Execute(cmd1)) return false;

   dabc::WorkerRef w1 = cmd1.GetRef("Object");
   w1.MakeThreadForWorker("MainThread");

   DOUT2("Create http server");
   return true;
}

bool dabc_root::ConnectMaster(const char* master_url)
{
   if (!dabc::mgr.null()) return false;

   if ((master_url==0) || (strlen(master_url)==0)) {
      EOUT("Master name is not specified!!!");
      return false;
   }

   static dabc::Configuration dabc_root_cfg;

   new dabc::Manager("dabc", &dabc_root_cfg);

   dabc::mgr.SyncWorker();

   DOUT0("Create manager");

   // dabc::mgr.Execute("InitFactories");

   dabc::mgr.CreateThread("MainThread");

   dabc::CmdCreateObject cmd2("dabc_root::RootSniffer","/ROOT");
   cmd2.SetBool("enabled", true);
   cmd2.SetBool("batch", false);

   if (!dabc::mgr.Execute(cmd2)) return false;

   dabc::WorkerRef w2 = cmd2.GetRef("Object");

   dabc_root::RootSniffer* sniff = dynamic_cast<dabc_root::RootSniffer*> (w2());
   if (sniff) sniff->InstallSniffTimer();

   w2.MakeThreadForWorker("MainThread");

   DOUT0("Create root sniffer done");

   if (!dabc::mgr()->CreateControl(false)) {
      DOUT0("Cannot create control instance");
      return false;
   }

   std::string master = master_url;
   if (master.find("dabc://") != 0) master = std::string("dabc://") + master;

   DOUT0("Create command channel for %s ", master.c_str());

   dabc::Command cmd("ConfigureMaster");
   cmd.SetStr("Master", master);
   if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
      DOUT0("FAIL to activate connection to master %s", master.c_str());
      return false;
   }

   DOUT0("Master %s configured !!!", master.c_str());

   return true;


   /*

   dabc::Command cmd("Ping");
   cmd.SetReceiver(master);
   cmd.SetTimeout(5.);

   int res = dabc::mgr.GetCommandChannel().Execute(cmd);

   if (res != dabc::cmd_true) {
      DOUT0("FAIL connection to %s", master.c_str());
      return false;
   }

   DOUT0("Connection to %s done", master.c_str());

   dabc::Command cmd3("AcceptClient");
   cmd3.SetReceiver(master);
   cmd3.SetTimeout(5.);

   int res3 = dabc::mgr.GetCommandChannel().Execute(cmd3);

   if (res3 != dabc::cmd_true) {
      DOUT0("FAIL to activate server-client relation to %s", master.c_str());
      return false;
   }

   DOUT0("Master %s get control over local process!!!", master.c_str());

   return true;
*/
}



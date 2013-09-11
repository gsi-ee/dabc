#include "DabcRoot.h"

#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

#include "dabc_root/RootSniffer.h"

bool DabcRoot::StartHttpServer(int port, bool sync_timer)
{
   if (!dabc::mgr.null()) return false;

   static dabc::Configuration dabc_root_cfg;

   new dabc::Manager("dabc", &dabc_root_cfg);

   dabc::mgr.SyncWorker();

   DOUT2("Create manager");

   dabc::mgr.Execute("InitFactories");

   dabc::mgr.CreateThread("MainThread");

   dabc::mgr.CreatePublisher();

   dabc::CmdCreateObject cmd2("dabc_root::RootSniffer","/ROOT");
   cmd2.SetBool("enabled", true);
   cmd2.SetBool("batch", false);
   cmd2.SetBool("synctimer", sync_timer);
   cmd2.SetStr("prefix", "ROOT");

   if (!dabc::mgr.Execute(cmd2)) return false;

   dabc::WorkerRef w2 = cmd2.GetRef("Object");

   dabc_root::RootSniffer* sniff = dynamic_cast<dabc_root::RootSniffer*> (w2());
   if (sniff) sniff->InstallSniffTimer();

   w2.MakeThreadForWorker("MainThread");

   DOUT2("Create root sniffer");

   dabc::CmdCreateObject cmd1("http::Server","/http");
   cmd1.SetBool("enabled", true);
   cmd1.SetInt("port", port);
   cmd1.SetStr("topname", "ROOT");
   cmd1.SetStr("select", "ROOT");

   if (!dabc::mgr.Execute(cmd1)) return false;

   dabc::WorkerRef w1 = cmd1.GetRef("Object");
   w1.MakeThreadForWorker("MainThread");

   DOUT2("Create http server");
   return true;
}

bool DabcRoot::ConnectMaster(const char* master_url, bool sync_timer)
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

   dabc::mgr.Execute("InitFactories");

   dabc::mgr.CreateThread("MainThread");

   dabc::mgr.CreatePublisher();

   dabc::CmdCreateObject cmd2("dabc_root::RootSniffer","/ROOT");
   cmd2.SetBool("enabled", true);
   cmd2.SetBool("batch", false);
   cmd2.SetBool("synctimer", sync_timer);
   cmd2.SetStr("prefix", "ROOT/"+dabc::mgr.GetLocalId());

   if (!dabc::mgr.Execute(cmd2)) return false;

   dabc::WorkerRef w2 = cmd2.GetRef("Object");

   dabc_root::RootSniffer* sniff = dynamic_cast<dabc_root::RootSniffer*> (w2());
   if (sniff) sniff->InstallSniffTimer();

   w2.MakeThreadForWorker("MainThread");

   DOUT0("Create root sniffer done");

   // we selecting ROOT sniffer as the only objects, seen from the server
   if (!dabc::mgr()->CreateControl(false, "ROOT")) {
      DOUT0("Cannot create control instance");
      return false;
   }

   DOUT0("Create command channel for %s ", master_url);

   dabc::Command cmd("ConfigureMaster");
   cmd.SetStr("Master", master_url);
   cmd.SetStr("NameSufix", "ROOT");
   if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
      DOUT0("FAIL to activate connection to master %s", master_url);
      return false;
   }

   DOUT0("Master %s configured !!!", master_url);

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


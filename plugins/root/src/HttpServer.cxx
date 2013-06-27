#include "dabc_root/HttpServer.h"

#include "dabc_root/RootSniffer.h"

#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/Worker.h"

dabc::Configuration dabc_cfg;


void TestServer::Start(int port)
{
   dabc_root::HttpServer::Start(port);
}


void dabc_root::HttpServer::Start(int port)
{
   if (!dabc::mgr.null()) return;

   new dabc::Manager("cmd", &dabc_cfg);

   dabc::mgr.SyncWorker();

   DOUT0("Create manager");

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

   DOUT0("Create root sniffer");


   dabc::CmdCreateObject cmd1("http::Server","/http");
   cmd1.SetBool("enabled", true);
   cmd1.SetInt("port", port);
   cmd1.SetStr("top", "ROOT");
   cmd1.SetStr("select", "ROOT");

   if (!dabc::mgr.Execute(cmd1)) return;

   dabc::WorkerRef w1 = cmd1.GetRef("Object");
   w1.MakeThreadForWorker("MainThread");

   DOUT0("Create http server");




}

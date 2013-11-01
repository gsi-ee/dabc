#include "DabcRoot.h"

#include "dabc/Command.h"
#include "dabc/api.h"
#include "dabc/Manager.h"

#include "dabc_root/RootSniffer.h"

bool DabcRoot::StartHttpServer(int port, bool sync_timer)
{
   if (!dabc::mgr.null()) return false;

   dabc::SetDebugLevel(0);

   dabc::CreateManager("dabc", -1);

   // dabc::mgr.CreateThread("MainThread");

   dabc::mgr.CreatePublisher();

   dabc::CmdCreateObject cmd2("dabc_root::RootSniffer","/ROOT");
   cmd2.SetBool("enabled", true);
   cmd2.SetBool("batch", false);
   cmd2.SetBool("synctimer", sync_timer);
   cmd2.SetStr("prefix", "ROOT");

   dabc_root::RootSniffer* sniff = new dabc_root::RootSniffer("/ROOT", cmd2);
   sniff->InstallSniffTimer();
   dabc::WorkerRef w2 = sniff;
   w2.MakeThreadForWorker("MainThread");

   DOUT1("Create root sniffer %p thrdname %s", sniff, w2.thread().GetName());

   dabc::CmdCreateObject cmd1("http::Server","/http");
   cmd1.SetBool("enabled", true);
   cmd1.SetInt("port", port);

   if (!dabc::mgr.Execute(cmd1)) return false;

   dabc::WorkerRef w1 = cmd1.GetRef("Object");
   w1.MakeThreadForWorker("MainThread");

   return true;
}

bool DabcRoot::ConnectMaster(const char* master_url, bool sync_timer)
{
   if (!dabc::mgr.null()) return false;

   if ((master_url==0) || (strlen(master_url)==0)) {
      EOUT("Master name is not specified!!!");
      return false;
   }

   dabc::SetDebugLevel(0);

   dabc::CreateManager("dabc", 0);

//   dabc::mgr.CreateThread("MainThread");

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

   DOUT1("Create root sniffer done");

   // we selecting ROOT sniffer as the only objects, seen from the server
   if (!dabc::mgr()->CreateControl(false)) {
      DOUT0("Cannot create control instance");
      return false;
   }

   DOUT1("Create command channel for %s ", master_url);

   dabc::Command cmd("ConfigureMaster");
   cmd.SetStr("Master", master_url);
   cmd.SetStr("NameSufix", "ROOT");
   if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
      DOUT0("FAIL to activate connection to master %s", master_url);
      return false;
   }

   DOUT1("Master %s configured !!!", master_url);

   return true;
}


void DabcRoot::EnableDebug(bool on)
{
   dabc::SetDebugLevel(on ? 1 : 0);
}


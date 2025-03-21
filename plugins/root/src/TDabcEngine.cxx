// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "root/TDabcEngine.h"

#include "dabc/Command.h"
#include "dabc/api.h"
#include "dabc/Url.h"
#include "dabc/Manager.h"

#include "root/Monitor.h"

#include "THttpServer.h"

TDabcEngine::TDabcEngine() : THttpEngine("dabc","Bridge between DABC and ROOT")
{
   // constructor
}

TDabcEngine::~TDabcEngine()
{
   // destructor
}

Bool_t TDabcEngine::Create(const char *args)
{
   // create different kinds of DABC access to the ROOT server. Possible
   //
   // 1. DABC server
   // In this case args should be ":portnumber"
   // Optionally, one can allow clients to connect. Example
   //    serv->CreateEngine("dabc:1237?allowclients");
   //
   // 2. DABC slave
   // One could attach ROOT process to other DABC server. In this case one should
   // specify host and port number of the master DABC process like:
   //    serv->CreateEngine("dabc:masterhost:1237");
   //
   // 3. DABC http server
   // To start DABC version of http server, one should specify:
   //    serv->CreateEngine("dabc:http:8090");
   //
   // 4. DABC fastcgi server
   // To start DABC version of fastcgi server, one should specify:
   //    serv->CreateEngine("dabc:fastcgi:8090");
   // Optionally, one can enable debug mode for the fastcgi server "dabc:fastcgi:8090?debug"
   //
   // For all kinds of servers on can optionally specify name of top folder
   // For instance, when starting http server:
   //    serv->CreateEngine("dabc:http:8090?top=myapp");


   if (args) Info("Create", "args = %s", args);

   dabc::Url url;

   if (strncmp(args,"http:",5) == 0) {
      url.SetUrl(std::string("http://localhost:") + (args+5), false);
   } else
   if (strncmp(args,"fastcgi:",8) == 0) {
      url.SetUrl(std::string("fastcgi://localhost:") + (args+8), false);
   } else
   if (strncmp(args,"master:",7) == 0) {
      url.SetUrl(std::string("dabc://") + (args+7), false);
   } else
   if (strchr(args,':')) {
      url.SetUrl(std::string("dabc://") + args, false);
   } else {
      url.SetUrl(std::string("server://localhost:") + args, false);
   }

   if (!url.IsValid()) {
      EOUT("Wrong arguments %s", args);
      return kFALSE;
   }

   std::string topfolder = url.GetOptionStr("top", "ROOT");

   if (dabc::mgr.null()) {
      dabc::SetDebugLevel(0);
      dabc::CreateManager("dabc", -1);
   }

   // creating web server
   if (url.GetProtocol() == "http") {
      if (dabc::mgr.FindItem("/http").null()) {
         dabc::CmdCreateObject cmd1("http::Civetweb","/http");
         cmd1.SetInt("port", url.GetPort());
         cmd1.SetStr("urlopt", url.GetOptions());
         if (!dabc::mgr.Execute(cmd1)) return kFALSE;

         dabc::WorkerRef w1 = cmd1.GetRef("Object");
         w1.MakeThreadForWorker("MainThread");
      }
   } else
   // creating fastcgi server
   if (url.GetProtocol() == "fastcgi") {
      if (dabc::mgr.FindItem("/fastcgi").null()) {
         dabc::CmdCreateObject cmd1("http::FastCgi","/fastcgi");
         cmd1.SetInt("port", url.GetPort());
         cmd1.SetBool("debug", url.HasOption("debug"));
         cmd1.SetStr("urlopt", url.GetOptions());
         if (!dabc::mgr.Execute(cmd1)) return kFALSE;

         dabc::WorkerRef w1 = cmd1.GetRef("Object");
         w1.MakeThreadForWorker("MainThread");
      }
      return kTRUE;
   } else
   // connect to master
   if (url.GetProtocol() == "dabc") {

      std::string master_url = url.GetHostNameWithPort();

      // we selecting ROOT sniffer as the only objects, seen from the server
      if (!dabc::mgr.CreateControl(false)) {
         DOUT0("Cannot create control instance");
         return false;
      }

      DOUT1("Create slave command channel for master %s ", master_url.c_str());

      dabc::Command cmd("ConfigureMaster");
      cmd.SetStr("Master", master_url);
      cmd.SetStr("NameSufix", topfolder);
      if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
         DOUT0("FAIL to activate connection to master %s", master_url.c_str());
         return kFALSE;
      }
   } else {
      if (!dabc::mgr.CreateControl(true, url.GetPort(), url.HasOption("allowclients"))) return kFALSE;
   }

   dabc::mgr.CreatePublisher();

   if (dabc::mgr.FindItem("/ROOT").null()) {

      std::string player_class = url.GetOptionStr("player", "root::Monitor");

      dabc::CmdCreateObject cmd2(player_class,"/ROOT");
      cmd2.SetBool("enabled", true);
      cmd2.SetStr("prefix", topfolder);

      if (!dabc::mgr.Execute(cmd2)) return kFALSE;
      dabc::WorkerRef player = cmd2.GetRef("Object");

      if (player.null()) return kFALSE;

      player.MakeThreadForWorker("MainThread");
      DOUT1("Create root player %p thrdname %s", player(), player.thread().GetName());
   }

   return kTRUE;
}


void TDabcEngine::Process()
{
   // method called from ROOT context and used to perform actions in root context

   if (!GetServer() || !GetServer()->GetSniffer()) return;

   root::MonitorRef player = dabc::mgr.FindItem("/ROOT");

   player.ProcessActionsInRootContext(GetServer(), GetServer()->GetSniffer());
}

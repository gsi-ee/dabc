// $Id$

#include "DabcRoot.h"

#include <string>

#include "TObject.h"
#include "TRootSniffer.h"
#include "THttpServer.h"

#include "dabc/Command.h"
#include "dabc/api.h"
#include "dabc/Manager.h"

#include "root/Monitor.h"

static std::string gDabcRootTopFolder = "ROOT";

static THttpServer* gDabcHttpServer = 0;


void DabcRoot::SetTopFolderName(const char* name)
{
   if (name && *name && strpbrk(name,"/\\[]#()")==0) {
      gDabcRootTopFolder = name;
   } else {
      EOUT("Wrong top folder name %s", name ? name : "(null)");
   }
}

bool DabcRoot::StartHttpServer(int port, bool sync_timer)
{
   if (gDabcHttpServer==0) gDabcHttpServer = new THttpServer("");

   if (!sync_timer) gDabcHttpServer->SetTimer(100, kFALSE);

   std::string arg = dabc::format("TDabcEngine:http:%d", port);

   if (gDabcRootTopFolder!="ROOT")
      arg += dabc::format("?top=%s", gDabcRootTopFolder.c_str());

   return gDabcHttpServer->CreateEngine(arg.c_str());
}


bool DabcRoot::StartDabcServer(int port, bool sync_timer, bool allow_slaves)
{
   if (gDabcHttpServer==0) gDabcHttpServer = new THttpServer;

   if (!sync_timer) gDabcHttpServer->SetTimer(100, kFALSE);

   std::string arg = dabc::format("TDabcEngine:%d", port);

   const char* separ = "?";

   if (allow_slaves) { arg+="?allowclients"; separ = "&"; }

   if (gDabcRootTopFolder!="ROOT")
      arg += dabc::format("%stop=%s", separ, gDabcRootTopFolder.c_str());

   return gDabcHttpServer->CreateEngine(arg.c_str());
}


bool DabcRoot::ConnectMaster(const char* master_url, bool sync_timer)
{
   if (gDabcHttpServer==0) gDabcHttpServer = new THttpServer;

   if (!sync_timer) gDabcHttpServer->SetTimer(100, kFALSE);

   std::string arg = dabc::format("TDabcEngine:master:%s", master_url);

   if (gDabcRootTopFolder!="ROOT")
      arg += dabc::format("?top=%s", gDabcRootTopFolder.c_str());

   return gDabcHttpServer->CreateEngine(arg.c_str());
}


void DabcRoot::EnableDebug(bool on)
{
   dabc::SetDebugLevel(on ? 1 : 0);
}

bool DabcRoot::Register(const char* folder, TObject* obj)
{
   if (gDabcHttpServer==0) return false;

   return gDabcHttpServer->Register(folder, obj);
}

bool DabcRoot::Unregister(TObject* obj)
{
   if (gDabcHttpServer==0) return false;

   return gDabcHttpServer->Unregister(obj);
}


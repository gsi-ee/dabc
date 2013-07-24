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

#include "http/Server.h"

#include <fstream>
#include <stdlib.h>

#include "dabc/threads.h"

#include "dabc/Hierarchy.h"
#include "dabc/Manager.h"
#include "dabc/Url.h"


static int begin_request_handler(struct mg_connection *conn)
{
   http::Server* serv = (http::Server*) mg_get_request_info(conn)->user_data;

   return serv->begin_request(conn);
}


static const char* open_file_handler(const struct mg_connection* conn,
                                     const char *path, size_t *data_len)
{
   http::Server* serv = (http::Server*) mg_get_request_info((struct mg_connection*)conn)->user_data;

   return serv->open_file(conn, path, data_len);

}


http::Server::Server(const std::string& name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fHttpPort(0),
   fServerFolder("localhost"),
   fTop2Name(),
   fSelPath(""),
   fClientsFolder("Clients"),
   fShowClients(false),
   fHierarchy(),
   fHierarchyMutex(),
   fCtx(0),
   fFiles(),
   fHttpSys(),
   fGo4Sys(),
   fRootSys(),
   fJSRootIOSys()
{
   memset(&fCallbacks, 0, sizeof(fCallbacks));
   fCallbacks.begin_request = begin_request_handler;
   fCallbacks.open_file = open_file_handler;

   fHttpPort = Cfg("port", cmd).AsUInt(8080);
   fEnabled = Cfg("enabled", cmd).AsBool(false);
   if (fHttpPort<=0) fEnabled = false;

   if (!fEnabled) return;

   fServerFolder = Cfg("topname", cmd).AsStdStr(fServerFolder);
   fSelPath = Cfg("select", cmd).AsStdStr(fSelPath);
   fShowClients = Cfg("clients", cmd).AsBool(fShowClients);

   fHttpSys = ".";
}

http::Server::~Server()
{
   if (fCtx!=0) mg_stop(fCtx);

   fCtx = 0;

   //fHierarchy.Destroy();
}


void http::Server::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("http server was not enabled - why it is started??");
      return;
   }
   const char* dabcsys = getenv("DABCSYS");
   if (dabcsys!=0) fHttpSys = dabc::format("%s/plugins/http", dabcsys);

   const char* go4sys = getenv("GO4SYS");
   if (go4sys!=0) fGo4Sys = go4sys;

   const char* rootsys = getenv("ROOTSYS");
   if (rootsys!=0) fRootSys = rootsys;

   const char* jsrootiosys = getenv("JSROOTIOSYS");
   if (jsrootiosys!=0) fJSRootIOSys = jsrootiosys;
   if (fJSRootIOSys.empty()) fJSRootIOSys = fHttpSys + "/JSRootIO";

   DOUT0("JSROOTIOSYS = %s ", fJSRootIOSys.c_str());

   DOUT0("Starting HTTP server on port %d", fHttpPort);
   DOUT0("HTTPSYS = %s", fHttpSys.c_str());

   std::string sport = dabc::format("%d", fHttpPort);

   const char *options[3];
   options[0] = "listening_ports";
   options[1] = sport.c_str();
   options[2] = 0;

   // Start the web server.
   fCtx = mg_start(&fCallbacks, this, options);

   fHierarchy.Create("Full");

   DOUT0("CLIENTS = %s", DBOOL(fShowClients));

   if (!fShowClients) {
      fClientsFolder = "";
      if (fServerFolder.empty()) fServerFolder = "localhost";
   } else {
      if (!fServerFolder.empty()) {
         fTop2Name = fServerFolder;
         fServerFolder = "Server";
      }
   }

   if (!fServerFolder.empty()) {
      dabc::Hierarchy h = fHierarchy.CreateChild(fServerFolder);
      if (!fTop2Name.empty()) h.CreateChild(fTop2Name.c_str());
   }

   if (!fClientsFolder.empty())
      fHierarchy.CreateChild(fClientsFolder);

   ActivateTimeout(0);
}

double http::Server::ProcessTimeout(double last_diff)
{
   // dabc::LockGuard lock(fHierarchyMutex);

//   DOUT0("\n\n\n========================= START BUILDING ================");


   dabc::Hierarchy server_hierarchy;
   server_hierarchy.Create("Full");

   if (!fServerFolder.empty()) {
      dabc::Hierarchy h = server_hierarchy.CreateChild(fServerFolder);
      if (!fTop2Name.empty()) h = h.CreateChild(fTop2Name.c_str());
      dabc::Reference main = dabc::mgr;
      if (!fSelPath.empty()) main = main.FindChild(fSelPath.c_str());
      h.Build("", main);
   }

   // we build extra slaves when they not shown in main structure anyway
   if (fShowClients && !fClientsFolder.empty()) {
      dabc::WorkerRef chl = dabc::mgr.GetCommandChannel();
      dabc::Hierarchy h = server_hierarchy.CreateChild(fClientsFolder);

      dabc::Command cmd("BuildClientsHierarchy");
      cmd.SetPtr("Container", h());
      chl.Execute(cmd);
   }

   dabc::LockGuard lock(fHierarchyMutex);

   fHierarchy.Update(server_hierarchy);


   // this is method to enable history recording on server side -
   // via configuration one could record history for all ratemeters, for instance
   // TODO: implement such configuration

//   dabc::Hierarchy item = fHierarchy.FindChild("Server/Gener/App/Multi/GRate");
//   if (!item.null()) item.EnableHistory(100);

//   item = fHierarchy.FindChild("Server/Gener/App/fesa/BeamRate");
//   if (!item.null()) item.EnableHistory(100);

   //server_hierarchy.Destroy();

   return 1.;
}


int http::Server::begin_request(struct mg_connection *conn)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   DOUT2("BEGIN_REQ: %s", request_info->uri);

   std::string content;
   std::string content_type = "text/html";
   bool iserror = false;

   if ((strcmp(request_info->uri,"/")==0) ||
       (strcmp(request_info->uri,"/main.htm")==0) || (strcmp(request_info->uri,"/main.html")==0) ||
       (strcmp(request_info->uri,"/index.htm")==0) || (strcmp(request_info->uri,"/main.html")==0)) {
      std::string content_type = "text/html";
      content = open_file(0, "httpsys/files/main.htm");
   } else
   if (strstr(request_info->uri,"/h.xml")!=0) {
      content_type = "text/xml";

      dabc::LockGuard lock(fHierarchyMutex);

      content =
            std::string("<?xml version=\"1.0\"?>\n") +
            std::string("<dabc version=\"2\" xmlns:dabc=\"http://dabc.gsi.de/xhtml\">\n")+
            fHierarchy.SaveToXml(false) +
            std::string("</dabc>\n");
   } else
   if (strstr(request_info->uri, "chartreq.htm")!=0) {
      content_type = "text/plain";

      const char* res = open_file(conn, request_info->uri, 0);
      if (res) content = res;

   } else
   if (strstr(request_info->uri, "getbinary")!=0) {
      if (ProcessGetBinary(conn, request_info->query_string)) return 1;

      iserror = true;
   } else
   if (strstr(request_info->uri, "gethistory")!=0) {
      if (ProcessGetHistory(conn, request_info->query_string)) return 1;

      iserror = true;
   } else {
      // let load some files

      DOUT2("**** GET REQ:%s query:%s", request_info->uri, request_info->query_string ? request_info->query_string : "---");

      if ((request_info->uri[0]=='/') && (strstr(request_info->uri,"httpsys")==0) && (strstr(request_info->uri,".htm")!=0) ) {
         std::string fname = "httpsys/files";
         fname += request_info->uri;
         const char* res = open_file(0, fname.c_str(), 0);
         if (res!=0) content = res;
      }

      if (content.empty()) return 0;
   }

   // Prepare the message we're going to send

   if (iserror) {
      mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n");
   } else {

      // Send HTTP reply to the client
      mg_printf(conn,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"        // Always set Content-Length
             "\r\n"
             "%s",
             content_type.c_str(),
             (int) content.length(),
             content.c_str());
   }

   // Returning non-zero tells mongoose that our function has replied to
   // the client, and mongoose should not send client any more data.
   return 1;
}

bool http::Server::MakeRealFileName(std::string& fname)
{
   if (fname.empty()) return false;
   size_t pos = fname.rfind("httpsys/");
   if (pos!=std::string::npos) {
      fname.erase(0, pos+7);
      fname = fHttpSys + fname;
      return true;
   }

   if (!fRootSys.empty()) {
      pos = fname.rfind("rootsys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+7);
         fname = fRootSys + fname;
         return true;
      }
   }

   if (!fJSRootIOSys.empty()) {
      pos = fname.rfind("jsrootiosys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+11);
         fname = fJSRootIOSys + fname;
         return true;
      }
   }

   if (!fGo4Sys.empty()) {
      pos = fname.rfind("go4sys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+6);
         fname = fGo4Sys + fname;
         return true;
      }
   }

   return false;
}



const char* http::Server::open_file(const struct mg_connection* conn,
                                    const char *path, size_t *data_len)
{
   if ((path==0) || (*path==0)) return 0;

   if (strstr(path,"getbinary")!=0)
      return 0;

   const struct mg_request_info *request_info = 0;
   if (conn!=0) request_info = mg_get_request_info((struct mg_connection*)conn);

   bool force = true;

   const char* query = 0;
   const char* meth = 0;
   if (request_info) {
      if (request_info->query_string) query = request_info->query_string;
      if (request_info->request_method) meth = request_info->request_method;
   }

   DOUT0("Request file %s  meth:%s query:%s", path, meth ? meth : "---", query ? query : "---");

   char* buf = 0;
   int length = 0;
   std::string fname(path);

   if (strstr(path,"chartreq.htm")!=0) {

      // DOUT0("Request chartreq.htm processed");

      force = true;

      std::string rates = query ? query : "";

      std::string content = "[\n";
      bool first = true;

      dabc::LockGuard lock(fHierarchyMutex);

      while (rates.length() > 0) {
         size_t pos = rates.find('&');
         std::string part;
         if (pos==std::string::npos) {
            part = rates;
            rates.clear();
         } else {
            part.append(rates, 0, pos);
            rates.erase(0, pos+1);
         }

         if (!first) content+=",\n";

         dabc::Hierarchy chld = fHierarchy.FindChild(part.c_str());

         if (chld.null()) { EOUT("Didnot find child %s", part.c_str()); }

         if (!chld.null() && chld.HasField("value")) {
            content += dabc::format("   { \"name\" : \"%s\" , \"value\" : \"%s\" }", part.c_str(), chld.GetField("value"));
            first = false;
         }
      }

      content += "\n]\n";


      length = content.length();
      buf = (char*) malloc(length+1);
      strncpy(buf, content.c_str(), length+1);
      buf[length] = 0;

      // DOUT0("Produced buffer = %s", buf);
   }

   if (buf==0) {

      if (!MakeRealFileName(fname)) return 0;

      if (!force) {
         dabc::LockGuard lock(ObjectMutex());

         FilesMap::iterator iter = fFiles.find(fname);
         if (iter!=fFiles.end()) {
            if (data_len) *data_len = iter->second.size;
            DOUT2("Return file %s len %d", fname.c_str(), iter->second.size);
            return (const char*) iter->second.ptr;
         }
      }

      std::ifstream is(fname.c_str());

      if (!is) return 0;

      is.seekg (0, is.end);
      length = is.tellg();
      is.seekg (0, is.beg);

      buf = (char*) malloc(length+1);
      is.read(buf, length);
      if (is)
         DOUT2("all characters read successfully from file.");
      else
        EOUT("only %d could be read from %s", is.gcount(), path);
   }

   buf[length] = 0;

   dabc::LockGuard lock(ObjectMutex());

   FilesMap::iterator iter = fFiles.find(fname);

   if (iter == fFiles.end()) {
      fFiles[fname] = FileBuf();
      iter = fFiles.find(fname);
   } else {
      iter->second.release();
   }

   if (iter!=fFiles.end()) {

      iter->second.ptr = buf;
      iter->second.size = length;

      if (data_len) *data_len = iter->second.size;

      DOUT2("Return file %s len %u", path, iter->second.size);

      return (const char*) iter->second.ptr;
   }

   EOUT("Did not find file %s %s", path, fname.c_str());

   return 0;
}

int http::Server::ProcessGetHistory(struct mg_connection* conn, const char *query)
{
   dabc::Url url(std::string("gethistory?") + (query ? query : ""));

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
      return 0;
   }

   std::string itemname = url.GetOptionsPart(0);
   if (itemname.empty()) {
      EOUT("Item is not specified in gethistory request");
      return 0;
   }

   std::string sver = url.GetOptionStr("ver");
   long unsigned query_version(0);
   if (sver.length()>0)
      if (!dabc::str_to_luint(sver.c_str(), &query_version)) query_version = 0;
   int limit = url.GetOptionInt("limit", 0);

   int check_requester_counter = 0;
   std::string reply;
   dabc::Hierarchy item;
   dabc::Command cmd;

   while (check_requester_counter < 100) {

      if (check_requester_counter++>0) WorkerSleep(0.05);

      dabc::LockGuard lock(fHierarchyMutex);
      item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("Wrong request for non-existing item %s", itemname.c_str());
         break;
      }

      if (item.HasField("#doingreq")) { item.Release(); continue; }

      // process request locally
      if (item.HasLocalHistory() || item.HasActualRemoteHistory()) {
         reply = item()->RequestHistory(query_version, limit);
         break;
      }

      cmd = item.ProduceHistoryRequest();
      if (!cmd.null()) item.SetField("#doingreq","1");

      break;
   }

   if (!cmd.null()) {
      // try to execute command, which should obtain request
      dabc::mgr.Execute(cmd, 5.);

      dabc::LockGuard lock(fHierarchyMutex);
      item.SetField("#doingreq",0);

      if (item.ApplyHierarchyRequest(cmd))
         reply = item()->RequestHistory(query_version, limit);
   }

   if (reply.empty()) {
      EOUT("HISTORY REQUESTER FAILS %s", itemname.c_str());

      mg_printf(conn, "HTTP/1.1 500 Server Error\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: close\r\n\r\n");
   } else {
      mg_printf(conn,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/xml\r\n"
                 "Content-Length: %u\r\n"
                 "Connection: keep-alive\r\n"
                  "\r\n", (unsigned) reply.length());
      mg_write(conn, reply.c_str(), (size_t) reply.length());
   }
   return 1;
}



int http::Server::ProcessGetBinary(struct mg_connection* conn, const char *query)
{
   dabc::Url url(std::string("getbinary?") + (query ? query : ""));

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
      return 0;
   }

   std::string itemname = url.GetOptionsPart(0);
   if (itemname.empty()) {
      EOUT("Item is not specified in getbinary request");
      return 0;
   }

   std::string sver = url.GetOptionStr("ver");
   long unsigned query_version(0);
   if (sver.length()>0)
      if (!dabc::str_to_luint(sver.c_str(), &query_version)) query_version = 0;

   dabc::Hierarchy item;
   dabc::Buffer replybuf;
   dabc::Command cmd;

   bool force = false;

   int check_requester_counter = 0;

   // we need this loop in the case, when several threads want to get new
   // binary data from item

   // in this case one thread will initiate it and other threads should loop and wait until
   // version of binary data is changed

   while (check_requester_counter < 100) {

      if (check_requester_counter++>0) WorkerSleep(0.05);

      dabc::LockGuard lock(fHierarchyMutex);

      item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("Wrong request for non-existing item %s", itemname.c_str());
         return 0;
      }

      if (item.HasField("#doingreq")) { item.Release(); continue; }

      if (!force) replybuf = item.GetBinaryData(query_version);

      if (!replybuf.null()) break;

      cmd = item.ProduceBinaryRequest();
      if (!cmd.null()) item.SetField("#doingreq","1");
      break;
   } // end of check_requester_counter


   if (!cmd.null()) {
      dabc::mgr.Execute(cmd, 5.);

      dabc::LockGuard lock(fHierarchyMutex);

      item.SetField("#doingreq",0);

      replybuf = item.ApplyBinaryRequest(cmd);
   }


   if (replybuf.null()) {
      mg_printf(conn, "HTTP/1.1 500 Server Error\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: close\r\n\r\n");
   } else {

      mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/x-binary\r\n"
            "Content-Length: %u\r\n"
            "Connection: keep-alive\r\n"
            "\r\n", (unsigned) replybuf.GetTotalSize());
      mg_write(conn, replybuf.SegmentPtr(), (size_t) replybuf.GetTotalSize());
   }

   return 1;
}

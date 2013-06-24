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


http::Server::Server(const std::string& name) :
   dabc::Worker(MakePair(name)),
   fHttpPort(0),
   fEnabled(false),
   fHierarchy(),
   fHierarchyMutex(),
   fCtx(0),
   fFiles(),
   fHttpSys()
{
   memset(&fCallbacks, 0, sizeof(fCallbacks));
   fCallbacks.begin_request = begin_request_handler;
   fCallbacks.open_file = open_file_handler;

   fHttpPort = Cfg("port").AsUInt(8080);
   fEnabled = Cfg("enabled").AsBool(true);
   if (fHttpPort<=0) fEnabled = false;

   if (!fEnabled) return;

   fHttpSys = ".";
}

http::Server::~Server()
{
   if (fCtx!=0) mg_stop(fCtx);

   fCtx = 0;
}


void http::Server::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("http server was not enabled - why it is started??");
      return;
   }
   const char* dabcsys = getenv("DABCSYS");
   if (dabcsys!=0) fHttpSys = dabc::format("%s/plugins/http", dabcsys);

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

   fHierarchy.CreateChild("localhost");

   ActivateTimeout(0);
}

double http::Server::ProcessTimeout(double last_diff)
{
   // dabc::LockGuard lock(fHierarchyMutex);

   dabc::Hierarchy local = fHierarchy.FindChild("localhost");

   if (local.null()) {
      EOUT("Did not find localhost in hierarchy");
   } else {
      // TODO: make via XML file like as for remote node!!

      local.UpdateHierarchy(dabc::mgr);
   }

   return 1.;
}


int http::Server::begin_request(struct mg_connection *conn)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   DOUT0("BEGIN_REQ: %s", request_info->uri);

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
      content_type = "xml";

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
   } else {
      // let load some files

      DOUT0("**** GET REQ:%s query:%s", request_info->uri, request_info->query_string ? request_info->query_string : "---");

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

      DOUT0("Request chartreq.htm processed");

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

      DOUT0("Produced buffer = %s", buf);
   }

   if (buf==0) {
      size_t pos = fname.rfind("httpsys/");
      if (pos==std::string::npos) return 0;
      fname.erase(0, pos+7);
      fname = fHttpSys + fname;

      if (!force) {
         dabc::LockGuard lock(ObjectMutex());

         FilesMap::iterator iter = fFiles.find(fname);
         if (iter!=fFiles.end()) {
            if (data_len) *data_len = iter->second.size;
            DOUT0("Return file %s len %d", fname.c_str(), iter->second.size);
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
         DOUT0("all characters read successfully from file.");
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

      DOUT0("Return file %s len %u", path, iter->second.size);

      return (const char*) iter->second.ptr;
   }

   EOUT("Did not find file %s %s", path, fname.c_str());

   return 0;
}

int http::Server::ProcessGetBinary(struct mg_connection* conn, const char *query)
{
   std::string itemname = query ? query : "";
   std::string sver;
   long unsigned query_version(0);
   uint64_t item_version(0);

   size_t pos = itemname.find("&");
   if (pos != std::string::npos) {
      sver = itemname.substr(pos+1);
      itemname.erase(pos);
   }

   if (itemname.empty()) {
      EOUT("Item is not specified in getbinary request");
      return 0;
   }

   if  (sver.find("ver=") == 0) {
      sver.erase(0,4);
      if (!dabc::str_to_luint(sver.c_str(), &query_version)) query_version = 0;
   }

   dabc::Hierarchy item;
   dabc::BinData bindata;
   std::string producer_name, request_name;
   dabc::WorkerRef wrk;

   bool force = false;

   int check_requester_counter = 100;

   // we need this loop in the case, when several threads want to get new
   // binary data from item

   // in this case one thread will initiate it and other threads should loop and wait until
   // version of binary data is changed

   while (check_requester_counter-- > 0) {

      {
         dabc::LockGuard lock(fHierarchyMutex);
         item = fHierarchy.FindChild(itemname.c_str());

         if (item.null()) {
            EOUT("Wrong request for non-existing item %s", itemname.c_str());
            return 0;
         }

         // take data under lock that we are sure - nothing change for me
         item_version = item.GetVersion();
         bindata = item()->bindata();
      }

      DOUT0("BINARY REQUEST name:%s CURRENT:%u REQUESTED:%u", itemname.c_str(), (unsigned) item_version, (unsigned) query_version);

      // we only can reply if we know that buffered information is not old
      // user can always force new buffer if he provide too big qyery version number

      bool can_reply = !bindata.null() && (item_version==bindata.version()) && (query_version<=item_version) && !force;

      if (can_reply) {
         DOUT0("!!!! BINRARY READY %s sz %u", itemname.c_str(), bindata.length());

         unsigned reply_length = bindata.length();

         // we do not need to send data again while requester give as same number again
         if ((query_version>0) && (query_version==bindata.version())) {
            reply_length = 0;
            DOUT0("!!!! NO NEED TO SEND DATA AGAIN");
         }

         mg_printf(conn,
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/x-binary\r\n"
               "Content-Version: %u\r\n"
               "Content-Length: %u\r\n"
               "Connection: keep-alive\r\n"
               "\r\n",
               (unsigned) bindata.version(), reply_length);
         if (reply_length>0)
            mg_write(conn, bindata.data(), (size_t) reply_length);
         return 1;
      }

      dabc::Hierarchy parent = item;

      while (!parent.null()) {
         if (parent.HasField(dabc::prop_binary_producer)) {
            producer_name = parent.Field(dabc::prop_binary_producer).AsStdStr();
            request_name = item.RelativeName(parent);
            break;
         }
         parent = (dabc::HierarchyContainer*) parent.GetParent();
      }

      if (!producer_name.empty()) wrk = dabc::mgr.FindItem(producer_name);

      if (wrk.null() || request_name.empty()) {
         EOUT("NO WAY TO GET BINARY DATA %s ", itemname.c_str());
         return 0;
      }

      {
         dabc::LockGuard lock(fHierarchyMutex);
         if (!item.HasField("#doingreq")) {
            item.SetField("#doingreq","1");
            break;
         }
      }
      DOUT0("There is running request - loop again");
      wrk.Release();
      item.Release();
      producer_name.clear();
      request_name.clear();

      WorkerSleep(0.05);
   } // end of check_requester_counter

   if (check_requester_counter <= 0) {
      EOUT("BINARY REQUESTER BLOCKING FOR VERY LONG TIME");

      mg_printf(conn, "HTTP/1.1 500 Server Error\r\n"
                       "Content-Length: 0\r\n"
                        "Connection: close\r\n\r\n");
      return 1;
   }

   DOUT0("GETBINARY name:%s %s %s" , itemname.c_str(), producer_name.c_str(), request_name.c_str());

   dabc::Command cmd("GetBinary");
   cmd.SetStr("Item", request_name);

   DOUT0("************* EXECUTING GETBINARY COMMAND *****************");

   bool resok = true;

   if (wrk.Execute(cmd, 5.) != dabc::cmd_true) {
      EOUT("Fail to get binary data");
      resok = false;
   } else {
      bindata = cmd.GetRef("#BinData");

      dabc::LockGuard lock(fHierarchyMutex);
      item.SetField("#doingreq",0);
      if (!bindata.null()) {
         bindata()->SetVersion(item_version);
         item()->bindata() = bindata;
      } else {
         resok = false;
      }
   }

   if (resok) {
      DOUT0("Send binary data %u!!!!", bindata.length());

      mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/x-binary\r\n"
            "Content-Version: %u\r\n"
            "Content-Length: %u\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            (unsigned) bindata.version(), bindata.length());
      mg_write(conn, bindata.data(), (size_t) bindata.length());
   } else {
      mg_printf(conn, "HTTP/1.1 500 Server Error\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: close\r\n\r\n");
   }

   return 1;
}



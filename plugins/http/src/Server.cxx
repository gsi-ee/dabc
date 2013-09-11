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
#include "dabc/Publisher.h"

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

   ActivateTimeout(0);
}

double http::Server::ProcessTimeout(double last_diff)
{
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

      std::string xmlcode;

      if (dabc::PublisherRef(GetPublisher()).SaveGlobalNamesListAsXml(xmlcode))
         content = std::string("<?xml version=\"1.0\"?>\n") +
                   std::string("<dabc version=\"2\" xmlns:dabc=\"http://dabc.gsi.de/xhtml\">\n")+
                   xmlcode +
                   std::string("</dabc>\n");
      else
         iserror = true;
   } else
   if (strstr(request_info->uri, "getbin")!=0) {
      if (ProcessGetBin(conn, request_info->query_string)) return 1;

      iserror = true;
   } else
   if (strstr(request_info->uri, "getimage.png")!=0) {
      if (ProcessGetImage(conn, request_info->query_string)) return 1;
      iserror = true;
   } else
   if (strstr(request_info->uri, "getitem")!=0) {
      if (ProcessGetItem(conn, request_info->query_string)) return 1;
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

   DOUT4("Request file %s  meth:%s query:%s", path, meth ? meth : "---", query ? query : "---");

   char* buf = 0;
   int length = 0;
   std::string fname(path);

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


int http::Server::ProcessGetImage(struct mg_connection* conn, const char *query)
{
   dabc::Url url(std::string("getbin?") + (query ? query : ""));

   std::string itemname;
   uint64_t version(0);

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
   } else {
      itemname = url.GetOptionsPart(0);
      if (url.HasOption("version")) {
         int v = url.GetOptionInt("version", 0);
         if (v>0) version = (unsigned) v;
      }
   }

   dabc::Buffer replybuf;

   if (!itemname.empty())
      replybuf = dabc::PublisherRef(GetPublisher()).GetBinary(itemname, version);

   if (replybuf.null()) {
      EOUT("IMAGE REQUEST FAILS %s", itemname.c_str());

      mg_printf(conn, "HTTP/1.1 500 Server Error\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: close\r\n\r\n");
   } else {

      // DOUT0("HISTORY ver %u REPLY\n%s", (unsigned) query_version, reply.c_str());

      unsigned image_size = replybuf.GetTotalSize() - sizeof(dabc::BinDataHeader);

      mg_printf(conn,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: image/png\r\n"
                 "Content-Length: %u\r\n"
                 "Connection: keep-alive\r\n"
                  "\r\n", image_size);
      mg_write(conn, ((char*) replybuf.SegmentPtr()) + sizeof(dabc::BinDataHeader), (size_t) image_size);
   }

   return 1;
}


int http::Server::ProcessGetItem(struct mg_connection* conn, const char *query)
{
   dabc::Url url(std::string("getitem?") + (query ? query : ""));

   std::string itemname;
   unsigned hlimit(0);
   uint64_t version(0);

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
   } else {
      itemname = url.GetOptionsPart(0);
      if (itemname.empty()) {
         EOUT("Item is not specified in getitem request");
      }
      if (url.HasOption("history")) {
         int hist = url.GetOptionInt("history", 0);
         if (hist>0) hlimit = (unsigned) hist;
      }
      if (url.HasOption("version")) {
         int v = url.GetOptionInt("version", 0);
         if (v>0) version = (unsigned) v;
      }

   }

   DOUT0("HLIMIT = %u query = %s", hlimit, query);

   std::string replybuf;

   if (!itemname.empty()) {
      dabc::Hierarchy res;

      res = dabc::PublisherRef(GetPublisher()).Get(itemname, version, hlimit);

      if (!res.null()) {
         // result is only item fields, we need to decorate it with some more attributes

         replybuf = dabc::format("<Reply xmlns:dabc=\"http://dabc.gsi.de/xhtml\" itemname=\"%s\" %s=\"%lu\">\n",itemname.c_str(), dabc::prop_version, (long unsigned) res.GetVersion());

         replybuf += res.SaveToXml(hlimit > 0 ? dabc::xmlmask_History : 0);
         replybuf += "</Reply>";
         // DOUT0("getitem %s\n%s", itemname.c_str(), replybuf.c_str());
      }
   }

   if (replybuf.empty()) {
      mg_printf(conn, "HTTP/1.1 500 Server Error\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: close\r\n\r\n");
   } else {

      mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/xml\r\n"
            "Content-Length: %u\r\n"
            "Connection: keep-alive\r\n"
            "\r\n", (unsigned) replybuf.length());
      mg_write(conn, replybuf.c_str(), (size_t) replybuf.length());
   }

   return 1;
}


int http::Server::ProcessGetBin(struct mg_connection* conn, const char *query)
{
   dabc::Url url(std::string("getbin?") + (query ? query : ""));

   std::string itemname;
   uint64_t version(0);

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
   } else {
      itemname = url.GetOptionsPart(0);
      if (url.HasOption("version")) {
         int v = url.GetOptionInt("version", 0);
         if (v>0) version = (unsigned) v;
      }
   }

   dabc::Buffer replybuf;

   if (!itemname.empty())
      replybuf = dabc::PublisherRef(GetPublisher()).GetBinary(itemname, version);

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

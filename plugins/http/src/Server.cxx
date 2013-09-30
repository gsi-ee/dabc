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

   fHttpPort = Cfg("port", cmd).AsInt(8080);
   fHttpsPort = Cfg("ports", cmd).AsInt(0);
   fEnabled = Cfg("enabled", cmd).AsBool(false);
   fAuthFile = Cfg("auth_file", cmd).AsStr();
   fAuthDomain = Cfg("auth_domain", cmd).AsStr("dabc@server");
   fSslCertif = Cfg("ssl_certif", cmd).AsStr("");

   if (!fSslCertif.empty() && (fHttpsPort<=0)) fHttpsPort = 443;
   if (fSslCertif.empty()) fHttpsPort = 0;

   if ((fHttpPort<=0) && (fHttpsPort<=0)) fEnabled = false;

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
   DOUT0("HTTPSYS = %s", fHttpSys.c_str());

   std::string sport;

   if (fHttpPort>0) sport = dabc::format("%d",fHttpPort);
   if (fHttpsPort>0) {
      if (!sport.empty()) sport.append(",");
      sport.append(dabc::format("%ds",fHttpsPort));
   }
   //std::string sport = dabc::format("%d", fHttpPort);
   DOUT0("Starting HTTP server on port %s", sport.c_str());

   const char *options[100];
   int op(0);

   options[op++] = "listening_ports";
   options[op++] = sport.c_str();
   options[op++] = "num_threads";
   options[op++] = "5";

   if (!fSslCertif.empty()) {
      options[op++] = "ssl_certificate";
      options[op++] = fSslCertif.c_str(); // "ssl_cert.pem";
   }

   if (!fAuthFile.empty() && !fAuthDomain.empty()) {
      options[op++] = "global_auth_file";
      options[op++] = fAuthFile.c_str();
      options[op++] = "authentication_domain";
      options[op++] = fAuthDomain.c_str();
   }
   options[op++] = 0;

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

   DOUT0("BEGIN_REQ: uri:%s query:%s", request_info->uri, request_info->query_string);

   std::string filename;

   if (IsFileRequested(request_info->uri, filename))
      return ProcessFileRequest(conn, filename);

   std::string content, pathname;

   const char* rslash = strrchr(request_info->uri,'/');
   if (rslash==0) {
      filename = request_info->uri;
   } else {
      pathname.append(request_info->uri, rslash - request_info->uri);
      if (pathname=="/") pathname.clear();
      filename = rslash+1;
   }

   // we return normal file
   if (filename.empty() || (filename == "index.htm")) {
      if (dabc::PublisherRef(GetPublisher()).HasChilds(pathname))
         filename = fHttpSys + "/files/main.htm";
      else
         filename = fHttpSys + "/files/single.htm";

      return ProcessFileRequest(conn, filename);
   }

   bool iserror = false;
   std::string content_type = "text/html";

   if (filename == "h.xml") {
      content_type = "text/xml";

      std::string xmlcode;

      if (dabc::PublisherRef(GetPublisher()).SaveGlobalNamesListAsXml(pathname, xmlcode))
         content = std::string("<?xml version=\"1.0\"?>\n") + xmlcode;
      else
         iserror = true;
   } else
   if (filename == "get.xml") {
      if (ProcessGetItem(conn, pathname, request_info->query_string, content))
         content_type = "text/xml";
      else
         iserror = true;
   } else
   if (filename == "get.bin") {
      if (ProcessGetBin(conn, pathname, request_info->query_string)) return 1;
      iserror = true;
   } else
   if (filename == "image.png") {
      if (ProcessGetPng(conn, pathname, request_info->query_string)) return 1;
      iserror = true;
   } else
   if (filename == "execute") {
      if (ProcessExecute(conn, pathname, request_info->query_string, content))
         content_type = "text/xml";
      else
         iserror = true;
   } else {
      // anything else is not exists
      iserror = true;
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

bool http::Server::IsFileRequested(const char* uri, std::string& res)
{
   if ((uri==0) || (strlen(uri)==0)) return false;

   std::string fname = uri;
   size_t pos = fname.rfind("httpsys/");
   if (pos!=std::string::npos) {
      fname.erase(0, pos+7);
      res = fHttpSys + fname;
      return true;
   }

   if (!fRootSys.empty()) {
      pos = fname.rfind("rootsys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+7);
         res = fRootSys + fname;
         return true;
      }
   }

   if (!fJSRootIOSys.empty()) {
      pos = fname.rfind("jsrootiosys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+11);
         res = fJSRootIOSys + fname;
         return true;
      }
   }

   if (!fGo4Sys.empty()) {
      pos = fname.rfind("go4sys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+6);
         res = fGo4Sys + fname;
         return true;
      }
   }

   return false;
}


int http::Server::ProcessFileRequest(struct mg_connection *conn, const std::string& fname)
{
   DOUT0("SEND FILE: %s", fname.c_str());

   mg_send_file(conn, fname.c_str());

   return 1;


   // TODO: this is code with the file cash, ignore for the moment

//   const struct mg_request_info *request_info = 0;
//   if (conn!=0) request_info = mg_get_request_info((struct mg_connection*)conn);

   void* content = 0;
   int content_len = 0;

   {
      dabc::LockGuard lock(ObjectMutex());

      FilesMap::iterator iter = fFiles.find(fname);
      if (iter!=fFiles.end()) {
         content_len = iter->second.size;
         content = iter->second.ptr;
      }
   }

   if (content == 0) {
      std::ifstream is(fname.c_str());

      char* buf = 0;
      int length = 0;

      if (is) {
         is.seekg (0, is.end);
         length = is.tellg();
         is.seekg (0, is.beg);

         buf = (char*) malloc(length+1);
         is.read(buf, length);
         if (is) {
            buf[length] = 0; // to be able use as c-string
            DOUT2("all characters read successfully from file.");
         } else {
            EOUT("only %d could be read from %s", is.gcount(), fname.c_str());
            free(buf);
            buf = 0; length = 0;
         }
      }

      if (buf!=0) {
         dabc::LockGuard lock(ObjectMutex());

         FileBuf& item = fFiles[fname];
         item.release(); // in any case release buffer

         item.ptr = buf;
         item.size = length;

         content = buf;
         content_len = length;
      }
   }

   if (content==0) {
      mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                      "Content-Length: 0\r\n" // Always set Content-Length
                      "Connection: close\r\n\r\n");
   } else {
      // Send HTTP reply to the client
      mg_printf(conn,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text\r\n"
             "Content-Length: %d\r\n"     // Always set Content-Length
             "\r\n", content_len);

      mg_write(conn, content, (size_t) content_len);
   }

   return 1;
}



bool http::Server::ProcessGetItem(struct mg_connection* conn, const std::string& itemname, const char *query, std::string& replybuf)
{
   if (itemname.empty()) {
      EOUT("Item is not specified in get.xml request");
      return false;
   }

   std::string surl = "getitem";
   if (query!=0) { surl.append("?"); surl.append(query); }

   dabc::Url url(surl);
   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
      return false;
   }

   unsigned hlimit(0);
   uint64_t version(0);

   if (url.HasOption("history")) {
      int hist = url.GetOptionInt("history", 0);
      if (hist>0) hlimit = (unsigned) hist;
   }
   if (url.HasOption("version")) {
      int v = url.GetOptionInt("version", 0);
      if (v>0) version = (unsigned) v;
   }

   // DOUT0("HLIMIT = %u query = %s", hlimit, query);

   dabc::Hierarchy res = dabc::PublisherRef(GetPublisher()).Get(itemname, version, hlimit);

   if (res.null()) return false;
      // result is only item fields, we need to decorate it with some more attributes

   replybuf = dabc::format("<Reply xmlns:dabc=\"http://dabc.gsi.de/xhtml\" itemname=\"%s\" %s=\"%lu\">\n",itemname.c_str(), dabc::prop_version, (long unsigned) res.GetVersion());
   replybuf += res.SaveToXml(hlimit > 0 ? dabc::xmlmask_History : 0);
   replybuf += "</Reply>";
   return true;
}


bool http::Server::ProcessExecute(struct mg_connection* conn, const std::string& itemname, const char *query, std::string& replybuf)
{
   if (itemname.empty()) {
      EOUT("Item is not specified in execute request");
      return false;
   }

   if (query==0) query = "";
   dabc::Command res = dabc::PublisherRef(GetPublisher()).ExeCmd(itemname, query);

   replybuf = dabc::format("<Reply xmlns:dabc=\"http://dabc.gsi.de/xhtml\" itemname=\"%s\">\n", itemname.c_str());

   if (!res.null())
      replybuf += res.SaveToXml();

   replybuf += "\n</Reply>";

   return true;
}

bool http::Server::ProcessGetBin(struct mg_connection* conn, const std::string& itemname, const char *query)
{
   if (itemname.empty()) return false;

   dabc::Url url(std::string("getbin?") + (query ? query : ""));

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
      return false;
   }

   uint64_t version(0);
   if (url.HasOption("version")) {
      int v = url.GetOptionInt("version", 0);
      if (v>0) version = (unsigned) v;
   }

   dabc::Buffer replybuf = dabc::PublisherRef(GetPublisher()).GetBinary(itemname, version);

   if (replybuf.null()) return false;

   mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/x-binary\r\n"
            "Content-Length: %u\r\n"
            "Connection: keep-alive\r\n"
            "\r\n", (unsigned) replybuf.GetTotalSize());
   mg_write(conn, replybuf.SegmentPtr(), (size_t) replybuf.GetTotalSize());

   return true;
}

bool http::Server::ProcessGetPng(struct mg_connection* conn, const std::string& itemname, const char *query)
{
   if (itemname.empty()) return false;

   dabc::Url url(std::string("getbin?") + (query ? query : ""));

   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query);
      return false;
   }

   uint64_t version(0);
   if (url.HasOption("version")) {
      int v = url.GetOptionInt("version", 0);
      if (v>0) version = (unsigned) v;
   }

   dabc::Buffer replybuf = dabc::PublisherRef(GetPublisher()).GetBinary(itemname, version);

   if (replybuf.null()) return false;

   unsigned image_size = replybuf.GetTotalSize() - sizeof(dabc::BinDataHeader);

   mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: image/png\r\n"
              "Content-Length: %u\r\n"
              "Connection: keep-alive\r\n"
               "\r\n", image_size);
   mg_write(conn, ((char*) replybuf.SegmentPtr()) + sizeof(dabc::BinDataHeader), (size_t) image_size);

   return true;
}


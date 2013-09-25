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

   DOUT0("open_file_handler  serv = %p", serv);

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
   fAuthFile = Cfg("auth_file", cmd).AsStr();
   fAuthDomain = Cfg("auth_domain", cmd).AsStr("dabc@server");

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

   const char *options[100];
   int op(0);

   options[op++] = "listening_ports";
   options[op++] = sport.c_str();
   if (!fAuthFile.empty() && !fAuthDomain.empty()) {
      options[op++] = "global_auth_file";
      options[op++] = "dabc_authentification";
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

   // files will be proceeded by open_file requests
   if (IsFileName(request_info->uri)) return 0;

   const char* rslash = strrchr(request_info->uri,'/');
   std::string pathname, filename;
   if (rslash==0) {
      filename = request_info->uri;
   } else {
      pathname.append(request_info->uri, rslash - request_info->uri);
      if (pathname=="/") pathname.clear();
      filename = rslash+1;
   }

   DOUT0("BEGIN_REQ: path:%s file:%s", pathname.c_str(), filename.c_str());


   std::string content;
   std::string content_type = "text/html";
   bool iserror = false;

   if (filename.empty() || (filename == "index.htm")) {
      const char* res = 0;

      if (dabc::PublisherRef(GetPublisher()).HasChilds(pathname))
         res = open_file(0, "httpsys/files/main.htm");
      else
         res = open_file(0, "httpsys/files/single.htm");
      if (res) content = res;
          else iserror = true;
   } else
   if ((strcmp(request_info->uri,"/")==0) ||
       (strcmp(request_info->uri,"/main.htm")==0) || (strcmp(request_info->uri,"/main.html")==0) ||
       (strcmp(request_info->uri,"/index.htm")==0) || (strcmp(request_info->uri,"/main.html")==0)) {
      content = open_file(0, "httpsys/files/main.htm");
   } else
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


bool http::Server::IsFileName(const char* path)
{
   // we only allow to load files from predefined directories
   if ((path==0) || (strlen(path)==0)) return false;

   if (strstr(path,"httpsys/")!=0) return true;
   if (strstr(path,"jsrootiosys/")!=0) return true;
   if (strstr(path,"rootsys/")!=0) return true;
   if (strstr(path,"go4sys/")!=0) return true;
   return false;
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
   if (path==0) return 0;

   DOUT0("OPEN_FILE: %s", path);

   std::string fname(path);

   if (fname == "dabc_authentification") {

      fname = fAuthFile;

      // static const char* mypass = "linev:mydomain.com:$apr1$9WFy7NNd$PyUs4OgY8M2okLsfUBG9l0\n";

      // use htdigest
      // static const char* mypass = "linev:mydomain.com:9ddef69584c26bb469738bef2f565d10\n";
      //if (data_len) *data_len = strlen(mypass);

      //return mypass;
   } else {
      if (!IsFileName(path) || !MakeRealFileName(fname)) return 0;
   }

//   const struct mg_request_info *request_info = 0;
//   if (conn!=0) request_info = mg_get_request_info((struct mg_connection*)conn);

   bool force = true;

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

   // if file cannot be open, return empty
   if (!is) return 0;

   is.seekg (0, is.end);
   int length = is.tellg();
   is.seekg (0, is.beg);

   char* buf = (char*) malloc(length+1);
   is.read(buf, length);
   if (is)
      DOUT2("all characters read successfully from file.");
   else
      EOUT("only %d could be read from %s", is.gcount(), path);

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


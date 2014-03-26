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

//#include "dabc/threads.h"

#include "dabc/Hierarchy.h"
#include "dabc/Manager.h"
#include "dabc/Url.h"
#include "dabc/Publisher.h"


const char* http::Server::GetMimeType(const char* path)
{
   static const struct {
     const char *extension;
     int ext_len;
     const char *mime_type;
   } builtin_mime_types[] = {
     {".xml", 4, "text/xml"},
     {".json", 5, "application/json"},
     {".bin", 4, "application/x-binary"},
     {".gif", 4, "image/gif"},
     {".jpg", 4, "image/jpeg"},
     {".png", 4, "image/png"},
     {".html", 5, "text/html"},
     {".htm", 4, "text/html"},
     {".shtm", 5, "text/html"},
     {".shtml", 6, "text/html"},
     {".css", 4, "text/css"},
     {".js",  3, "application/x-javascript"},
     {".ico", 4, "image/x-icon"},
     {".jpeg", 5, "image/jpeg"},
     {".svg", 4, "image/svg+xml"},
     {".txt", 4, "text/plain"},
     {".torrent", 8, "application/x-bittorrent"},
     {".wav", 4, "audio/x-wav"},
     {".mp3", 4, "audio/x-mp3"},
     {".mid", 4, "audio/mid"},
     {".m3u", 4, "audio/x-mpegurl"},
     {".ogg", 4, "application/ogg"},
     {".ram", 4, "audio/x-pn-realaudio"},
     {".xslt", 5, "application/xml"},
     {".xsl", 4, "application/xml"},
     {".ra",  3, "audio/x-pn-realaudio"},
     {".doc", 4, "application/msword"},
     {".exe", 4, "application/octet-stream"},
     {".zip", 4, "application/x-zip-compressed"},
     {".xls", 4, "application/excel"},
     {".tgz", 4, "application/x-tar-gz"},
     {".tar", 4, "application/x-tar"},
     {".gz",  3, "application/x-gunzip"},
     {".arj", 4, "application/x-arj-compressed"},
     {".rar", 4, "application/x-arj-compressed"},
     {".rtf", 4, "application/rtf"},
     {".pdf", 4, "application/pdf"},
     {".swf", 4, "application/x-shockwave-flash"},
     {".mpg", 4, "video/mpeg"},
     {".webm", 5, "video/webm"},
     {".mpeg", 5, "video/mpeg"},
     {".mov", 4, "video/quicktime"},
     {".mp4", 4, "video/mp4"},
     {".m4v", 4, "video/x-m4v"},
     {".asf", 4, "video/x-ms-asf"},
     {".avi", 4, "video/x-msvideo"},
     {".bmp", 4, "image/bmp"},
     {".ttf", 4, "application/x-font-ttf"},
     {NULL,  0, NULL}
   };

   int path_len = strlen(path);

   for (int i = 0; builtin_mime_types[i].extension != NULL; i++) {
      if (path_len <= builtin_mime_types[i].ext_len) continue;
      const char* ext = path + (path_len - builtin_mime_types[i].ext_len);
      if (strcmp(ext, builtin_mime_types[i].extension) == 0) {
        return builtin_mime_types[i].mime_type;
     }
  }

  return "text/plain";
}


http::Server::Server(const std::string& name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fHttpSys(),
   fGo4Sys(),
   fRootSys(),
   fJSRootIOSys()
{
   fHttpSys = ".";

   const char* dabcsys = getenv("DABCSYS");
   if (dabcsys!=0) fHttpSys = dabc::format("%s/plugins/http", dabcsys);

   const char* go4sys = getenv("GO4SYS");
   if (go4sys!=0) fGo4Sys = go4sys;

   const char* rootsys = getenv("ROOTSYS");
   if (rootsys!=0) fRootSys = rootsys;

   const char* jsrootiosys = getenv("JSROOTIOSYS");
   if (jsrootiosys!=0) fJSRootIOSys = jsrootiosys;
   if (fJSRootIOSys.empty()) fJSRootIOSys = fHttpSys + "/JSRootIO";

   DOUT1("JSROOTIOSYS = %s ", fJSRootIOSys.c_str());
   DOUT1("HTTPSYS = %s", fHttpSys.c_str());
}

http::Server::~Server()
{
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


bool http::Server::ProcessExecute(const std::string& itemname, const std::string& query, std::string& replybuf)
{
   if (itemname.empty()) {
      EOUT("Item is not specified in execute request");
      return false;
   }

   dabc::Command res = dabc::PublisherRef(GetPublisher()).ExeCmd(itemname, query);

   replybuf = dabc::format("<Reply xmlns:dabc=\"http://dabc.gsi.de/xhtml\" itemname=\"%s\">\n", itemname.c_str());

   if (!res.null())
      replybuf += res.SaveToXml();

   replybuf += "\n</Reply>";

   return true;
}


bool http::Server::Process(const std::string& path, const std::string& file, const std::string& query,
                           std::string& content_type, std::string& content_str, dabc::Buffer& content_bin)
{
   // we return normal file
   if (file.empty() || (file == "index.htm")) {
      if (dabc::PublisherRef(GetPublisher()).HasChilds(path) != 0)
         content_str = fHttpSys + "/files/main.htm";
      else
         content_str = fHttpSys + "/files/single.htm";

      content_type = "__file__";
      return true;
   }

   if (file == "h.xml") {
      content_type = "text/xml";

      std::string xmlcode;

      if (dabc::PublisherRef(GetPublisher()).SaveGlobalNamesListAsXml(path, xmlcode)) {
         content_str = std::string("<?xml version=\"1.0\"?>\n") + xmlcode;
         return true;
      }
      return false;
   } else

   if (file == "execute") {
      content_type = "text/xml";
      return ProcessExecute(path, query, content_str);
   } else

   if (!file.empty()) {

      dabc::CmdGetBinary cmd(path, file, query);
      cmd.SetTimeout(5.);

      dabc::WorkerRef ref = GetPublisher();

      if (ref.Execute(cmd) == dabc::cmd_true) {
         content_type = cmd.GetStr("content_type");

         if (content_type.empty())
            content_type = GetMimeType(file.c_str());

         content_bin = cmd.GetRawData();
         return !content_bin.null();
      }

      return false;

   }
/*
   if (file == "get.xml") {
      content_type = "text/xml";
      content_bin = dabc::PublisherRef(GetPublisher()).GetBinary(path, "xml", query);
      return !content_bin.null();
   } else

   if (file == "get.json") {
      content_type = "application/json";
      content_bin = dabc::PublisherRef(GetPublisher()).GetBinary(path, "json", query);
      return !content_bin.null();
   } else

   if (file == "get.bin") {
      content_type = "application/x-binary";
      content_bin = dabc::PublisherRef(GetPublisher()).GetBinary(path, "bin", query);
      return !content_bin.null();
   } else

   if (file == "get.png") {
      content_type = "image/png";
      content_bin = dabc::PublisherRef(GetPublisher()).GetBinary(path, "png", query);
      return !content_bin.null();
   }
*/

   return false;
}

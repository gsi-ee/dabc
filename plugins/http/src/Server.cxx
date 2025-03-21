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

#include <cstring>
#include <cstdlib>

#include "dabc/Configuration.h"
#include "dabc/Url.h"
#include "dabc/Publisher.h"

#ifndef DABC_WITHOUT_ZLIB
#include "zlib.h"
#endif

const char *http::Server::GetMimeType(const char *path)
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
     {".js", 3, "application/x-javascript"},
     {".mjs", 4, "text/javascript"},
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
     {nullptr, 0, nullptr}
   };

   int path_len = strlen(path);

   for (int i = 0; builtin_mime_types[i].extension != nullptr; i++) {
      if (path_len <= builtin_mime_types[i].ext_len) continue;
      const char *ext = path + (path_len - builtin_mime_types[i].ext_len);
      if (strcmp(ext, builtin_mime_types[i].extension) == 0) {
         return builtin_mime_types[i].mime_type;
      }
   }

   return "text/plain";
}


http::Server::Server(const std::string &server_name, dabc::Command cmd) :
   dabc::Worker(MakePair(server_name)),
   fLocations(),
   fHttpSys(),
   fJsRootSys(),
   fDefaultAuth(-1)
{
   fHttpSys = ".";

   const char *dabcsys = std::getenv("DABCSYS");
   if (dabcsys) {
      AddLocation(dabcsys, "dabcsys/");
      AddLocation(dabcsys, "${DABCSYS}/");
   }

   std::string plugins_dir = dabc::Configuration::GetPluginsDir();
   if (!plugins_dir.empty()) {
      AddLocation(plugins_dir, "dabc_plugins/");
      AddLocation(plugins_dir + "/http", "httpsys/", "dabc_", "/files/");
      fOwnJsRootSys = fJsRootSys = plugins_dir + "/http/js";
      fHttpSys = plugins_dir + "/http";
   }

   std::string urlopt = cmd.GetStr("urlopt");
   if (!urlopt.empty()) {
      dabc::Url url;
      url.SetOptions(urlopt);
      unsigned cnt = 0;

      while (cnt < 100) {
         std::string name = dabc::format("loc%u", cnt++);
         if (!url.HasOption(name+"d") || !url.HasOption(name+"a")) break;
         AddLocation(url.GetOptionStr(name+"d"), url.GetOptionStr(name+"a"), url.GetOptionStr(name+"n"), url.GetOptionStr(name+"s"));
      }
   }

   const char *jsrootsys = std::getenv("JSROOTSYS");
   if (jsrootsys) fJsRootSys = jsrootsys;

   if (!fJsRootSys.empty()) {
      AddLocation(fJsRootSys, "jsrootsys/", "root_", "/files/");
      DOUT1("JSROOTSYS = %s ", fJsRootSys.c_str());
      if (fOwnJsRootSys.empty())
         fOwnJsRootSys = fJsRootSys;
   }

   DOUT1("HTTPSYS = %s", fHttpSys.c_str());

   fAutoLoad = Cfg("AutoLoad", cmd).AsStr(""); // AsStr("httpsys/scripts/dabc.js;httpsys/scripts/gauge.js");
   fTopTitle = Cfg("TopTitle", cmd).AsStr("DABC online server");
   fBrowser = Cfg("Browser", cmd).AsStr("");
   fLayout = Cfg("Layout", cmd).AsStr("");
   fDrawItem = Cfg("DrawItem", cmd).AsStr("");
   fDrawOpt = Cfg("DrawOpt", cmd).AsStr("");
   fMonitoring = Cfg("Monitoring", cmd).AsInt(0);
}

http::Server::~Server()
{
}

void http::Server::AddLocation(const std::string &filepath,
                               const std::string &absprefix,
                               const std::string &nameprefix,
                               const std::string &nameprefixrepl)
{
   fLocations.emplace_back(Location());
   fLocations.back().fFilePath = filepath;
   fLocations.back().fAbsPrefix = absprefix;
   fLocations.back().fNamePrefix = nameprefix;
   fLocations.back().fNamePrefixRepl = nameprefixrepl;
}


bool http::Server::VerifyFilePath(const char *fname)
{
   if (!fname || (*fname == 0)) return false;

   int level = 0;

   while (*fname != 0) {

      // find next slash or backslash
      const char *next = strpbrk(fname, "/\\");
      if (!next) return true;

      // most important - change to parent dir
      if ((next == fname + 2) && (*fname == '.') && (*(fname+1) == '.')) {
         fname += 3; level--;
         if (level<0) return false;
         continue;
      }

      // ignore current directory
      if ((next == fname + 1) && (*fname == '.'))  {
         fname += 2;
         continue;
      }

      // ignore slash at the front
      if (next == fname) {
         fname ++;
         continue;
      }

      fname = next+1;
      level++;
   }

   return true;
}

bool http::Server::IsFileRequested(const char *uri, std::string& res)
{
   if (!uri || (*uri == 0)) return false;

   std::string fname = uri;

   for (unsigned n=0;n<fLocations.size();n++) {
      size_t pos = fname.rfind(fLocations[n].fAbsPrefix);
      if (pos!=std::string::npos) {
         fname.erase(0, pos + fLocations[n].fAbsPrefix.length() - 1);
         if (!VerifyFilePath(fname.c_str())) return false;
         res = fLocations[n].fFilePath + fname;
         return true;
      }
   }

   return false;
}

void http::Server::ExtractPathAndFile(const char *uri, std::string& pathname, std::string& filename)
{
   pathname.clear();
   const char *rslash = strrchr(uri,'/');
   if (!rslash) {
      filename = uri;
   } else {
      pathname.append(uri, rslash - uri);
      if (pathname == "/") pathname.clear();
      filename = rslash+1;
   }
}


bool http::Server::IsAuthRequired(const char *uri)
{
   if (fDefaultAuth < 0) return false;

   std::string pathname, fname;

   if (IsFileRequested(uri, fname))
      return fDefaultAuth > 0;

   ExtractPathAndFile(uri, pathname, fname);

   int res = dabc::PublisherRef(GetPublisher()).NeedAuth(pathname);

   // DOUT0("Request AUTH for path %s res = %d", pathname.c_str(), res);

   if (res<0) res = fDefaultAuth;

   return res > 0;
}

bool http::Server::Process(const char *uri, const char *_query,
                           std::string& content_type,
                           std::string& content_header,
                           std::string& content_str,
                           dabc::Buffer& content_bin)
{

   std::string pathname, filename, query;

   content_header.clear();

   //ExtractPathAndFile(uri, pathname, filename);

   content_type = dabc::PublisherRef(GetPublisher()).UserInterfaceKind(uri, pathname, filename);

   DOUT2("http::Server::Process uri %s path %s file %s type %s", uri ? uri : "---", pathname.c_str(), filename.c_str(), content_type.c_str());

   if (content_type == "__error__") return false;

   if (content_type == "__user__") {
      content_str = pathname;
      if (filename.empty()) content_str += "main.htm";
                       else content_str += filename;
      content_type = "__file__";
      IsFileRequested(content_str.c_str(), content_str);
      return true;
   }

   if (filename == "draw.htm") {
      content_str = fOwnJsRootSys + "/files/draw.htm";
      content_type = "__file__";
      return true;
   }

   // check that filename starts with some special prefix, in such case redirect it to other location

   if (filename.empty() || (filename=="main.htm") || (filename=="index.htm")) {
      content_str = fOwnJsRootSys + "/files/online.htm";
      content_type = "__file__";
      return true;
   }

   for (unsigned n=0;n<fLocations.size();n++)
      if (!fLocations[n].fNamePrefix.empty() && (filename.find(fLocations[n].fNamePrefix) == 0)) {
         size_t len = fLocations[n].fNamePrefix.length();
         if ((filename.length()<=len) || (filename[len]=='.')) continue;
         filename.erase(0, len);
         if (!VerifyFilePath(filename.c_str())) return false;
         content_str = fLocations[n].fFilePath + fLocations[n].fNamePrefixRepl + filename;
         content_type = "__file__";
         return true;
      }

   if (_query) query = _query;

   if (filename == "execute") {
      if (pathname.empty()) return false;

      dabc::Command res = dabc::PublisherRef(GetPublisher()).ExeCmd(pathname, query);

      if (res.GetResult() <= 0) return false;

      content_type = "application/json";
      content_str = res.SaveToJson();

      return true;
   }

   if (filename.empty()) return false;

   bool iszipped = false;

   if ((filename.length() > 3) && (filename.rfind(".gz") == filename.length()-3)) {
      filename.resize(filename.length()-3);
      iszipped = true;
   }

   if ((filename == "h.xml") || (filename == "h.json")) {

      bool isxml = (filename == "h.xml");

      dabc::CmdGetNamesList cmd(isxml ? "xml" : "json", pathname, query);

      if (!fAutoLoad.empty()) cmd.AddHeader("_autoload", fAutoLoad);
      if (!fTopTitle.empty()) cmd.AddHeader("_toptitle", fTopTitle);
      if (!fBrowser.empty()) cmd.AddHeader("_browser", fBrowser);
      if (!fLayout.empty()) cmd.AddHeader("_layout", fLayout);
      if (!fDrawItem.empty() && (pathname=="/")) {
         cmd.AddHeader("_drawitem", fDrawItem);
         cmd.AddHeader("_drawopt", fDrawOpt);
      }
      if (fMonitoring >= 100)
         cmd.AddHeader("_monitoring", std::to_string(fMonitoring), false);

      dabc::WorkerRef wrk = GetPublisher();

      if (wrk.Execute(cmd) != dabc::cmd_true) return false;

      content_str = cmd.GetStr("astext");

      if (isxml) {
         content_type = "text/xml";
         content_str = std::string("<?xml version=\"1.0\"?>\n<dabc>\n") + content_str + "</dabc>";
      } else {
         content_type = "application/json";
      }

   } else if (filename == "multiget.json") {

      content_type = "application/json";

      std::string opt = query;
      dabc::Url::ReplaceSpecialSymbols(opt);

      if (opt.find("items=") == 0) {
         std::size_t separ = opt.find("&");
         if (separ == std::string::npos) separ = opt.length();
         std::string items = opt.substr(6, separ-6);
         opt = opt.erase(0, separ+1);
         DOUT3("MULTIGET path %s items=%s rest:%s", pathname.c_str(), items.c_str(), opt.c_str());

         dabc::RecordField field(items);
         std::vector<std::string> arr = field.AsStrVect();

         content_str="[";
         dabc::WorkerRef ref = GetPublisher();

         for (unsigned n=0;n<arr.size();n++) {
            if (n>0) content_str.append(",");

            dabc::CmdGetBinary cmd(pathname+arr[n], "get.json", opt);
            cmd.SetTimeout(5.);

            if (ref.Execute(cmd) == dabc::cmd_true) {
               content_bin = cmd.GetRawData();
            }
            content_str.append(dabc::format("{ \"item\": \"%s\", \"result\":", arr[n].c_str()));

            if (content_bin.null())
               content_str.append("null");
            else
               content_str.append((const char*) content_bin.SegmentPtr(), content_bin.SegmentSize());

            content_str.append("}");

            content_bin.Release();
         }

         content_str.append("]");

      } else {
         content_str = "null";
      }

   } else {

      dabc::CmdGetBinary cmd(pathname, filename, query);
      cmd.SetTimeout(5.);

      dabc::WorkerRef ref = GetPublisher();

      if (ref.Execute(cmd) == dabc::cmd_true) {
         content_type = cmd.GetStr("content_type");

         if (content_type.empty())
            content_type = GetMimeType(filename.c_str());

         if (cmd.HasField("MVersion"))
            content_header.append(dabc::format("MVersion: %u\r\n", (unsigned) cmd.GetUInt("MVersion")));

         if (cmd.HasField("BVersion"))
            content_header.append(dabc::format("BVersion: %u\r\n", (unsigned) cmd.GetUInt("BVersion")));

         content_bin = cmd.GetRawData();
         if (content_bin.null()) content_str = cmd.GetStr("StringReply");
      }

      // TODO: in some cases empty binary may be not an error
      if (content_bin.null() && (content_str.length() == 0)) {
         DOUT2("Is empty buffer is error for uri %s ?", uri);
         return false;
      }
   }

   if (iszipped) {
#ifdef DABC_WITHOUT_ZLIB
      DOUT0("It is requested to compress buffer, but ZLIB is not available!!!");
#else

      unsigned long objlen = 0;
      Bytef* objptr = nullptr;

      if (!content_bin.null()) {
         objlen = content_bin.GetTotalSize();
         objptr = (Bytef*) content_bin.SegmentPtr();
      } else {
         objlen = content_str.length();
         objptr = (Bytef*) content_str.c_str();
      }

      unsigned long objcrc = crc32(0, nullptr, 0);
      objcrc = crc32(objcrc, objptr, objlen);

      // reserve place for header plus space required for the target buffer
      unsigned long zipbuflen = 18 + compressBound(objlen);
      if (zipbuflen < 512) zipbuflen = 512;
      void* zipbuf = std::malloc(zipbuflen);

      if (!zipbuf) {
         EOUT("Fail to allocate %lu bytes memory !!!", zipbuflen);
         return true;
      }

      char *bufcur = (char*) zipbuf;

      *bufcur++ = 0x1f;  // first byte of ZIP identifier
      *bufcur++ = 0x8b;  // second byte of ZIP identifier
      *bufcur++ = 0x08;  // compression method
      *bufcur++ = 0x00;  // FLAG - empty, no any file names
      *bufcur++ = 0;    // empty timestamp
      *bufcur++ = 0;    //
      *bufcur++ = 0;    //
      *bufcur++ = 0;    //
      *bufcur++ = 0;    // XFL (eXtra FLags)
      *bufcur++ = 3;    // OS   3 means Unix

      zipbuflen-=18; // ZIP cannot use header and footer

      // WORKAROUD - seems to be, compress places 2 bytes before and 4 bytes after the compressed buffer

      // int res = compress((Bytef*)bufcur, &zipbuflen, objptr, objlen);
      // bufcur += zipbuflen;

      int res = compress((Bytef*)bufcur-2, &zipbuflen, objptr, objlen);
      if (res!=Z_OK) {
         EOUT("Fail to compress buffer with ZLIB");
         std::free(zipbuf);
         return true;
      }

      *(bufcur-2) = 0;    // XFL (eXtra FLags)
      *(bufcur-1) = 3;    // OS   3 means Unix

      bufcur += (zipbuflen - 6);


      *bufcur++ = objcrc & 0xff;    // CRC32
      *bufcur++ = (objcrc >> 8) & 0xff;
      *bufcur++ = (objcrc >> 16) & 0xff;
      *bufcur++ = (objcrc >> 24) & 0xff;

      *bufcur++ = objlen & 0xff;          // original data length
      *bufcur++ = (objlen >> 8) & 0xff;
      *bufcur++ = (objlen >> 16) & 0xff;
      *bufcur++ = (objlen >> 24) & 0xff;

      content_bin = dabc::Buffer::CreateBuffer(zipbuf, bufcur - (char*) zipbuf, true);
      content_str.clear();

      content_header.append("Content-Encoding: gzip\r\n");

      DOUT3("Compress original object %lu into zip buffer %lu", objlen, zipbuflen);
#endif
   }

   // exclude caching of dynamic data
   content_header.append("Cache-Control: private, no-cache, no-store, must-revalidate, max-age=0, proxy-revalidate, s-maxage=0\r\n");

   return true;
}

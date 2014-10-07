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

#include <string.h>

#include "dabc/Hierarchy.h"
#include "dabc/Manager.h"
#include "dabc/Url.h"
#include "dabc/Publisher.h"

#ifndef DABC_WITHOUT_ZLIB
#include "zlib.h"
#endif



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
   fDabcSys(),
   fHttpSys(),
   fGo4Sys(),
   fJsRootSys(),
   fDefaultAuth(-1)
{
   fHttpSys = ".";
   fDabcSys = ".";

   const char* dabcsys = getenv("DABCSYS");
   if (dabcsys!=0) {
      fDabcSys = dabcsys;
      fHttpSys = dabc::format("%s/plugins/http", dabcsys);
      fJsRootSys = dabc::format("%s/plugins/root/js", dabcsys);
   }

   const char* go4sys = getenv("GO4SYS");
   if (go4sys!=0) fGo4Sys = go4sys;

   const char* jsrootsys = getenv("JSROOTSYS");
   if (jsrootsys!=0) fJsRootSys = jsrootsys;

   DOUT1("JSROOTSYS = %s ", fJsRootSys.c_str());
   DOUT1("HTTPSYS = %s", fHttpSys.c_str());
}

http::Server::~Server()
{
}

bool http::Server::VerifyFilePath(const char* fname)
{
   if ((fname==0) || (*fname==0)) return false;

   int level = 0;

   while (*fname != 0) {

      // find next slash or backslash
      const char* next = strpbrk(fname, "/\\");
      if (next==0) return true;

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
      if (next==fname) {
         fname ++;
         continue;
      }

      fname = next+1;
      level++;
   }

   return true;
}

bool http::Server::IsFileRequested(const char* uri, std::string& res)
{
   if ((uri==0) || (strlen(uri)==0)) return false;

   std::string fname = uri;
   size_t pos = fname.rfind("httpsys/");
   if (pos!=std::string::npos) {
      fname.erase(0, pos+7);
      if (!VerifyFilePath(fname.c_str())) return false;
      res = fHttpSys + fname;
      return true;
   }

   if (!fDabcSys.empty()) {
      // DABCSYS can be only in the beginning
      if (fname.find("${DABCSYS}")==0) {
         fname.erase(0, 10);
         if (!VerifyFilePath(fname.c_str())) return false;
         res = fDabcSys + fname;
         return true;
      }
      if (fname.find("dabcsys")==0) {
         fname.erase(0, 7);
         if (!VerifyFilePath(fname.c_str())) return false;
         res = fDabcSys + fname;
         return true;
      }
   }

   if (!fJsRootSys.empty()) {
      pos = fname.rfind("jsrootsys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+9);
         if (!VerifyFilePath(fname.c_str())) return false;
         res = fJsRootSys + fname;
         return true;
      }
   }

   if (!fGo4Sys.empty()) {
      pos = fname.rfind("go4sys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+6);
         if (!VerifyFilePath(fname.c_str())) return false;
         res = fGo4Sys + fname;
         return true;
      }
   }

   return false;
}

void http::Server::ExtractPathAndFile(const char* uri, std::string& pathname, std::string& filename)
{
   pathname.clear();
   const char* rslash = strrchr(uri,'/');
   if (rslash == 0) {
      filename = uri;
   } else {
      pathname.append(uri, rslash - uri);
      if (pathname == "/") pathname.clear();
      filename = rslash+1;
   }
}


bool http::Server::IsAuthRequired(const char* uri)
{
   if (fDefaultAuth<0) return false;

   std::string pathname, fname;

   if (IsFileRequested(uri, fname)) return fDefaultAuth > 0;

   ExtractPathAndFile(uri, pathname, fname);

   int res = dabc::PublisherRef(GetPublisher()).NeedAuth(pathname);

   // DOUT0("Request AUTH for path %s res = %d", pathname.c_str(), res);

   if (res<0) res = fDefaultAuth;

   return res > 0;
}

bool http::Server::Process(const char* uri, const char* _query,
                           std::string& content_type,
                           std::string& content_header,
                           std::string& content_str,
                           dabc::Buffer& content_bin)
{

   std::string pathname, filename, query;

   content_header.clear();

   //ExtractPathAndFile(uri, pathname, filename);

   //DOUT0("URI = %s path %s file %s", uri ? uri : "---", pathname.c_str(), filename.c_str());

   content_type = dabc::PublisherRef(GetPublisher()).UserInterfaceKind(uri, pathname, filename);

   // DOUT0("URI = %s path %s file %s type %s", uri ? uri : "---", pathname.c_str(), filename.c_str(), content_type.c_str());

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
      content_str = fHttpSys + "/files/draw.htm";
      content_type = "__file__";
      return true;
   }

   if ((filename == "rootdraw.htm") && !fJsRootSys.empty()) {
      content_str = fJsRootSys + "/files/draw.htm";
      content_type = "__file__";
      return true;
   }

   if (filename.empty() || (filename=="main.htm")) {
      content_str = fHttpSys + "/files/main.htm";
      content_type = "__file__";
      return true;
   }

   if (_query!=0) query = _query;

   // DOUT0("Process %s filename '%s'", pathname.c_str(), filename.c_str());

   // if no file specified, we tried to detect that is requested
/*   if (filename.empty()) {

      content_str = dabc::PublisherRef(GetPublisher()).UserInterfaceKind(pathname);

      DOUT0("UI kind '%s'", content_str.c_str());

      if (content_str.empty()) return false;

      if (content_str == "__tree__") content_str = fHttpSys + "/files/main.htm"; else
      if (content_str == "__single__") content_str = fHttpSys + "/files/single.htm"; else {
         // here we only allow to check filename for prefixes like DABCSYS
         IsFileRequested(content_str.c_str(), content_str);
      }

      content_type = "__file__";
      return true;
   }
*/

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

   if (filename == "h.xml") {
      content_type = "text/xml";

      std::string xmlcode;

      if (!dabc::PublisherRef(GetPublisher()).SaveGlobalNamesListAs("xml", pathname, query, xmlcode)) return false;

      content_str = std::string("<?xml version=\"1.0\"?>\n") + xmlcode;

   } else
   if (filename == "h.json") {

      content_type = "application/json";

      DOUT3("Get global names list path %s", pathname.c_str());

      if (!dabc::PublisherRef(GetPublisher()).SaveGlobalNamesListAs("json", pathname, query, content_str)) return false;

   } else {

      dabc::CmdGetBinary cmd(pathname, filename, query);
      cmd.SetTimeout(5.);

      dabc::WorkerRef ref = GetPublisher();

      if (ref.Execute(cmd) == dabc::cmd_true) {
         content_type = cmd.GetStr("content_type");

         if (content_type.empty())
            content_type = GetMimeType(filename.c_str());

         if (cmd.HasField("MVersion"))
            content_header.append(dabc::format("MVersion: %u\r\n", cmd.GetUInt("MVersion")));

         if (cmd.HasField("BVersion"))
            content_header.append(dabc::format("BVersion: %u\r\n", cmd.GetUInt("BVersion")));

         content_bin = cmd.GetRawData();
      }

      // TODO: in some cases empty binary may be not an error
      if (content_bin.null()) { DOUT0("Is empty buffer is error for uri %s ?", uri); return false; }
   }

   if (iszipped) {
#ifdef DABC_WITHOUT_ZLIB
      DOUT0("It is requested to compress buffer, but ZLIB is not available!!!");
#else

      unsigned long objlen = 0;
      Bytef* objptr = 0;

      if (!content_bin.null()) {
         objlen = content_bin.GetTotalSize();
         objptr = (Bytef*) content_bin.SegmentPtr();
      } else {
         objlen = content_str.length();
         objptr = (Bytef*) content_str.c_str();
      }

      unsigned long objcrc = crc32(0, NULL, 0);
      objcrc = crc32(objcrc, objptr, objlen);

      // reserve place for header plus space required for the target buffer
      unsigned long zipbuflen = 18 + compressBound(objlen);
      if (zipbuflen<512) zipbuflen = 512;
      void* zipbuf = malloc(zipbuflen);

      if (zipbuf==0) {
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
         free(zipbuf);
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

   return true;
}

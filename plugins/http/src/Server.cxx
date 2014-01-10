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

http::Server::Server(const std::string& name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fHttpSys(),
   fGo4Sys(),
   fRootSys(),
   fJSRootIOSys()
{
   fEnabled = Cfg("enabled", cmd).AsBool(true);

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


bool http::Server::ProcessGetItem(const std::string& itemname, const std::string& query, std::string& replybuf)
{
   if (itemname.empty()) {
      EOUT("Item is not specified in get.xml request");
      return false;
   }

   std::string surl = "getitem";
   if (query.length()>0) { surl.append("?"); surl.append(query); }

   dabc::Url url(surl);
   if (!url.IsValid()) {
      EOUT("Cannot decode query url %s", query.c_str());
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

   if (file == "get.xml") {
      content_type = "text/xml";
      return ProcessGetItem(path, query, content_str);
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
   } else

   if (file == "execute") {
      content_type = "text/xml";
      return ProcessExecute(path, query, content_str);
   }

   return false;
}



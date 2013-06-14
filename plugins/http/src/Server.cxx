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
   dabc::LockGuard lock(fHierarchyMutex);

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

   std::string content;
   std::string content_type = "text/html";

   if ((strcmp(request_info->uri,"/")==0) ||
       (strcmp(request_info->uri,"/main.htm")==0) || (strcmp(request_info->uri,"/main.html")==0) ||
       (strcmp(request_info->uri,"/index.htm")==0) || (strcmp(request_info->uri,"/main.html")==0)) {
      std::string content_type = "text/html";
      content = open_file(0, "httpsys/files/main.htm");
   } else
   if (strcmp(request_info->uri,"/h.xml")==0) {
      content_type = "xml";

      dabc::LockGuard lock(fHierarchyMutex);

      content =
            std::string("<?xml version=\"1.0\"?>\n") +
            std::string("<dabc version=\"2\" xmlns:dabc=\"http://dabc.gsi.de/xhtml\">\n")+
            fHierarchy.SaveToXml(false) +
            std::string("</dabc>\n");
   } else
   if (strcmp(request_info->uri,"/chartreq.htm")==0) {
//      content_type = "xml";
//      content = "<?xml version=\"1.0\"?>\n"
      //                "<node>"
//                "</node>";
      std::string rates = request_info->query_string ? request_info->query_string : "";

      //content = "{ \"rates\": [\n";
      content = "[\n";
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

      content_type = "text/plain";

      // DOUT0("Request %s", content.c_str());
   } else
   if (strcmp(request_info->uri,"/h.htm")==0) {

      const char* hhhh = open_file((mg_connection*) 1, "httpsys/files/hierarchy.htm");
      if ((hhhh==0)) {
         EOUT("Cannot find files in httpsys!");
         return 0;
      }

      content = hhhh;
   } else
   if (strcmp(request_info->uri,"/nodetopology.txt")==0) {
      content_type = "text/plain";
      dabc::LockGuard lock(fHierarchyMutex);
      content = fHierarchy.SaveToJSON(true, true);
      // DOUT0("Provide hierarchy \n%s\n", content.c_str());
   } else {
      // let load some files

      DOUT0("**** GET REQ:%s query:%s", request_info->uri, request_info->query_string ? request_info->query_string : "---");

      return 0;
   }


   // Prepare the message we're going to send

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

   // Returning non-zero tells mongoose that our function has replied to
   // the client, and mongoose should not send client any more data.
   return 1;
}

const char* http::Server::open_file(const struct mg_connection* conn,
                                    const char *path, size_t *data_len)
{
   if ((path==0) || (*path==0)) return 0;

   bool force = (((long) conn) == 1);

   DOUT3("Request file %s", path);

   std::string fname(path);

   size_t pos = fname.rfind("httpsys/");
   if (pos==std::string::npos) return 0;
   fname.erase(0, pos+7);
   fname = fHttpSys + fname;

   if (!force) {
      dabc::LockGuard lock(ObjectMutex());

      FilesMap::iterator iter = fFiles.find(fname);
      if (iter!=fFiles.end()) {
         if (data_len) *data_len = iter->second.size;
         // DOUT0("Return file %s len %d", fname.c_str(), iter->second.size);
         return (const char*) iter->second.ptr;
      }
   }

   std::ifstream is(fname.c_str());

   if (!is) return 0;

   is.seekg (0, is.end);
   int length = is.tellg();
   is.seekg (0, is.beg);

   char* buf = (char*) malloc(length+1);

   is.read(buf, length);
   if (is)
     DOUT0("all characters read successfully from file.");
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

      // DOUT0("Return file %s len %u", path, iter->second.size);

      return (const char*) iter->second.ptr;
   }

   EOUT("Did not find file %s %s", path, fname.c_str());

   return 0;
}


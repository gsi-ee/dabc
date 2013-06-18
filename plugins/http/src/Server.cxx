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


#ifdef WITH_ROOT
#include "TH1.h"
#include "TFile.h"
#include "TList.h"
#include "TMemFile.h"
#include "TStreamerInfo.h"
#include "TBufferFile.h"
#endif


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
      content = open_file((struct mg_connection*)1, "httpsys/files/main.htm");
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
   if (strstr(request_info->uri,"/chartreq.htm")!=0) {

      DOUT0("Request chartreq.htm processed");

      std::string rates = request_info->query_string ? request_info->query_string : "";

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
   } else
   if (strstr(request_info->uri,"/nodetopology.txt")!=0) {
      content_type = "text/plain";
      dabc::LockGuard lock(fHierarchyMutex);
      content = fHierarchy.SaveToJSON(true, true);
      // DOUT0("Provide hierarchy \n%s\n", content.c_str());
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

   if (strstr(path,"streamerinfo.data")!=0) {
      DOUT0("Produce streamer info data %s", path);

#ifdef WITH_ROOT
      static TBufferFile* sbuf = 0;

      if (sbuf==0) {

         TMemFile* mem = new TMemFile("dummy.file", "RECREATE");
         TH1F* d = new TH1F("d","d", 10, 0, 10);
         d->Write();
         mem->WriteStreamerInfo();

         TList* l = mem->GetStreamerInfoList();
         l->Print("*");

         sbuf = new TBufferFile(TBuffer::kWrite, 100000);
         sbuf->MapObject(l);
         l->Streamer(*sbuf);

         delete l;

         delete mem;


         unsigned char* ptr = (unsigned char*) sbuf->Buffer();
         for (int n=0;n<100;n++)
            DOUT0("sbuf[%3d] = %3u", n, (unsigned int) (ptr[n]));

      }

      DOUT0("Return streamer info length %d", sbuf->Length());

      if (data_len) *data_len = sbuf->Length();
      return sbuf->Buffer();

#else
      if (data_len) *data_len = 0;
      return "";
#endif

   }


   if (strstr(path,"binary.data")!=0) {

      DOUT0("Produce binary data %s", path);

#ifdef WITH_ROOT
      static TH1* h1 = 0;
      static TBufferFile* buf = 0;

      if (h1==0) {
//         TFile* f = TFile::Open("f.root");
//         h1 = (TH1*) f->Get("h");

//         h1->SetDirectory(0);
//         delete f;


         h1 = new TH1F("myhisto","Tilte of myhisto", 100, -10., 10.);
         h1->SetDirectory(0);
         DOUT0("Get h1 %s  class %s", h1->GetName(), h1->ClassName());


         TMemFile* mem = new TMemFile("dummy.file", "RECREATE");
         TH1F* d = new TH1F("d","d", 10, 0, 10);
         d->Write();
         mem->WriteStreamerInfo();

         TList* l = mem->GetStreamerInfoList();
         l->Print("*");
         delete l;

         delete mem;

      }

      h1->FillRandom("gaus", 10000);

      if (buf!=0) { delete buf; buf = 0; }

      if (buf==0) {
         buf = new TBufferFile(TBuffer::kWrite, 100000);

         gFile = 0;
         buf->MapObject(h1);
         h1->Streamer(*buf);

         DOUT0("Produced buffer length %d", buf->Length());
      }

//      char* ptr = buf->Buffer();
//      for (int n=0;n<buf->Length();n++)
//         DOUT0("buf[%3d] = %3u", n, (unsigned int) (ptr[n]));

      DOUT0("Return length %d", buf->Length());

      if (data_len) *data_len = buf->Length();
      return buf->Buffer();


#else
      if (data_len) *data_len = 0;
      return "";
#endif

   }


   bool force = (((long) conn) == 1);

   force = true;

   if (force || (conn==0)) {
      DOUT0("Request file %s", path);
   } else {

      const struct mg_request_info *request_info = mg_get_request_info((struct mg_connection*)conn);

      const char* query = "---";
      if (request_info->query_string) query = request_info->query_string;
      const char* meth = "---";
      if (request_info->request_method) meth = request_info->request_method;

      DOUT0("Request file %s  meth:%s query:%s", path, meth, query);
   }

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
         DOUT0("Return file %s len %d", fname.c_str(), iter->second.size);
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

      DOUT0("Return file %s len %u", path, iter->second.size);

      return (const char*) iter->second.ptr;
   }

   EOUT("Did not find file %s %s", path, fname.c_str());

   return 0;
}


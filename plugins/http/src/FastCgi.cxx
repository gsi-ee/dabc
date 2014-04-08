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

#include "http/FastCgi.h"

#include <string.h>
#include <unistd.h>

#include "dabc/Publisher.h"

#ifndef DABC_WITHOUT_FASTCGI

#include "fcgiapp.h"
#include <fstream>


void FCGX_send_file(FCGX_Request* request, const char* fname)
{
   std::ifstream is(fname);

   char* buf = 0;
   int length = 0;

   if (is) {
      is.seekg (0, is.end);
      length = is.tellg();
      is.seekg (0, is.beg);

      buf = (char*) malloc(length);
      is.read(buf, length);
      if (!is) {
         free(buf);
         buf = 0; length = 0;
      }
   }

   if (buf==0) {
      FCGX_FPrintF(request->out,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n" // Always set Content-Length
            "Connection: close\r\n\r\n");
   } else {

/*      char sbuf[100], etag[100];
      time_t curtime = time(NULL);
      strftime(sbuf, sizeof(sbuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&curtime));
      snprintf(etag, sizeof(etag), "\"%lx.%ld\"",
               (unsigned long) curtime, (long) length);

      // Send HTTP reply to the client
      FCGX_FPrintF(request->out,
             "HTTP/1.1 200 OK\r\n"
             "Date: %s\r\n"
             "Last-Modified: %s\r\n"
             "Etag: %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"     // Always set Content-Length
             "\r\n", sbuf, sbuf, etag, FCGX_mime_type(fname), length);

*/
      FCGX_FPrintF(request->out,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"     // Always set Content-Length
             "\r\n", http::Server::GetMimeType(fname), length);

      FCGX_PutStr(buf, length, request->out);

      free(buf);
   }

}



#endif


http::FastCgi::FastCgi(const std::string& name, dabc::Command cmd) :
   http::Server(name, cmd),
   fCgiPort(9000),
   fDebugMode(false),
   fSocket(0),
   fThrd(0)
{
   fCgiPort = Cfg("port", cmd).AsInt(9000);
   fDebugMode = Cfg("debug", cmd).AsBool(false);
}

http::FastCgi::~FastCgi()
{
   if (fThrd) {
      fThrd->Kill();
      delete fThrd;
      fThrd = 0;
   }

   if (fSocket>0) {
      // close opened socket
      close(fSocket);
      fSocket = 0;
   }
}


void http::FastCgi::OnThreadAssigned()
{
   http::Server::OnThreadAssigned();

   std::string sport = ":9000";
   if (fCgiPort>0) sport = dabc::format(":%d",fCgiPort);

#ifndef DABC_WITHOUT_FASTCGI
   FCGX_Init();

   DOUT0("Starting FastCGI server on port %s", sport.c_str());

   fSocket = FCGX_OpenSocket(sport.c_str(), 10);

   fThrd = new dabc::PosixThread();
   fThrd->Start(http::FastCgi::RunFunc, this);

#else
   EOUT("DABC compiled without FastCgi support");
#endif

}

void* http::FastCgi::RunFunc(void* args)
{

#ifndef DABC_WITHOUT_FASTCGI

   http::FastCgi* server = (http::FastCgi*) args;

   FCGX_Request request;

   FCGX_InitRequest(&request, server->fSocket, 0);

   int count(0);

   while (1) {

      int rc = FCGX_Accept_r(&request);

      if (rc!=0) continue;

      count++;

      const char* inp_path = FCGX_GetParam("PATH_INFO", request.envp);
      const char* inp_query = FCGX_GetParam("QUERY_STRING", request.envp);

      if (server->fDebugMode) {

         FCGX_FPrintF(request.out,
            "Content-type: text/html\r\n"
            "\r\n"
            "<title>FastCGI echo (fcgiapp version)</title>"
            "<h1>FastCGI echo (fcgiapp version)</h1>\n"
            "Request number %d<p>\n", count);

         char *contentLength = FCGX_GetParam("CONTENT_LENGTH", request.envp);
         int len = 0;

         if (contentLength != NULL)
             len = strtol(contentLength, NULL, 10);

         if (len <= 0) {
             FCGX_FPrintF(request.out, "No data from standard input.<p>\n");
         }
         else {
             int i, ch;

             FCGX_FPrintF(request.out, "Standard input:<br>\n<pre>\n");
             for (i = 0; i < len; i++) {
                 if ((ch = FCGX_GetChar(request.in)) < 0) {
                     FCGX_FPrintF(request.out, "Error: Not enough bytes received on standard input<p>\n");
                     break;
                 }
                 FCGX_PutChar(ch, request.out);
             }
             FCGX_FPrintF(request.out, "\n</pre><p>\n");
         }

         FCGX_FPrintF(request.out, "URI:     %s<p>\n", inp_path);
         FCGX_FPrintF(request.out, "QUERY:   %s<p>\n", inp_query ? inp_query : "---");
         FCGX_FPrintF(request.out, "<p>\n");

         FCGX_FPrintF(request.out, "Environment:<br>\n<pre>\n");
         for(char** envp = request.envp; *envp != NULL; envp++) {
             FCGX_FPrintF(request.out, "%s\n", *envp);
         }
         FCGX_FPrintF(request.out, "</pre><p>\n");

         FCGX_Finish_r(&request);
         continue;
      }


      std::string content_type, content_str;
      dabc::Buffer content_bin;

      if (server->IsFileRequested(inp_path, content_str)) {
         FCGX_send_file(&request, content_str.c_str());
         FCGX_Finish_r(&request);
         continue;
      }

      if (!server->Process(inp_path, inp_query,
                           content_type, content_str, content_bin)) {
         FCGX_FPrintF(request.out, "HTTP/1.1 404 Not Found\r\n"
                                   "Content-Length: 0\r\n"
                                   "Connection: close\r\n\r\n");
      } else

      if (content_type=="__file__") {
         FCGX_send_file(&request, content_str.c_str());
      } else

      if (!content_bin.null()) {
         FCGX_FPrintF(request.out,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: %s\r\n"
                  "Content-Length: %ld\r\n"
                  "Connection: keep-alive\r\n"
                  "\r\n",
                  content_type.c_str(),
                  content_bin.GetTotalSize());

         FCGX_PutStr((const char*) content_bin.SegmentPtr(), (int) content_bin.GetTotalSize(), request.out);
      } else {

         // Send HTTP reply to the client
         FCGX_FPrintF(request.out,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"        // Always set Content-Length
             "\r\n"
             "%s",
             content_type.c_str(),
             content_str.length(),
             content_str.c_str());
      }


      FCGX_Finish_r(&request);

   } /* while */

#endif

   return 0;
}

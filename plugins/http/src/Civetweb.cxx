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

#include "http/Civetweb.h"

#include <cstring>

http::Civetweb::Civetweb(const std::string &name, dabc::Command cmd) :
   http::Server(name, cmd),
   fHttpPort(),
   fHttpsPort(),
   fAuthFile(),
   fAuthDomain(),
   fSslCertif(),
   fCtx(nullptr)
{
   fHttpPort = Cfg("port", cmd).AsStr("8090");
   fHttpsPort = Cfg("ports", cmd).AsStr();
   fNumThreads = Cfg("thrds", cmd).AsInt(5);
   fAuthFile = Cfg("auth_file", cmd).AsStr();
   fAuthDomain = Cfg("auth_domain", cmd).AsStr("dabc@server");

   // when authentication file specified, one could decide which is default behavior
   if (!fAuthFile.empty())
      fDefaultAuth = Cfg("auth_default", cmd).AsBool(true) ? 1 : 0;

   fSslCertif = Cfg("ssl_certif", cmd).AsStr("");
   if (!fSslCertif.empty() && (fHttpsPort.length()==0)) fHttpsPort = "443";
   if (fSslCertif.empty()) fHttpsPort.clear();

   memset(&fCallbacks, 0, sizeof(fCallbacks));
}

http::Civetweb::~Civetweb()
{
   if (fCtx) {
      mg_stop(fCtx);
      fCtx = nullptr;
   }
}


void http::Civetweb::OnThreadAssigned()
{
   http::Server::OnThreadAssigned();

   std::string sport, sthrds;

   if (fHttpPort.length() > 0)
      sport = fHttpPort;
   if (fHttpsPort.length()>0) {
      if (!sport.empty()) sport.append(",");
      sport.append(fHttpsPort);
   }
   sthrds = dabc::format("%d", fNumThreads);

   //std::string sport = dabc::format("%d", fHttpPort);
   DOUT0("Starting HTTP server on port(s) %s", sport.c_str());

   const char *options[20];
   int op(0);

   options[op++] = "listening_ports";
   options[op++] = sport.c_str();
   options[op++] = "num_threads";
   options[op++] = sthrds.c_str();

#ifndef NO_SSL
   if (!fSslCertif.empty()) {
      options[op++] = "ssl_certificate";
      options[op++] = fSslCertif.c_str(); // "ssl_cert.pem";
   }
#endif

   if (!fAuthFile.empty() && !fAuthDomain.empty()) {
      options[op++] = "global_auth_file";
      options[op++] = fAuthFile.c_str();
      options[op++] = "authentication_domain";
      options[op++] = fAuthDomain.c_str();
   }

   options[op++] = "enable_directory_listing";
   options[op++] = "no";

   options[op++] = nullptr;

   // fCallbacks.begin_request = http::Civetweb::begin_request_handler;
   fCallbacks.log_message = http::Civetweb::log_message_handler;

   // Start the web server.
   fCtx = mg_start(&fCallbacks, this, options);

   mg_set_request_handler(fCtx,"/",http::Civetweb::begin_request_handler,0);

   if (!fCtx) EOUT("Fail to start civetweb on port %s", sport.c_str());
}

int http::Civetweb::log_message_handler(const struct mg_connection *conn, const char *message)
{
   //const struct mg_context *ctx = mg_get_context(conn);
   //http::Civetweb* server = (http::Civetweb*) mg_get_user_data(ctx);

   (void) conn;
   EOUT("civetweb: %s",message);

   return 0;
}


int http::Civetweb::begin_request_handler(struct mg_connection *conn, void* )
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);
   http::Civetweb* server = (http::Civetweb*) (request_info ? request_info->user_data : nullptr);
   if (!server) return 0;

   DOUT3("BEGIN_REQ: uri:%s query:%s", request_info->local_uri, request_info->query_string);

   std::string filename;

   if (server->IsFileRequested(request_info->local_uri, filename)) {
      const char *mime_type = http::Server::GetMimeType(filename.c_str());
      if (mime_type)
         mg_send_mime_file(conn, filename.c_str(), mime_type);
      else
         mg_send_file(conn, filename.c_str());
      return 1;
   }

   std::string content_type, content_header, content_str;
   dabc::Buffer content_bin;

   if (!server->Process(request_info->local_uri, request_info->query_string,
                        content_type, content_header, content_str, content_bin)) {
      mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                      "Content-Length: 0\r\n"
                      "Connection: close\r\n\r\n");
   } else if (content_type=="__file__") {
      mg_send_file(conn, content_str.c_str());
   } else if (!content_bin.null()) {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "%s"
                "Content-Length: %u\r\n"
                "Connection: keep-alive\r\n"
                "\r\n",
                content_type.c_str(),
                content_header.c_str(),
                (unsigned) content_bin.GetTotalSize());
      mg_write(conn, content_bin.SegmentPtr(), (size_t) content_bin.GetTotalSize());
   } else {

      // Send HTTP reply to the client
      mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "%s"
            "Content-Length: %d\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
             content_type.c_str(),
             content_header.c_str(),
             (int) content_str.length(),
             content_str.c_str());
   }

    // Returning non-zero tells civetweb that our function has replied to
    // the client, and civetweb should not send client any more data.
    return 1;
}

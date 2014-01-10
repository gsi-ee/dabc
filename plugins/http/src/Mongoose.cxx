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

#include "http/Mongoose.h"

#include <string.h>

#include "dabc/Publisher.h"

http::Mongoose::Mongoose(const std::string& name, dabc::Command cmd) :
   http::Server(name, cmd),
   fHttpPort(0),
   fHttpsPort(0),
   fAuthFile(),
   fAuthDomain(),
   fSslCertif(),
   fCtx(0)
{
   fHttpPort = Cfg("port", cmd).AsInt(8090);
   fHttpsPort = Cfg("ports", cmd).AsInt(0);
   fAuthFile = Cfg("auth_file", cmd).AsStr();
   fAuthDomain = Cfg("auth_domain", cmd).AsStr("dabc@server");
   fSslCertif = Cfg("ssl_certif", cmd).AsStr("");
   if (!fSslCertif.empty() && (fHttpsPort<=0)) fHttpsPort = 443;
   if (fSslCertif.empty()) fHttpsPort = 0;

   memset(&fCallbacks, 0, sizeof(fCallbacks));
}

http::Mongoose::~Mongoose()
{
   if (fCtx!=0) {
      mg_stop(fCtx);
      fCtx = 0;
   }
}


void http::Mongoose::OnThreadAssigned()
{
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

   fCallbacks.begin_request = http::Mongoose::begin_request_handler;

   // Start the web server.
   fCtx = mg_start(&fCallbacks, this, options);
}


int http::Mongoose::begin_request_handler(struct mg_connection *conn)
{
   http::Mongoose* server = (http::Mongoose*) mg_get_request_info(conn)->user_data;
   if (server==0) return 0;

   const struct mg_request_info *request_info = mg_get_request_info(conn);

   DOUT2("BEGIN_REQ: uri:%s query:%s", request_info->uri, request_info->query_string);

   std::string pathname, filename, query;

   if (server->IsFileRequested(request_info->uri, filename)) {
      mg_send_file(conn, filename.c_str());
      return 1;
   }

   const char* rslash = strrchr(request_info->uri,'/');
   if (rslash==0) {
      filename = request_info->uri;
   } else {
      pathname.append(request_info->uri, rslash - request_info->uri);
      if (pathname=="/") pathname.clear();
      filename = rslash+1;
   }

   if (request_info->query_string) query = request_info->query_string;


   std::string content_type, content_str;
   dabc::Buffer content_bin;

   if (!server->Process(pathname, filename, query,
                        content_type, content_str, content_bin)) {
      mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n");
   } else

   if (content_type=="__file__") {
      mg_send_file(conn, content_str.c_str());
   } else

   if (!content_bin.null()) {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %u\r\n"
                "Connection: keep-alive\r\n"
                "\r\n", content_type.c_str(), (unsigned) content_bin.GetTotalSize());
      mg_write(conn, content_bin.SegmentPtr(), (size_t) content_bin.GetTotalSize());
   } else {

      // Send HTTP reply to the client
      mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
             content_type.c_str(),
             (int) content_str.length(),
             content_str.c_str());
   }

    // Returning non-zero tells mongoose that our function has replied to
    // the client, and mongoose should not send client any more data.
    return 1;
}

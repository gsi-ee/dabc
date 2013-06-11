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


static int begin_request_handler(struct mg_connection *conn)
{
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  char content[100];

  // Prepare the message we're going to send
  int content_length = snprintf(content, sizeof(content),
                                "Hello from DABC 2.0! Remote port: %d",
                                request_info->remote_port);

  // Send HTTP reply to the client
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            content_length, content);

  // Returning non-zero tells mongoose that our function has replied to
  // the client, and mongoose should not send client any more data.
  return 1;
}


http::Server::Server(const std::string& name) :
   dabc::Worker(MakePair(name)),
   fHttpPort(0),
   fEnabled(false),
   fCtx(0)
{
   memset(&fCallbacks, 0, sizeof(fCallbacks));
   fCallbacks.begin_request = begin_request_handler;

   fHttpPort = Cfg("port").AsUInt(8080);
   fEnabled = Cfg("enabled").AsBool(true);
   if (!fEnabled) return;
}

http::Server::~Server()
{
   if (fCtx!=0) mg_stop(fCtx);

   fCtx = 0;
}

void http::Server::OnThreadAssigned()
{
   DOUT0("Starting HTTP server on port %d", fHttpPort);

   if (!IsEnabled()) {
      EOUT("http server was not enabled - why it is started??");
      return;
   }

   const char *options[] = {"listening_ports", "8080", 0};

   // Start the web server.
   fCtx = mg_start(&fCallbacks, NULL, options);
}

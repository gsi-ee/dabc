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

#include "dabc/Hierarchy.h"
#include "dabc/Manager.h"


static int begin_request_handler(struct mg_connection *conn)
{
   http::Server* serv = (http::Server*) mg_get_request_info(conn)->user_data;

   return serv->begin_request(conn);
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
   if (!IsEnabled()) {
      EOUT("http server was not enabled - why it is started??");
      return;
   }

   DOUT0("Starting HTTP server on port %d", fHttpPort);

   std::string sport = dabc::format("%d", fHttpPort);

   const char *options[3];
   options[0] = "listening_ports";
   options[1] = sport.c_str();
   options[2] = 0;

   // Start the web server.
   fCtx = mg_start(&fCallbacks, this, options);
}

int http::Server::begin_request(struct mg_connection *conn)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   std::string content;
   std::string content_type = "text/html";

   if (strcmp(request_info->uri,"/")==0) {
      dabc::formats(content, "Hello from DABC 2.0!<br><br>"
                             "This is is <a href=\"h.xml\">hierarchy </a> of dabc objects");
   } else
   if (strcmp(request_info->uri,"/h.xml")==0) {
      content_type = "xml";
      dabc::Hierarchy h;
      h.UpdateHierarchy(dabc::mgr);
      content =
            std::string("<?xml version=\"1.0\"?>\n") +
            std::string("<dabc version=\"2\" xmlns:dabc=\"http://dabc.gsi.de/xhtml\">\n")+
            h.SaveToXml(false) +
            std::string("</dabc>\n");
   } else {
      dabc::formats(content, "Hello from DABC 2.0!<br>"
                             "Requested url:%s<br>"
                             "Client port:%d<br>"
                             "Page not exists, go to  <a href=\"/\">home</a> page<br>",
                             request_info->uri, request_info->remote_port);
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


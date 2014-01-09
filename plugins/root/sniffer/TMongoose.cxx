// $Id$
// Author: Sergey Linev   21/12/2013

#include "TMongoose.h"

#include "../mongoose/mongoose.h"

#include <stdlib.h>

#include "THttpServer.h"

static int begin_request_handler(struct mg_connection *conn)
{
   THttpServer* serv = (THttpServer*) mg_get_request_info(conn)->user_data;
   if (serv == 0) return 0;

   const struct mg_request_info *request_info = mg_get_request_info(conn);

   TString filename;

   if (serv->IsFileRequested(request_info->uri, filename)) {
      mg_send_file(conn, filename.Data());
      return 1;
   }

   THttpCallArg arg;
   arg.SetPathAndFileName(request_info->uri); // path and file name
   arg.SetQuery(request_info->query_string);  //! additional arguments

   if (!serv->ExecuteHttp(&arg) || arg.Is404()) {
      mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                      "Content-Length: 0\r\n"
                      "Connection: close\r\n\r\n");
      return 1;
   }

   if (arg.IsFile()) {
      mg_send_file(conn, arg.GetContent());
      return 1;
   }

   if (arg.IsBinData()) {
      mg_printf(conn,
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %ld\r\n"
               "Connection: keep-alive\r\n"
               "\r\n",
               arg.GetContentType(),
               arg.GetBinDataLength());
      mg_write(conn, arg.GetBinData(), (size_t) arg.GetBinDataLength());
      return 1;
   }


   // Send HTTP reply to the client
   mg_printf(conn,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: %s\r\n"
          "Content-Length: %d\r\n"        // Always set Content-Length
          "\r\n"
          "%s",
          arg.GetContentType(),
          arg.GetContentLength(),
          arg.GetContent());

   // Returning non-zero tells mongoose that our function has replied to
   // the client, and mongoose should not send client any more data.
   return 1;
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMongoose                                                            //
//                                                                      //
// http server implementation, based on mongoose embedded server        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


TMongoose::TMongoose() :
   THttpEngine("mongoose", "compact embedded http server"),
   fCtx(0),
   fCallbacks(0)
{
   // constructor
}

TMongoose::~TMongoose()
{
   // destructor

   if (fCtx!=0) mg_stop((struct mg_context*) fCtx);
   if (fCallbacks != 0) free(fCallbacks);
   fCtx = 0;
   fCallbacks = 0;
}

Bool_t TMongoose::Create(const char* args)
{
   // Creates embedded mongoose server
   // As argument, http port should be specified in form "8090"

   fCallbacks = malloc(sizeof(struct mg_callbacks));
   memset(fCallbacks, 0, sizeof(struct mg_callbacks));
   ((struct mg_callbacks*) fCallbacks)->begin_request = begin_request_handler;

   const char* sport = "8080";

   // for the moment the only argument is port number
   if (args!=0) sport = args;

   const char *options[100];
   int op(0);

   Info("Create", "Starting HTTP server on port %s", sport);

   options[op++] = "listening_ports";
   options[op++] = sport;
   options[op++] = "num_threads";
   options[op++] = "5";
   options[op++] = 0;

   // Start the web server.
   fCtx = mg_start((struct mg_callbacks*) fCallbacks, GetServer(), options);

   return kTRUE;
}


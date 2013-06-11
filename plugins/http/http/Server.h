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

#ifndef HTTP_Server
#define HTTP_Server

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#include "mongoose.h"


namespace http {

   /** \brief %Server provides http access to DABC
    *
    */

   class Server : public dabc::Worker  {
      protected:
         int fHttpPort;
         bool fEnabled;

         struct mg_context *   fCtx;
         struct mg_callbacks   fCallbacks;

         virtual void OnThreadAssigned();

      public:
         Server(const std::string& name);

         virtual ~Server();

         virtual const char* ClassName() const { return "HttpServer"; }

         bool IsEnabled() const { return fEnabled; }
   };

}

#endif

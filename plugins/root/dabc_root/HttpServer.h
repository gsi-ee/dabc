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

#ifndef DABC_ROOT_HttpServer
#define DABC_ROOT_HttpServer


namespace dabc_root {

   /** \brief %Inetrface class to start HTTP server from ROOT
    *
    */

   class HttpServer {
      public:
         HttpServer() {}
         virtual ~HttpServer() {}

         static void Start(int port = 8080);
   };



}

#endif

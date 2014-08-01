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

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

namespace http {

   /** \brief %Server provides http access to DABC
    *
    */

   class Server : public dabc::Worker  {

      protected:
         std::string fDabcSys;      ///< location of DABC installation
         std::string fHttpSys;      ///< location of http plugin, need to read special files
         std::string fGo4Sys;       ///< location of go4 (if any)
         std::string fRootSys;      ///< location of ROOT (if any)
         std::string fJSRootIOSys;  ///< location of JSRootIO (if any)
         int         fDefaultAuth;  ///< 0 - false, 1 - true, -1 - ignored

         /** Check if file is requested. Can only be from server */
         bool IsFileRequested(const char* uri, std::string& fname);

         void ExtractPathAndFile(const char* uri, std::string& pathname, std::string& filename);

         /** Returns true if authentication is required */
         bool IsAuthRequired(const char* uri);

         /** Method process different URL requests, should be called from server thread */
         bool Process(const char* uri, const char* query,
                      std::string& content_type,
                      std::string& content_header,
                      std::string& content_str,
                      dabc::Buffer& content_bin);

      public:
         Server(const std::string& name, dabc::Command cmd = 0);

         virtual ~Server();

         static const char* GetMimeType(const char* fname);
   };

}

#endif

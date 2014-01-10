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
         std::string fHttpSys;     ///< location of http plugin, need to read special files
         std::string fGo4Sys;      ///< location of go4 (if any)
         std::string fRootSys;     ///< location of ROOT (if any)
         std::string fJSRootIOSys; ///< location of JSRootIO (if any)

         bool ProcessGetItem(const std::string& itemname, const std::string& query, std::string& replybuf);

         bool ProcessExecute(const std::string& itemname, const std::string& query, std::string& replybuf);

         /** \brief Check if file is requested. Can only be from server */
         bool IsFileRequested(const char* uri, std::string& fname);

         bool Process(const std::string& path, const std::string& file, const std::string& query,
                      std::string& content_type, std::string& content_str, dabc::Buffer& content_bin);

      public:
         Server(const std::string& name, dabc::Command cmd = 0);

         virtual ~Server();
   };

}

#endif

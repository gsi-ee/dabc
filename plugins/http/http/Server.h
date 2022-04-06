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

#include <vector>

namespace http {

   /** \brief %Server provides http access to DABC
    *
    */

   class Server : public dabc::Worker  {

      protected:

         struct Location {
            std::string fFilePath;      ///< path on file system /home/user/dabc
            std::string fAbsPrefix;     ///< prefix in http address like "/dabcsys"
            std::string fNamePrefix;    ///< prefix in filename like "dabc_"
            std::string fNamePrefixRepl; ///< replacement for name prefix like "/files/"
         };

         std::vector<Location> fLocations; ///< different locations known to server
         std::string fHttpSys;      ///< location of http plugin, need to read special files
         std::string fOwnJsRootSys; ///< location of internal JSROOT code, need to read special files
         std::string fJsRootSys;    ///< location of JSROOT code, need to read special files
         int         fDefaultAuth;  ///< 0 - false, 1 - true, -1 - ignored

         std::string fAutoLoad;    ///< _autoload value in h.json
         std::string fTopTitle;    ///< _toptitle value in h.json
         std::string fBrowser;     ///< _browser value in h.json
         std::string fLayout;      ///< _layout value in h.json
         std::string fDrawItem;    ///< _drawitem value in h.json, only for top page
         std::string fDrawOpt;     ///< _drawopt value in h.json, only for top page
         int         fMonitoring;  ///< _monitoring value in h.json, only for top page

         /** Check if relative path below current dir - prevents file access to top directories via http */
         static bool VerifyFilePath(const char* fname);

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
         Server(const std::string &name, dabc::Command cmd = nullptr);

         virtual ~Server();

         /** Add files location */
         void AddLocation(const std::string &filepath,
                          const std::string &absprefix,
                          const std::string &nameprefix = "",
                          const std::string &nameprefixrepl = "");

         virtual const char* ClassName() const { return "HttpServer"; }

         static const char* GetMimeType(const char* fname);
   };

}

#endif

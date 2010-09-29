/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_Url
#define DABC_Url

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {

   /** Class dabc::Url supports following format:
    *    [protocol://][hostname[:port]][/filename.ext][?options]
    */

   class Url {
      protected:
         std::string fUrl;      /** Full URL */
         bool        fValid;    /** Is URL valid */
         std::string fProtocol;
         std::string fHostName;
         int         fPort;
         std::string fFileName;
         std::string fOptions;

      public:
         Url();
         Url(const char* url);
         Url(const std::string& url);
         virtual ~Url();

         void Reset();
         bool SetUrl(const std::string& url);

         std::string GetUrl() const      { return fUrl; }
         bool IsValid() const            { return fValid; }

         std::string GetProtocol() const { return fProtocol; }
         std::string GetHostName() const { return fHostName; }
         int         GetPort() const     { return fPort; }
         std::string GetFileName() const { return fFileName; }
         std::string GetOptions() const  { return fOptions; }

   };
}



#endif

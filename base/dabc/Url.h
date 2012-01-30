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
         bool SetUrl(const std::string& url, bool showerr = true);

         std::string GetUrl() const      { return fUrl; }
         bool IsValid() const            { return fValid; }

         std::string GetProtocol() const { return fProtocol; }
         std::string GetHostName() const { return fHostName; }
         int         GetPort() const     { return fPort; }
         std::string GetPortStr() const;
         std::string GetFileName() const { return fFileName; }
         std::string GetOptions() const  { return fOptions; }
         std::string GetFullName() const;


         /**! \brief Prodoces url string with unique address of specified item */
         static std::string ComposeItemName(int nodeid, const std::string& itemname = "");

         /**! Method creates url string with port address, which includes nodeid and full portname
          * If optional parameter portid is specified, it added as last symbols of the address
          * Like call dabc::Url::ComposePortName(1, "MyModule/Output", 2) will produce string
          * "dabc://node1/MyModule/Output2" */
         static std::string ComposePortName(int nodeid, const char* fullportname, int portid = -1);

         /** \brief Method decompose from url nodeid and full item name, which includes all parents */
         static bool DecomposeItemName(const std::string& url, int& nodeid, std::string& itemtname);

         /** \brief Extracts only nodeid from url, if node is not specified or url is invalid, -1 is returned */
         static int ExtractNodeId(const std::string& url);

   };
}



#endif

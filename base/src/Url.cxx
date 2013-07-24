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

#include "dabc/Url.h"
#include "dabc/logging.h"

dabc::Url::Url()
{
   SetUrl(std::string());
}

dabc::Url::Url(const char* url)
{
   SetUrl(std::string(url ? url : ""));
}

dabc::Url::Url(const std::string& url)
{
   SetUrl(url);
}

dabc::Url::~Url()
{
}

void dabc::Url::Reset()
{
   SetUrl("", false);
}


bool dabc::Url::SetUrl(const std::string& url, bool showerr)
{
   fValid = false;
   fUrl.clear();
   fProtocol.clear();
   fHostName.clear();
   fPort = 0;
   fFileName.clear();
   fOptions.clear();

   if (url.length()==0) return false;

   fUrl = url;
   fValid = true;

   std::string s = fUrl;
   std::size_t pos = s.find("://");

   if (pos != std::string::npos) {
      fProtocol = s.substr(0, pos);
      s.erase(0, pos + 3);
   }

   if (s.length() == 0) return fValid;


   pos = s.rfind("?");
   if (pos != std::string::npos) {
      fOptions = s.substr(pos+1);
      s.erase(pos);
   }

   pos = s.find("/");

   if (pos==0) {
      fFileName = s;
   } else
   if (pos != std::string::npos) {
      fHostName = s.substr(0, pos);
      fFileName = s.substr(pos+1);
   } else {
      fHostName = s;
   }

   pos = fHostName.find(":");
   if (pos != std::string::npos) {
      if (!dabc::str_to_int(fHostName.c_str()+pos+1, &fPort)) {
         if (showerr) EOUT("Invalid URL format:%s - wrong port number", fHostName.c_str());
         fValid = false;
      }
      fHostName.erase(pos);
   }

   return fValid;
}

std::string dabc::Url::GetFullName() const
{
   if (fFileName.length()==0) return fHostName;
   if (fHostName.length()==0) return fFileName;

   return dabc::format("%s/%s", fHostName.c_str(), fFileName.c_str());
}

std::string dabc::Url::GetPortStr() const
{
   if (fPort<=0) return std::string();

   return dabc::format("%d", fPort);
}

std::string dabc::Url::GetHostNameWithPort(int dfltport) const
{
   int port = fPort;
   if (port <= 0) port = dfltport;

   return (port>0) ?  fHostName + dabc::format(":%d", port) : fHostName;
}

std::string dabc::Url::ComposeItemName(int nodeid, const std::string& itemname)
{
   if (nodeid<0) return std::string();

   return dabc::format("dabc://node%d/%s", nodeid, itemname.length() > 0 ? itemname.c_str() : "");
}

std::string dabc::Url::ComposePortName(int nodeid, const std::string& fullportname, int portid)
{
   std::string sbuf;
   if ((nodeid<0) || fullportname.empty()) return sbuf;

   sbuf = ComposeItemName(nodeid, fullportname);

   if (portid>=0)
      sbuf += dabc::format("%d", portid);

   return sbuf;
}

bool dabc::Url::DecomposeItemName(const std::string& name, int& nodeid, std::string& fullportname)
{
   Url url;

//   DOUT0("Url %s valid %d protocol %s host %s file %s", name, url.IsValid(), url.GetProtocol().c_str(), url.GetHostName().c_str(), url.GetFileName().c_str());

   if (!url.SetUrl(name, false)) return false;

   if (url.GetProtocol().length()==0) {
      fullportname = url.GetFullName();
      nodeid = -1;
      return true;
   }

   if (url.GetProtocol().compare("dabc")!=0) return false;

   std::string node = url.GetHostName();
   if (node.compare(0, 4, "node")!=0) return false;
   node.erase(0,4);

   nodeid = -1;
   if (!str_to_int(node.c_str(), &nodeid)) return false;
   if (nodeid<0) return false;

   fullportname = url.GetFileName();
   return true;
}


int dabc::Url::ExtractNodeId(const std::string& url)
{
   int nodeid(-1);
   std::string itemname;

   if (DecomposeItemName(url, nodeid, itemname)) return nodeid;

   return -1;
}

std::string dabc::Url::GetOptionsPart(int number) const
{
   std::string res;
   GetOption("", number, &res);
   return res;
}


bool dabc::Url::GetOption(const std::string& optname, int optionnumber, std::string* value) const
{
   if (value) value->clear();

   if (fOptions.empty()) return false;
   if ((optionnumber<0) && optname.empty()) return false;

   int cnt = 0;
   size_t p = 0;

   while (p < fOptions.length()) {

      size_t separ = fOptions.find("&", p);

      if (separ==std::string::npos) separ = fOptions.length();

//      printf("Search for option %s  fullstr %s p=%d separ=%d\n", optname.c_str(), fOptions.c_str(), p, separ);

      if (optionnumber>=0) {
         if (cnt==optionnumber) {
            if (value) *value = fOptions.substr(p, separ-p);
            return true;
         }
         cnt++;
      } else
      if (separ-p >= optname.length()) {
         bool find = fOptions.compare(p, optname.length(), optname)==0;
         if (find) {

            p+=optname.length();

            // this is just option name, nothing else - value is empty http://host?option
            if (p==separ) return true;

            if (fOptions[p]=='=') {
               // also empty option possible, but with syntax http://host?option=
               p++;
               if ((p<separ) && value) *value = fOptions.substr(p, separ-p);
               return true;
            }
         }
      }

      p = separ + 1; // +1 while we should skip & from search, no matter at the end of the string
   }

   return false;
}


std::string dabc::Url::GetOptionStr(const std::string& optname, const std::string& dflt) const
{
   std::string res;

   if (GetOption(optname, -1, &res)) return res;

   return dflt;
}

int dabc::Url::GetOptionInt(const std::string& optname, int dflt) const
{
   std::string res = GetOptionStr(optname);

   if (res.empty()) return dflt;

   int resi(0);
   if (str_to_int(res.c_str(), &resi)) return resi;

   return dflt;
}


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

   if (pos != std::string::npos) {
      fHostName = s.substr(0, pos);
      fFileName = s.substr(pos+1);
   } else {
      fHostName = s;
   }

   pos = fHostName.find(":");
   if (pos != std::string::npos) {
      if (!dabc::str_to_int(fHostName.c_str()+pos+1, &fPort)) {
         if (showerr) EOUT(("Invalid URL format:%s - wrong port number", fHostName.c_str()));
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

std::string dabc::Url::ComposeItemName(int nodeid, const std::string& itemname)
{
   if (nodeid<0) return std::string();

   return dabc::format("dabc://node%d/%s", nodeid, itemname.length() > 0 ? itemname.c_str() : "");
}

std::string dabc::Url::ComposePortName(int nodeid, const char* fullportname, int portid)
{
   std::string sbuf;
   if ((nodeid<0) || (fullportname==0)) return sbuf;

   sbuf = ComposeItemName(nodeid, fullportname);

   if (portid>=0)
      sbuf += dabc::format("%d", portid);

   return sbuf;
}

bool dabc::Url::DecomposeItemName(const std::string& name, int& nodeid, std::string& fullportname)
{
   Url url;

//   DOUT0(("Url %s valid %d protocol %s host %s file %s", name, url.IsValid(), url.GetProtocol().c_str(), url.GetHostName().c_str(), url.GetFileName().c_str()));

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

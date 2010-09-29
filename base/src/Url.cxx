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

bool dabc::Url::SetUrl(const std::string& url)
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
         EOUT(("Invalid URL format:%s - wrong port number"));
         fValid = false;
      }
      fHostName.erase(pos);
   }

   return fValid;
}

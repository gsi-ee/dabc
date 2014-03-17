#include "rfio/FileInterface.h"

#include "rawapin.h"

#include <string.h>

#include "dabc/Url.h"
#include "dabc/logging.h"


rfio::FileInterface::FileInterface() :
   dabc::FileInterface(),
   fDataMoverIndx(0)
{
   memset(fDataMoverName, 0, sizeof(fDataMoverName));
}


dabc::FileInterface::Handle rfio::FileInterface::fopen(const char* fname, const char* mode, const char* opt)
{
   // clear possible parameters
   fDataMoverName[0] = 0;
   fDataMoverIndx = 0;

   if ((opt==0) || (*opt==0))
      return (Handle) rfio_fopen((char*)fname, (char*)mode);

   const char* pcc = strrchr(fname, ':');

   char rfioBase[128];

   if (pcc!=0) {
      int len = pcc - fname;
      strncpy(rfioBase, fname, len);
      rfioBase[len] = 0;
   } else {
      strcpy(rfioBase, fname);
   }

   bool isany = false;

   dabc::Url url;
   url.SetOptions(opt);
   int rfioCopyMode = 1;
   std::string rfioLustrePath = "/hera/hades/may14raw";
   int rfioCopyFrac = 1;
   int rfioMaxFile = 0;
   int rfioPathConv = 0;
   const char* pcOptions = "wb";

   if (url.HasOption("rfioCopyMode")) {
      rfioCopyMode = url.GetOptionInt("rfioCopyMode", rfioCopyMode);
      isany = true;
   }

   if (url.HasOption("rfioCopyFrac")) {
      rfioCopyFrac = url.GetOptionInt("rfioCopyFrac", rfioCopyFrac);
      isany = true;
   }

   if (url.HasOption("rfioMaxFile")) {
      rfioMaxFile = url.GetOptionInt("rfioMaxFile", rfioMaxFile);
      isany = true;
   }

   if (url.HasOption("rfioPathConv")) {
      rfioPathConv = url.GetOptionInt("rfioPathConv", rfioPathConv);
      isany = true;
   }

   if (url.HasOption("rfioLustrePath")) {
      rfioLustrePath = url.GetOptionStr("rfioLustrePath", rfioLustrePath);
      isany = true;
   }

   if (isany) {
      DOUT0("rfioBase=%s rfioLustrePath=%s rfioCopyMode=%d rfioCopyFrac=%d rfioMaxFile=%d rfioPathConv=%d",
            rfioBase, rfioLustrePath.c_str(), rfioCopyMode, rfioCopyFrac, rfioMaxFile, rfioPathConv);

      return (Handle) rfio_fopen_gsidaq_dm(rfioBase, (char*) pcOptions,
                                           fDataMoverName, &fDataMoverIndx,
                                           rfioCopyMode, (char*) rfioLustrePath.c_str(),
                                           rfioCopyFrac, rfioMaxFile, rfioPathConv);
   }


   return (Handle) rfio_fopen((char*)fname, (char*)mode);
}


int rfio::FileInterface::GetFileIntPar(Handle, const char* parname)
{
   if (strcmp(parname, "RFIO")==0) return 8; // return RFIO version number
   if (strcmp(parname, "DataMoverIndx")==0) return fDataMoverIndx;
   return 0;
}

bool rfio::FileInterface::GetFileStrPar(Handle, const char* parname, char* sbuf, int sbuflen)
{
   if (strcmp(parname, "DataMoverName")==0)
      if (strlen(fDataMoverName) < (unsigned) sbuflen) {
         strcpy(sbuf, fDataMoverName);
         return true;
      }

   return false;
}


void rfio::FileInterface::fclose(Handle f)
{
   if (f!=0) rfio_fclose((RFILE*)f);
}

size_t rfio::FileInterface::fwrite(const void* ptr, size_t sz, size_t nmemb, Handle f)
{

   return ((f==0) || (ptr==0) || (sz==0)) ? 0 : rfio_fwrite((const char*)ptr, sz, nmemb, (RFILE*) f) / sz;
}

size_t rfio::FileInterface::fread(void* ptr, size_t sz, size_t nmemb, Handle f)
{
   return ((f==0) || (ptr==0) || (sz==0)) ? 0 : rfio_fread((char*) ptr, sz, nmemb, (RFILE*) f) / sz;
}

bool rfio::FileInterface::fseek(Handle f, long int offset, bool relative)
{
   if (f==0) return false;

   int fileid = rfio_ffileid((RFILE*)f);

   if (fileid<0) return false;

   return rfio_lseek(fileid, offset, relative ? SEEK_CUR : SEEK_SET) >= 0;
}


bool rfio::FileInterface::feof(Handle f)
{
   return f==0 ? false : (rfio_fendfile((RFILE*)f) > 0);
}

bool rfio::FileInterface::fflush(Handle f)
{
   return true;

   // return f==0 ? false : ::fflush((FILE*)f)==0;
}

dabc::Object* rfio::FileInterface::fmatch(const char* fmask, bool select_files)
{
   return 0;
}

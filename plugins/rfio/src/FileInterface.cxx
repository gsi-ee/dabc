#include "rfio/FileInterface.h"

#include "rawapin.h"

#include <string.h>

#include "dabc/Url.h"
#include "dabc/logging.h"


rfio::FileInterface::FileInterface() :
   dabc::FileInterface(),
   fRemote(0),
   fDataMoverIndx(0)
{
   memset(fDataMoverName, 0, sizeof(fDataMoverName));
}

rfio::FileInterface::~FileInterface()
{
   if (fRemote!=0) {
      DOUT0("Close existing connection to RFIO data mover");
      rfio_fclose((RFILE*) fRemote);
      fRemote = 0;
   }
}


dabc::FileInterface::Handle rfio::FileInterface::fopen(const char* fname, const char* mode, const char* opt)
{
   // clear possible parameters
   if (((opt==0) || (*opt==0)) && (fRemote==0)) {
      return (Handle) rfio_fopen((char*)fname, (char*)mode);
   }

   if (fRemote == 0) {
      bool isany = false;

      dabc::Url url;
      url.SetOptions(opt);
      int rfioCopyMode = 1;
      std::string rfioLustrePath = "/hera/hades/may14raw";
      int rfioCopyFrac = 1;
      int rfioMaxFile = 0;
      int rfioPathConv = 0;
      std::string rfioOptions = "wb";

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

      if (url.HasOption("rfioOptions")) {
         rfioOptions = url.GetOptionStr("rfioOptions", rfioOptions);
         isany = true;
      }

      if (isany) {

         char rfioBase[1024];

         strcpy(rfioBase, fname);

         char* pcc = (char*) strrchr(rfioBase, ':');

         if (pcc!=0) {
            pcc++;
            strncpy(pcc, "\0", 1);  /* terminates after node name */
         }

         strcpy(fDataMoverName,"");
         fDataMoverIndx = 0;

         DOUT0("Try to connect to RFIO mover rfioBase=%s rfioOptions=%s rfioLustrePath=%s rfioCopyMode=%d rfioCopyFrac=%d rfioMaxFile=%d rfioPathConv=%d",
               rfioBase, rfioOptions.c_str(), rfioLustrePath.c_str(), rfioCopyMode, rfioCopyFrac, rfioMaxFile, rfioPathConv);

         fRemote = rfio_fopen_gsidaq_dm(rfioBase, (char*) rfioOptions.c_str(),
               fDataMoverName, &fDataMoverIndx,
               rfioCopyMode, (char*) rfioLustrePath.c_str(),
               rfioCopyFrac, rfioMaxFile, rfioPathConv);
         if (fRemote == 0) {
            EOUT("Fail to create connection with RFIO, using following arguments"
                  "rfioBase=%s rfioOptions=%s rfioLustrePath=%s rfioCopyMode=%d rfioCopyFrac=%d rfioMaxFile=%d rfioPathConv=%d",
                  rfioBase, rfioOptions.c_str(), rfioLustrePath.c_str(), rfioCopyMode, rfioCopyFrac, rfioMaxFile, rfioPathConv);
            return 0;
         }

         DOUT0("Open connection to datamover %d %s", fDataMoverIndx, fDataMoverName);
      }
   }

   if (fRemote==0)
      return (Handle) rfio_fopen((char*)fname, (char*)mode);

   int rev = rfio_fnewfile((RFILE*)fRemote, (char*) fname);

   if (rev!=0) {
      EOUT("Fail to create new RFIO file %s via existing datamover %d %s connection", fDataMoverIndx, fDataMoverName, fname);
      return 0;
   }

   return (Handle) fRemote;
}


int rfio::FileInterface::GetFileIntPar(Handle, const char* parname)
{
   if (strcmp(parname, "RFIO")==0) return 8; // return RFIO version number
   if (fRemote && strcmp(parname, "DataMoverIndx")==0) return fDataMoverIndx;
   return 0;
}

bool rfio::FileInterface::GetFileStrPar(Handle, const char* parname, char* sbuf, int sbuflen)
{
   if (fRemote && strcmp(parname, "DataMoverName")==0)
      if (strlen(fDataMoverName) < (unsigned) sbuflen) {
         strcpy(sbuf, fDataMoverName);
         return true;
      }

   return false;
}


void rfio::FileInterface::fclose(Handle f)
{
   if ((fRemote!=0) && (f==fRemote)) {
      rfio_fendfile((RFILE*) fRemote);
   } else
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

#ifdef ORIGIN_ADSM
   int fileid = -1;
   printf("rfio::FileInterface::fseek not working with original version of ADSM library\n");
#else
   int fileid = rfio_ffileid((RFILE*)f);
#endif

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

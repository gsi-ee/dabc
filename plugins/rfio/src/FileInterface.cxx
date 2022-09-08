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

#include "rfio/FileInterface.h"

#include "rawapin.h"

#include <cstring>

#include "dabc/Url.h"
#include "dabc/logging.h"


rfio::FileInterface::FileInterface() :
   dabc::FileInterface(),
   fRemote(nullptr),
   fOpenedCounter(0),
   fDataMoverIndx(0)
{
   memset(fDataMoverName, 0, sizeof(fDataMoverName));
}

rfio::FileInterface::~FileInterface()
{
   if (fRemote) {
      DOUT2("Close existing connection to RFIO data mover");
      rfio_fclose((RFILE*) fRemote);
      fRemote = nullptr;
   }
}


dabc::FileInterface::Handle rfio::FileInterface::fopen(const char* fname, const char* mode, const char* opt)
{
   // clear possible parameters
   if ((!opt || (*opt==0)) && !fRemote) {
      return (Handle) rfio_fopen((char*)fname, (char*)mode);
   }

   if (!fRemote) {
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

         strncpy(rfioBase, fname, sizeof(rfioBase)-1);

         char* pcc = (char*) strrchr(rfioBase, ':');

         if (pcc) {
            pcc++;
            *pcc = 0;  /* terminates after node name */
         }

         strncpy(fDataMoverName, "", sizeof(fDataMoverName)-1);
         fDataMoverIndx = 0;

         DOUT1("Try to connect to RFIO mover rfioBase=%s rfioOptions=%s rfioLustrePath=%s rfioCopyMode=%d rfioCopyFrac=%d rfioMaxFile=%d rfioPathConv=%d",
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

         DOUT2("Successfully opened connection to datamover %d %s", fDataMoverIndx, fDataMoverName);
      }
   }

   if (!fRemote)
      return (Handle) rfio_fopen((char*)fname, (char*)mode);

   DOUT3("Calling rfio_fnewfile %s", fname);

   int rev = rfio_fnewfile((RFILE*)fRemote, (char*) fname);

   DOUT3("Did call rfio_fnewfile %s rev = %d", fname, rev);

   if (rev != 0) {
      EOUT("Fail to create new RFIO file %s via existing datamover %d %s connection", fname, fDataMoverIndx, fDataMoverName);
      return 0;
   }

   fOpenedCounter++;

   if (fOpenedCounter > 100) EOUT("Too many (%d) files, opened via RFIO connection", fOpenedCounter);

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
         strncpy(sbuf, fDataMoverName, sbuflen);
         return true;
      }

   return false;
}


void rfio::FileInterface::fclose(Handle f)
{
   if ((fRemote!=0) && (f==fRemote)) {
      rfio_fendfile((RFILE*) fRemote);
      fOpenedCounter--;
      if (fOpenedCounter < 0) EOUT("Too many close operations - counter (%d) is negative", fOpenedCounter);
      return;
   }

   if (fRemote!=0) EOUT("Get RFIO::fclose with unexpected argument when fRemote!=0 cnt %d", fOpenedCounter);

   if (f!=0) rfio_fclose((RFILE*)f);
}

size_t rfio::FileInterface::fwrite(const void* ptr, size_t sz, size_t nmemb, Handle f)
{

   return (!f || !ptr || (sz==0)) ? 0 : rfio_fwrite((const char*)ptr, sz, nmemb, (RFILE*) f) / sz;
}

size_t rfio::FileInterface::fread(void* ptr, size_t sz, size_t nmemb, Handle f)
{
   return (!f || !ptr || (sz==0)) ? 0 : rfio_fread((char*) ptr, sz, nmemb, (RFILE*) f) / sz;
}

bool rfio::FileInterface::fseek(Handle f, long int offset, bool relative)
{
   if (!f) return false;

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
   return !f ? false : (rfio_fendfile((RFILE*)f) > 0);
}

bool rfio::FileInterface::fflush(Handle)
{
   return true;

   // return !f ? false : ::fflush((FILE*)f)==0;
}

dabc::Object* rfio::FileInterface::fmatch(const char* fmask, bool select_files)
{
   (void) fmask;
   (void) select_files;
   return nullptr;
}

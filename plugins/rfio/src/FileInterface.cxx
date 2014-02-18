#include "rfio/FileInterface.h"

#include "rawapin.h"

dabc::FileInterface::Handle rfio::FileInterface::fopen(const char* fname, const char* mode)
{
   return (Handle) rfio_fopen((char*)fname, (char*)mode);
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

//   return false;

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

         /** Produce list of files, object must be explicitely destroyed with ref.Destroy call */
dabc::Object* rfio::FileInterface::fmatch(const char* fmask, bool select_files)
{
   return 0;
}

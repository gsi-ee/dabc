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

bool rfio::FileInterface::fwrite(Handle f, void* ptr, size_t sz)
{
   return (f==0) || (ptr==0) ? false : (rfio_fwrite((const char*)ptr, sz, 1, (RFILE*) f) == 1);
}

bool rfio::FileInterface::fread(Handle f, void* ptr, size_t sz)
{
   return (f==0) || (ptr==0) ? false : (rfio_fread((char*) ptr, sz, 1, (RFILE*) f) == 1);
}

bool rfio::FileInterface::feof(Handle f)
{
   // not implemented
   return false;

   // return f==0 ? false : feof((FILE*)f)>0;
}

bool rfio::FileInterface::fflush(Handle f)
{
   return true;

   // return f==0 ? false : ::fflush((FILE*)f)==0;
}

         /** Produce list of files, object must be explicitely destroyed with ref.Destroy call */
dabc::Object* rfio::FileInterface::fmatch(const char* fmask)
{
   return 0;
}

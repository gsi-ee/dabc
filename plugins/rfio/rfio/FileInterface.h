#ifndef RFIO_FileInterface
#define RFIO_FileInterface

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

namespace rfio {

   class FileInterface : public dabc::FileInterface {
      public:

         FileInterface() {}

         virtual ~FileInterface() {}

         virtual Handle fopen(const char* fname, const char* mode);

         virtual void fclose(Handle f);

         virtual size_t fwrite(const void* ptr, size_t sz, size_t nmemb, Handle f);

         virtual size_t fread(void* ptr, size_t sz, size_t nmemb, Handle f);

         virtual bool feof(Handle f);

         virtual bool fflush(Handle f);

         virtual bool fseek(Handle f, long int offset, bool realtive = true);

         virtual dabc::Object* fmatch(const char* fmask);
   };

}

#endif

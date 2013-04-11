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

         virtual bool fwrite(Handle f, void* ptr, size_t sz);

         virtual bool fread(Handle f, void* ptr, size_t sz);

         virtual bool feof(Handle f);

         virtual bool fflush(Handle f);

         virtual dabc::Object* fmatch(const char* fmask);
   };

}

#endif

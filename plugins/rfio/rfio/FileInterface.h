#ifndef RFIO_FileInterface
#define RFIO_FileInterface

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

namespace rfio {

   class FileInterface : public dabc::FileInterface {
      protected:
         void*  fRemote;              //! connection to datamover, done once when any special argument is appearing
         int    fOpenedCounter;       //! counter of opened files via special connection to datamover
         int    fDataMoverIndx;       //! obtained data mover index
         char   fDataMoverName[64];   //! obtained data mover name

      public:

         FileInterface();

         virtual ~FileInterface();

         virtual Handle fopen(const char *fname, const char *mode, const char *opt = nullptr);

         virtual void fclose(Handle f);

         virtual size_t fwrite(const void* ptr, size_t sz, size_t nmemb, Handle f);

         virtual size_t fread(void* ptr, size_t sz, size_t nmemb, Handle f);

         virtual bool feof(Handle f);

         virtual bool fflush(Handle f);

         virtual bool fseek(Handle f, long int offset, bool realtive = true);

         virtual dabc::Object* fmatch(const char* fmask, bool select_files = true);

         virtual int GetFileIntPar(Handle h, const char* parname);

         virtual bool GetFileStrPar(Handle h, const char* parname, char* sbuf, int sbuflen);

   };

}

#endif

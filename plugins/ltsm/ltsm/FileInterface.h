#ifndef LTSM_FileInterface
#define LTSM_FileInterface


// this switches between first version of ltsm file api (Jörg Behrendt) and new version (Thomas Stibor)
//#define LTSM_OLD_FILEAPI 1



#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif




#ifdef LTSM_OLD_FILEAPI
extern "C" {
#include "tsmfileapi.h"
}
#else
extern "C" {
#include "tsmapi.h"
}

#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif


namespace ltsm {

   class FileInterface : public dabc::FileInterface {
      protected:
//         void*  fRemote;              //! connection to datamover, done once when any special argument is appearing
//         int    fOpenedCounter;       //! counter of opened files via special connection to datamover
//         int    fDataMoverIndx;       //! obtained data mover index
//         char   fDataMoverName[64];   //! obtained data mover name

	   std::string fCurrentFile; // remember last open file, for info output
	   std::string fServername;
	   std::string fNode;
	   std::string fPassword;
	   std::string fOwner;
	   std::string fFsname;
	   std::string fDescription;


      public:

         FileInterface();

         virtual ~FileInterface();

         virtual Handle fopen(const char* fname, const char* mode, const char* opt = 0);

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

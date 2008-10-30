#ifndef MBS_MbsInputFile
#define MBS_MbsInputFile

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_FileIO
#include "dabc/FileIO.h"
#endif

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

namespace mbs {
   
   class MbsInputFile : public dabc::DataInput {
      public:
         MbsInputFile(const char* fname, 
                      const char* fileiotyp = "",
                      int nummulti = 0,
                      int firstmulti = 0);
         virtual ~MbsInputFile();
         
         bool Init();
         
         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer* buf);
         
      protected:  
         bool CloseFile();
         
         bool OpenNextFile();

         dabc::String        fFileName;
         dabc::String        fFileIOType;
         int                 fNumMultiple;
         int                 fFirstMultiple;
         
         dabc::Folder*       fFilesList;
      
         dabc::FileIO*       fIO;
         sMbsFileHeader      fHdr;
         sMbsBufferHeader    fBufHdr;
         bool                fSwapping;
         int                 fFixedBufferSize; // buffer size in old lmd file
   };
   
}

#endif

#ifndef MBS_MbsOutputFile
#define MBS_MbsOutputFile

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_FileIO
#include "dabc/FileIO.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

namespace mbs {
   
   class MbsOutputFile : public dabc::DataOutput {
      public:
         MbsOutputFile(const char* fname,
                       const char* fileiotyp = "",  // file IO type - posix, rfio and so on
                       uint64_t sizelimit = 0,      // maximum size of the file
                       int nmultiple = 0,           // number of multriple instances 0 - no, <0 - any, >0 - exact number
                       int firstmultiple = 0);      // first id of multiple files
         virtual ~MbsOutputFile();

         virtual bool WriteBuffer(dabc::Buffer* buf);
         
         const mbs::sMbsFileHeader* GetHeader() const { return &fHdr; }
         
         bool Init();
         
         bool IsAllowedMultipleFiles() const { return fNumMultiple!=0; }
      
      protected: 
      
         bool Close();
         bool StartNewFile();
      
         std::string        fFileName;
         std::string        fFileIOType;
         uint64_t            fSizeLimit;
         int                 fNumMultiple;
         int                 fFirstMultiple;
      
         int                 fCurrentFileNumber;
         uint64_t            fCurrentFileSize;
         std::string        fCurrentFileName;
         dabc::FileIO*       fIO;
         mbs::sMbsFileHeader fHdr;
         long                fSyncCounter;
         uint32_t            fBuffersCnt;
   };
}

#endif

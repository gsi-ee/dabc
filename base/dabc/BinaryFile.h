#ifndef DABC_BinaryFile
#define DABC_BinaryFile

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#include <stdint.h>

namespace dabc {

   class FileIO;

#pragma pack(1)

   typedef struct BinaryFileHeader {
      uint64_t magic;
      uint64_t version;
      uint64_t numbufs;
   };

   typedef struct BinaryFileBufHeader {
      uint64_t datalength;
      uint64_t headerlength;
      uint64_t buftype;
   };

#pragma pack(0)

   class BinaryFileInput : public DataInput {
      public:
         BinaryFileInput(FileIO* io);
         virtual ~BinaryFileInput();

         virtual bool Read_Init(Command* cmd = 0, WorkingProcessor* port = 0);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(Buffer* buf);

      protected:
         void CloseIO();

         FileIO*    fIO;
         int64_t    fVersion;     // file format version
         bool       fReadBufHeader; // indicate if we start reading of the buffer header
         BinaryFileBufHeader fBufHeader;
  };

   // _________________________________________________________________

   class BinaryFileOutput : public DataOutput {
      public:
         BinaryFileOutput(FileIO* io);
         virtual ~BinaryFileOutput();

         virtual bool Write_Init(Command* cmd = 0, WorkingProcessor* port = 0);

         virtual bool WriteBuffer(Buffer* buf);
      protected:
         void CloseIO();

         FileIO*    fIO;
         int64_t    fSyncCounter; // byte counter to perform regulary fsync operation
   };

}

#endif

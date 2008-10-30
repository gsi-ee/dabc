#include "mbs/MbsOutputFile.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"

mbs::MbsOutputFile::MbsOutputFile(const char* fname,
                                  const char* fileiotyp,
                                  uint64_t sizelimit,   
                                  int nmultiple,
                                  int firstmultiple) :  
   dabc::DataOutput(),
   fFileName(fname ? fname : ""),
   fFileIOType(fileiotyp ? fileiotyp : ""),
   fSizeLimit(sizelimit),
   fNumMultiple(nmultiple),
   fFirstMultiple(firstmultiple),
   fCurrentFileNumber(0),
   fCurrentFileSize(0),
   fCurrentFileName(),
   fIO(0),
   fHdr(),
   fSyncCounter(0),
   fBuffersCnt(0)
{
   ::memset(&fHdr, 0, sizeof(fHdr));
   
   if ((fSizeLimit>0) && (fSizeLimit<0x10000)) {
      EOUT(("File size limit too small, rise up to %u", 0x10000));
      fSizeLimit = 0x10000;
   }
}

mbs::MbsOutputFile::~MbsOutputFile()
{
   Close(); 
}

bool mbs::MbsOutputFile::Init()
{
   if (!Close()) return false;
   
   if (fNumMultiple!=0) fCurrentFileNumber = fFirstMultiple;
   
   return StartNewFile();
}
         
bool mbs::MbsOutputFile::StartNewFile()
{
   if (fIO!=0) {
      if (!IsAllowedMultipleFiles()) return false; 
       
      delete fIO; fIO = 0;
      fSyncCounter = 0;
      fCurrentFileNumber++;
      fCurrentFileSize = 0;
      fCurrentFileName = "";
      
      if ((fCurrentFileNumber>fNumMultiple) && (fNumMultiple>0)) return false;
   } 
   
   dabc::String fname;
   
   
   
   if (IsAllowedMultipleFiles())
      dabc::formats(fname, "%s_%04d.lmd", fFileName.c_str(), fCurrentFileNumber);
   else 
      fname = fFileName;

   dabc::FileIO* io = 0;
   
   if (fFileIOType.length()==0)
      io = new dabc::PosixFileIO(fname.c_str(), dabc::FileIO::Recreate);
   else
      io = dabc::Manager::Instance()->CreateFileIO(fFileIOType.c_str(), fname.c_str(), dabc::FileIO::Recreate);

   DOUT1(("Create MBS output file %s res = %s", fname.c_str(), DBOOL((io ? io->IsOk() : false))));
    
   if (io==0) return false;
   
   if (!io->IsOk() || !io->CanWrite()) {
      delete io; 
      return false;
   }
   
   fCurrentFileName = fname;
   
   fIO = io;
   
   fHdr.iType = MBS_TYPE(3000, 1);
   
   fHdr.iMaxWords = (sizeof(sMbsFileHeader) - sizeof(sMbsBufferHeader)) / 2;
   fHdr.iUsedWords = (sizeof(sMbsFileHeader) - sizeof(sMbsBufferHeader)) / 2;
   fHdr.SetEndian();

   // use -1 in length to keep always last byte as 0
   ::strncpy(fHdr.fUser.str, ::getenv("USER"), sizeof(fHdr.fUser.str)-1);
   fHdr.fUser.len = ::strlen(fHdr.fUser.str);
   
   ::snprintf(fHdr.fRun.str, sizeof(fHdr.fRun.str) - 1, "Pid %d", ::getpid());
   fHdr.fRun.len = ::strlen(fHdr.fRun.str);

   fIO->Write(&fHdr, sizeof(fHdr));
   
   fSyncCounter += sizeof(fHdr);
   
   fCurrentFileSize += sizeof(fHdr);
   
   return true;
}

bool mbs::MbsOutputFile::Close()
{
   if (fIO==0) return true;
   delete fIO; fIO = 0; 
   fSyncCounter = 0;
   fCurrentFileNumber = 0;
   fCurrentFileSize = 0;
   fCurrentFileName = "";
   return true; 
}
      
bool mbs::MbsOutputFile::WriteBuffer(dabc::Buffer* buf)
{
   if ((fIO==0) || (buf==0)) return false; 
   
   if (buf->GetTypeId() != mbs::mbt_Mbs100_1) {
      EOUT(("Buffer must contain complete MBS 100-1 buffer type %d != %d mbs::mbt_Mbs100_1", buf->GetTypeId(), mbs::mbt_Mbs100_1));
      return false; 
   }

   dabc::BufferSize_t size = buf->GetTotalSize();
   
   if ((fSizeLimit>0) && (fCurrentFileSize>sizeof(fHdr)) && 
       (fCurrentFileSize + size > fSizeLimit)) 
         if (!StartNewFile()) {
            EOUT(("Buffer cannot be written due to size limit"));
            return false;
         }
           
   if (buf->GetDataSize(0) < sizeof(sMbsBufferHeader)) {
      EOUT(("Size of first block in buffer smaller than required for header"));
      return false;
   }
   
   sMbsBufferHeader* hdr = (sMbsBufferHeader*) buf->GetDataLocation(0);
   
   hdr->iMaxWords = (size - sizeof(sMbsBufferHeader)) / 2;
   hdr->iType = MBS_TYPE(100, 1);
   hdr->iUsed = 0;
   hdr->SetEndian();
   if (hdr->iBufferId==0)
      hdr->iBufferId = fBuffersCnt++;

   hdr->iUsedWords = (size - sizeof(sMbsBufferHeader)) / 2;
   
//   DOUT1(("MBS FILE OUTPUT writes %d bytes", size));
   
   for (unsigned nseg=0; nseg<buf->NumSegments(); nseg++)
      fIO->Write(buf->GetDataLocation(nseg), buf->GetDataSize(nseg));
   
   fSyncCounter += size;
   fCurrentFileSize += size;
   
   if (fSyncCounter > 1e7) {
      fSyncCounter = 0;
      fIO->Sync();  
   }
       
   return true; 
}

#include "mbs/MbsInputFile.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/FileIO.h"
#include "dabc/Manager.h"

mbs::MbsInputFile::MbsInputFile(const char* fname,
                                const char* fileiotyp,
                                int nummulti,
                                int firstmulti) :
   dabc::DataInput(),
   fFileName(fname ? fname : ""),
   fFileIOType(fileiotyp ? fileiotyp : ""),
   fNumMultiple(nummulti),
   fFirstMultiple(firstmulti),
   fFilesList(0),
   fIO(0),
   fHdr(),
   fBufHdr(),
   fSwapping(false),
   fFixedBufferSize(0)
{
   ::memset(&fHdr, 0, sizeof(fHdr));
   ::memset(&fBufHdr, 0, sizeof(fBufHdr));
}

mbs::MbsInputFile::~MbsInputFile()
{
   CloseFile();
   if (fFilesList) {
      delete fFilesList;
      fFilesList = 0;
   }
}

bool mbs::MbsInputFile::Init()
{
   if (fFileName.length()==0) return false;

   if (fFilesList!=0) {
      EOUT(("Files list already exists"));
      return false;
   }

   if (strpbrk(fFileName.c_str(),"*?")!=0)
      fFilesList = dabc::Manager::Instance()->ListMatchFiles(fFileIOType.c_str(), fFileName.c_str());
   else
   if (fNumMultiple<0) {
      std::string mask = fFileName;
      mask += "_*.lmd";
      fFilesList = dabc::Manager::Instance()->ListMatchFiles(fFileIOType.c_str(), mask.c_str());
   } else
   if (fNumMultiple == 0) {
      fFilesList = new dabc::Folder(0,"FilesList", true);
      new dabc::Basic(fFilesList, fFileName.c_str());
   } else {
      int number = fFirstMultiple;

      fFilesList = new dabc::Folder(0,"FilesList", true);

      while (number<fNumMultiple) {
         std::string fname;
         dabc::formats(fname, "%s_%04d.lmd", fFileName.c_str(), number);
         new dabc::Basic(fFilesList, fname.c_str());
         number++;
      }
   }

   return OpenNextFile();
}

bool mbs::MbsInputFile::OpenNextFile()
{
   if (fIO!=0) {
      EOUT(("File must be close before calling OpenNextFile"));
      CloseFile();
   }

   if ((fFilesList==0) || (fFilesList->NumChilds()==0)) return false;

   const char* nextfilename = fFilesList->GetChild(0)->GetName();

   if (fFileIOType.length()==0)
      fIO = new dabc::PosixFileIO(nextfilename, dabc::FileIO::ReadOnly);
   else
      fIO = dabc::Manager::Instance()->CreateFileIO(fFileIOType.c_str(), nextfilename, dabc::FileIO::ReadOnly);

   delete fFilesList->GetChild(0);

   if (fIO==0) return false;

   if (!fIO->IsOk() || !fIO->CanRead()) {
      CloseFile();
      return false;
   }

   int sz = fIO->Read(&fHdr, sizeof(fHdr));
   if (sz!=sizeof(fHdr)) {
      CloseFile();
      return false;
   }

   fSwapping = ! fHdr.IsCorrectEndian();

   if (fSwapping) {
      fHdr.Swap();
      if (!fHdr.IsCorrectEndian()) {
         EOUT(("Cannot define endian %x != 1", fHdr.iEndian));
         CloseFile();
         return false;
      }
   }

   if (fHdr.iType == MBS_TYPE(2000, 1)) {
      fFixedBufferSize = fHdr.iMaxWords*2;
      if (fFixedBufferSize % 512 > 0) fFixedBufferSize += sizeof(sMbsBufferHeader);
      DOUT1(("Old format sz %d", fFixedBufferSize));
   } else
   if (fHdr.iType == MBS_TYPE(3000, 1)) {
      fFixedBufferSize = 0;
      DOUT1(("New format"));
   } else {
      EOUT(("Uncknown type of lmd file %x",  fHdr.iType));
      CloseFile();
      return false;
   }

   DOUT1(("Input file Type %x swapped: %s fixed size:%d", fHdr.iType, DBOOL(fSwapping), fFixedBufferSize));

   if (fFixedBufferSize > 0) {
      sz = fIO->Seek(fFixedBufferSize - sizeof(fHdr), dabc::FileIO::seek_Current);
      if (sz != fFixedBufferSize) {
         EOUT(("Cannot skip bytes in file header"));
         CloseFile();
         return false;
      }
      DOUT1(("Skip %d bytes", fFixedBufferSize - sizeof(fHdr)));
   }

   return true;
}


bool mbs::MbsInputFile::CloseFile()
{
   if (fIO) {
      delete fIO;
      fIO = 0;
   }

   fSwapping = false;

   return true;
}

unsigned mbs::MbsInputFile::Read_Size()
{
   int res;

   do {
      if (fIO==0) return di_Error;

      res = fIO->Read(&fBufHdr, sizeof(sMbsBufferHeader));

      if (res==0) {
         CloseFile();
         if (!OpenNextFile()) return di_EndOfStream;
      }
   } while (res==0);

   // this is error
   if (res!=sizeof(sMbsBufferHeader)) {
      EOUT(("Close input file while error"));
      CloseFile();
      return di_EndOfStream;
   }

   if (fSwapping) fBufHdr.Swap();

   // this is total length of the buffer, including buffer header
   int bufsize = fBufHdr.BufferLength();

   if ((fFixedBufferSize > 0) && (bufsize != fFixedBufferSize))
      EOUT(("missmatch of fixed buffer size %d and expected buffer size %d", fFixedBufferSize, bufsize));

   return bufsize;
}

unsigned mbs::MbsInputFile::Read_Complete(dabc::Buffer* buf)
{
   if ((fIO==0) || (buf==0)) return di_Error;

   unsigned bufsize = fBufHdr.BufferLength();

   if (buf->GetDataSize() < bufsize) {
      EOUT(("Cannot use buffer %d (need %d) for reading from the file", buf->GetDataSize(), bufsize));
      return di_Error;
   }

   memcpy(buf->GetDataLocation(), &fBufHdr, sizeof(sMbsBufferHeader));

   void* read_buf = (char*) buf->GetDataLocation() + sizeof(sMbsBufferHeader);
   int read_len = bufsize - sizeof(sMbsBufferHeader);

   int res = fIO->Read(read_buf, read_len);

   // this is end of file, just close it and return error
   if (res==0) {
      CloseFile();
      return di_EndOfStream;
   }

   if (res != read_len) {
      EOUT(("Error during buffer read"));
      CloseFile();
      return di_Error;
   }

   if (fSwapping)
      SwapMbsData(read_buf, read_len);

   buf->SetDataSize(fBufHdr.UsedBufferLength());

   if (fBufHdr.iType == MBS_TYPE(10,1)) buf->SetTypeId(mbt_Mbs10_1); else
   if (fBufHdr.iType == MBS_TYPE(100,1)) buf->SetTypeId(mbt_Mbs100_1); else
      EOUT(("Cannot identify Mbs buffer type"));

   return di_Ok;
}

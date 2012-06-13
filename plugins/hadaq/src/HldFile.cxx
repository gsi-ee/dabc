/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "hadaq/HldFile.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "dabc/logging.h"

// this definitions switches reading single hadtu chunks
// if disabled, we read from file full dabc buffers without header checking
#define HLD_READBUFFER_ELEMENTWISE 1



hadaq::HldFile::HldFile(): fBuffer(0)
{
   fMode = mNone;
   fLastError = HLD__SUCCESS;
   fEventCount=0;
   fBufsize=0;
}

hadaq::HldFile::~HldFile()
{
   Close();
   fLastError = HLD__SUCCESS;
}

bool hadaq::HldFile::OpenWrite(const char* fname, uint32_t buffersize)
{
   DOUT1(("Cannot open file:%s for writing - NOT IMPLEMENTED YET", fname));
   return false;
   Close();


//   fLastError = fLmdPutOpen((sLmdControl*) fControl,
//                            (char*) fname,
//                            LMD__INTERNAL_HEADER,
//                            buffersize,
//                            LMD__OVERWRITE,
//                            LMD__NO_INDEX,
//                            LMD__LARGE_FILE);
//
   //if (fLastError != HLD__SUCCESS) return false;
   fMode = mWrite;
   return true;
}

bool hadaq::HldFile::OpenRead(const char* fname, uint32_t buffersize)
{
   Close();
   fName=fname;
   if (buffersize < 1024) {
      printf("Buffer size %u too small, use 1024\n", buffersize);
      buffersize = 1024;
   }

   fFile = new std::ifstream(fname, std::ios::binary);
   if((fFile==0) || !fFile->good()) {
        delete fFile; fFile = 0;
        fMode=mError;
        DOUT1(("Eror opening file:%s for reading", fname));
        return false;
     }
   fBuffer=new char[buffersize];
   fMode = mRead;
   return true;
}

void hadaq::HldFile::Close()
{
   //std::cout<<"- hadaq::HldFile::Close()"<<std::endl;
   if (IsWriteMode()) {
      // implement close for writing here
      fLastError = HLD__FAILURE;
   } else
   if (IsReadMode()) {
      // TODO: error checking from ifstream
      delete fBuffer;fBuffer=0;
      delete fFile;fFile=0;
      fLastError = HLD__SUCCESS;
   } else
      fLastError = HLD__SUCCESS;
   fName="";
   fMode = mNone;
   fEventCount=0;
   fBufsize=0;
}

bool hadaq::HldFile::WriteElements(hadaq::Event* hdr, unsigned num)
{
   if (!IsWriteMode() || (num==0)) {
      fLastError = HLD__FAILURE;
      DOUT1(("Cannot write %d elements to file %s", num, fName.c_str()));
      return false;
   }

//   if (num==1)
//      //fLastError = fLmdPutElement((sLmdControl*) fControl, (sMbsHeader*) hdr);
//   else
      //fLastError = fLmdPutBuffer((sLmdControl*) fControl, (sMbsHeader*) hdr, num);

//   return (fLastError == HLD__SUCCESS);
   fLastError = HLD__SUCCESS;
   return true;
}

hadaq::HadTu* hadaq::HldFile::ReadElement(char* buffer, size_t len)
{
   if (!IsReadMode()) {
      fLastError = HLD__FAILURE;
      DOUT1(("Cannot read from file %s, was opened for writing", fName.c_str()));
      return 0;
   }
   char* target;
   if(buffer==0)
      {
         target=fBuffer;
         len=sizeof(fBuffer);
      }
   else
      {
        target = buffer;
      }
   // first read next event header:
   if(len<sizeof(hadaq::HadTu))
      {
         // output buffer is full, do not try to read next event.
         DOUT3(("Next hadtu header 0x%x bigger than read buffer limit 0x%x", sizeof(hadaq::HadTu), len));
         fLastError=HLD__FULLBUF;
         return 0;
      }


   ReadFile(target, sizeof(hadaq::HadTu));
   if (fLastError == HLD__EOFILE)
      {
         //Close();
         return 0;
      }
      // then read rest of buffer from file
   hadaq::HadTu* hadtu =(hadaq::HadTu*) target;
   size_t evlen=hadtu->GetSize();
   DOUT3(("ReadElement reads event of size: %d",evlen));
   if(evlen > len)
      {
        DOUT3(("Next hadtu element length 0x%x bigger than read buffer limit 0x%x", evlen, len));
        // rewind file pointer here to begin of hadtu header
        if(!RewindFile(-1 * sizeof(hadaq::HadTu)))
           {
              DOUT1(("Error on rewinding header position of file %s ", fName.c_str() ));              fLastError = HLD__FAILURE;
              //Close();
           }
        fLastError=HLD__FULLBUF;
        return 0;
      }
   ReadFile(target+sizeof(hadaq::HadTu), evlen - sizeof(hadaq::HadTu));
   if (fLastError == HLD__EOFILE)
      {
         DOUT1(("Found EOF before reading full element of length 0x%x! Maybe truncated or corrupt file?", evlen));
         //Close();
         return 0;
      }
   return (hadaq::HadTu*) target;
}

unsigned int hadaq::HldFile::ReadBuffer(void* buf, uint32_t& bufsize)
{
   if (!IsReadMode() || (buf==0) || (bufsize==0)) {
      fLastError = HLD__FAILURE;
      return 0;
   }
   unsigned int bytesread=0;
#ifdef HLD_READBUFFER_ELEMENTWISE
   // prevent event spanning over dabc buffers?
   // we do loop over complete events here
   char* cursor = (char*) buf;
   size_t rest=bufsize;
   hadaq::HadTu* thisunit=0;
   while((thisunit=ReadElement(cursor,rest))!=0)
    {
         size_t diff=thisunit->GetSize();
         cursor+=diff;
         rest-=diff;
         bytesread+=diff;
         fEventCount++;
         // FIXME: the above 2 printouts give somehow wrong shifted argument values. a bug?
         //DOUT1(("HldFile::ReadBuffer has read %d HadTu elements,  hadtu:0x%x, cursor:0x%x rest:0x%x diff:0x%x", fEventCount, (unsigned) thisunit,(unsigned) cursor, (unsigned) rest, (unsigned) diff));
         //printf("HldFile::ReadBuffer has read %d HadTu elements,  hadtu:0x%x, cursor:0x%x rest:0x%x diff:0x%x\n", fEventCount, (unsigned) thisunit,(unsigned) cursor, (unsigned) rest, (unsigned) diff);
         //std::cout<< "HldFile::ReadBuffer has read "<< fEventCount <<"elements,  hadtu:"<< (unsigned) thisunit<<", cursor:"<< (unsigned) cursor <<" rest:"<<(unsigned) rest<<" diff:"<< (unsigned) diff<<std::endl;
      }
#else
   bytesread=ReadFile( (char*) buf,bufsize);
#endif

   fBufsize+=bytesread;
     if (fLastError == HLD__EOFILE)
      {
         // FIXME: upper line will crash at dabc::format in vsnprintf JAM
         //DOUT1(("Read %d HadTu elements (%d bytes) from file %s", fEventCount, fBufsize, fName.c_str()));
         // FIXME: following line shows 0 instead fBufsize
         // DOUT1(("Read %d HadTu elements (%d bytes) from file", fEventCount, fBufsize));
          std::cout<<"Read "<< fEventCount<<" HadTu elements ("<< fBufsize << " bytes) from file "<< fName.c_str()<<std::endl;
      }

     return bytesread;
}

bool hadaq::HldFile::WriteEvents(hadaq::Event* hdr, unsigned num)
{
   return false; //WriteElements(hdr, num);
}

hadaq::Event* hadaq::HldFile::ReadEvent()
{
   return (hadaq::Event*) ReadElement();
}



size_t hadaq::HldFile::ReadFile(char* dest, size_t len)
{
   fFile->read(dest, len);
   if(fFile->eof() || !fFile->good())
         {
               fLastError = HLD__EOFILE;
               DOUT1(("End of input file %s", fName.c_str()));
               //return 0;
         }
   //cout <<"ReadFile reads "<< (hex) << fFile->gcount()<<" bytes to 0x"<< (hex) <<(int) dest<< endl;
   return (size_t) fFile->gcount();
}

bool hadaq::HldFile::RewindFile(int offset)
{
   fFile->seekg (offset, std::ios::cur);
   if(!fFile->good())
         {
               fLastError = HLD__FAILURE;
               DOUT1(("Problem rewinding file %s", fName.c_str()));
               return false;
         }
   return true;
}


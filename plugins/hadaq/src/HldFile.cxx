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


// this definitions switches to write events either separately (off)
// or to write the whole buffer after configuring event headers (on)
#define HLD_WRITEEVENTS_FAST 1



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

bool hadaq::HldFile::OpenFile(const char* fname, bool iswriting,
      uint32_t buffersize)
{
   Close();
   std::ios_base::openmode mode = std::ios::binary;
   fName = fname;
   if (iswriting) {
      mode |= std::fstream::out;
      fMode = mWrite;

   } else {
      mode |= std::fstream::in;
      fMode = mRead;
      if (buffersize < 1024) {
         printf("Aux read Buffer size %u too small, use 1024\n", buffersize);
         buffersize = 1024;
      }
      fBuffer = new char[buffersize];
   }
   fFile = new std::fstream(fname, mode);
   if ((fFile == 0) || !fFile->good()) {
      delete fFile;
      fFile = 0;
      fMode = mError;
      DOUT1("Eror opening file:%s", fname);
      return false;
   }

   return true;

}


bool hadaq::HldFile::OpenWrite(const char* fname,  hadaq::RunId rid)
{
   bool rev=true;
   Close();
   fRunNumber=rid;
   if(!OpenFile(fname, true, 0)) return false;
   // put here a dummy event into file:

   fHadEvent= new hadaq::Event;
   fHadEvent->Init(0, fRunNumber, EvtId_runStart);
   if(!WriteEvents(fHadEvent, 1)) rev = false;
   delete fHadEvent;
   return rev;
}

bool hadaq::HldFile::OpenRead(const char* fname, uint32_t buffersize)
{
   return (OpenFile(fname, false, buffersize));

//   Close();
//   fName=fname;
//   if (buffersize < 1024) {
//      printf("Buffer size %u too small, use 1024\n", buffersize);
//      buffersize = 1024;
//   }
//
//   fFile = new std::fstream(fname,  std::fstream::in | std::ios::binary);
//   if((fFile==0) || !fFile->good()) {
//        delete fFile; fFile = 0;
//        fMode=mError;
//        DOUT1("Eror opening file:%s for reading", fname));
//        return false;
//     }
//   fBuffer=new char[buffersize];
//   fMode = mRead;
//   return true;
}

void hadaq::HldFile::Close()
{
   //std::cout<<"- hadaq::HldFile::Close()"<<std::endl;
  fLastError = HLD__SUCCESS;
  if(fMode==mNone) return;
  if (IsWriteMode()) {
      // need to add empty terminating event:
      fHadEvent= new hadaq::Event;
      fHadEvent->Init(0, fRunNumber, EvtId_runStop);
      if(!WriteEvents(fHadEvent, 1)) fLastError= HLD__WRITE_ERR;
      delete fHadEvent;
   }

   // TODO: error checking from fstream
  fFile->close();
  delete fFile; fFile=0;
  delete [] fBuffer; fBuffer=0;
   fName="";
   fMode = mNone;
   fEventCount=0;
   fBufsize=0;
   fRunNumber=0;
}

bool hadaq::HldFile::WriteEvents(hadaq::Event* hdr, unsigned num)
{
   if (!IsWriteMode() || (num==0)) {
      fLastError = HLD__FAILURE;
      DOUT1("Cannot write %d elements to file %s", num, fName.c_str());
      return false;
   }
   hadaq::Event* cursor=hdr;
   while(num-- >0)
      {
         cursor->SetRunNr(fRunNumber);
            // JAM: must adjust run id to match id of this file. Otherwise, we may have queue delay effects between combiner and output
            // therefore we do not need to synchronize exactly the default runid of combiner (for online server) with file runid
         size_t elength=cursor->GetPaddedSize();
#ifndef HLD_WRITEEVENTS_FAST
         size_t written=WriteFile((char*) cursor,elength);
         if(written!=elength)
            {
               DOUT1("HldFile::WriteEvents: Write file %s length mismatch: %d bytes of %d requested written", fName.c_str(), written, elength);
               fLastError = HLD__WRITE_ERR;
               return false;
            }
#endif
         cursor = (hadaq::Event*)((char*) (cursor) + elength);
      } // while num

#ifdef HLD_WRITEEVENTS_FAST
   size_t buflength=(char*)cursor - (char*)hdr; // big buffer writes may be faster...
   size_t written=WriteFile((char*) hdr ,buflength);
   if(written!=buflength)
   {
      DOUT1("HldFile::WriteEvents: Write file %s length mismatch: %d bytes of %d requested written", fName.c_str(), written, buflength);
      fLastError = HLD__WRITE_ERR;
      return false;
   }
#endif
   fLastError = HLD__SUCCESS;
   return true;
}



unsigned int hadaq::HldFile::WriteBuffer(void* buf, uint32_t bufsize)
{
   return 0;
}

hadaq::HadTu* hadaq::HldFile::ReadElement(char* buffer, size_t len)
{
   if (!IsReadMode()) {
      fLastError = HLD__FAILURE;
      DOUT1("Cannot read from file %s, was opened for writing", fName.c_str());
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
         DOUT3("Next hadtu header 0x%x bigger than read buffer limit 0x%x", sizeof(hadaq::HadTu), len);
         fLastError=HLD__FULLBUF;
         return 0;
      }


   ReadFile(target, sizeof(hadaq::HadTu));
   if (fLastError == HLD__EOFILE)
      {
         return 0;
      }
      // then read rest of buffer from file
   hadaq::HadTu* hadtu =(hadaq::HadTu*) target;
   size_t evlen=hadtu->GetPaddedSize();
   DOUT3("ReadElement reads event of size: %d",evlen);
   if(evlen > len)
      {
        DOUT3("Next hadtu element length 0x%x bigger than read buffer limit 0x%x", evlen, len);
        // rewind file pointer here to begin of hadtu header
        if(!RewindFile(-1 * (int) sizeof(hadaq::HadTu)))
           {
              DOUT1("Error on rewinding header position of file %s ", fName.c_str());
              fLastError = HLD__FAILURE;
           }
        fLastError=HLD__FULLBUF;
        return 0;
      }
   ReadFile(target+sizeof(hadaq::HadTu), evlen - sizeof(hadaq::HadTu));
   if (fLastError == HLD__EOFILE)
      {
         DOUT1("Found EOF before reading full element of length 0x%x! Maybe truncated or corrupt file?", evlen);
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
         size_t diff=thisunit->GetPaddedSize();
         cursor+=diff;
         rest-=diff;
         bytesread+=diff;
         fEventCount++;
         // FIXME: the above 2 printouts give somehow wrong shifted argument values. a bug?
         //DOUT1("HldFile::ReadBuffer has read %d HadTu elements,  hadtu:0x%x, cursor:0x%x rest:0x%x diff:0x%x", fEventCount, (unsigned) thisunit,(unsigned) cursor, (unsigned) rest, (unsigned) diff);
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
         //DOUT1("Read %d HadTu elements (%d bytes) from file %s", fEventCount, fBufsize, fName.c_str());
         // FIXME: following line shows 0 instead fBufsize
         // DOUT1("Read %d HadTu elements (%d bytes) from file", fEventCount, fBufsize);
          std::cout<<"Read "<< fEventCount<<" HadTu elements ("<< fBufsize << " bytes) from file "<< fName.c_str()<<std::endl;
      }

     return bytesread;
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
               DOUT1("End of input file %s", fName.c_str());
               //return 0;
         }
   //cout <<"ReadFile reads "<< (hex) << fFile->gcount()<<" bytes to 0x"<< (hex) <<(int) dest<< endl;
   return (size_t) fFile->gcount();
}

size_t hadaq::HldFile::WriteFile(char* src, size_t len)
{
   std::streampos begin = fFile->tellp();
   if(begin<0)
      {
         fLastError = HLD__WRITE_ERR;
         DOUT1("Write position begin error in output file %s", fName.c_str());
         return 0;
      }
   fFile->write(src, len);
   if(!fFile->good())
         {
               fLastError = HLD__WRITE_ERR;
               DOUT1("Write Error in output file %s", fName.c_str());
               return 0;
         }
   std::streampos end = fFile->tellp();
   if(end<0)
        {
           fLastError = HLD__WRITE_ERR;
           DOUT1("Write position end error in output file %s", fName.c_str());
           return 0;
        }
   //cout <<"WriteFile writes "<< (hex) << (end-begin)<<" bytes from 0x"<< (hex) <<(int) src<< endl;
   return (size_t) (end-begin);
}


bool hadaq::HldFile::RewindFile(int offset)
{
   fFile->seekg(offset, std::ios::cur);
   if(!fFile->good()) {
      fLastError = HLD__FAILURE;
      DOUT1("Problem rewinding file %s", fName.c_str());
      return false;
   }
   return true;
}

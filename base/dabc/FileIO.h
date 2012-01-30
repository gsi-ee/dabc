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

#ifndef DABC_FileIO
#define DABC_FileIO

#include <stdint.h>

namespace dabc {

   class File; 

   class FileIO {
      
      friend class File; 
       
      public:

         enum AccessArgs { ReadOnly = 1, WriteOnly, ReadWrite, Create, Recreate };
         

         enum SeekArgs { seek_Begin = -1, seek_Current = 0, seek_End = 1 };
         
         int64_t Pos() const { return fPosition; }
      
         virtual bool IsOk() = 0;
         virtual bool CanRead() = 0;
         virtual bool CanWrite() = 0;
      
         virtual void Close() = 0;
         virtual int Read(void* buf, int len) = 0;
         virtual int Write(const void* buf, int len) = 0;
         virtual int Sync() = 0;
         /** origin = -1 from beginning, 0 - from current, 1 - from the end */
         virtual int64_t Seek(int64_t offset, SeekArgs origin) = 0;
         
         /** Indicates, if only stream mode is supported (no possibility of Seek) */
         virtual bool IsStream() = 0; 
      
         virtual int Stat(int64_t* size) = 0;
         
         virtual bool Eof();

         virtual ~FileIO() {}
      
      protected:  
         FileIO() : fPosition(0) {}
      
         int64_t  fPosition;
   };

   class PosixFileIO : public FileIO {
      public:
         PosixFileIO(const char* name, int option);
         virtual ~PosixFileIO();
         
         virtual bool IsOk()  { return _fd>=0; }
         virtual bool CanRead() { return fCanRead; }
         virtual bool CanWrite() { return fCanWrite; }
         
         virtual void Close();
         virtual int Read(void* buf, int len);
         virtual int Write(const void* buf, int len);
         virtual int64_t Seek(int64_t offset, SeekArgs origin); 
         virtual bool IsStream() { return false; }
         virtual int Sync();
         virtual int Stat(int64_t* size);
      
      protected:  
         int    _fd;
         bool   fCanRead;
         bool   fCanWrite;
   };

}

#endif

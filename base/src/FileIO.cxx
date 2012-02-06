// $Id$

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

#include "dabc/FileIO.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "dabc/logging.h"

bool dabc::FileIO::Eof()
{
   int64_t sz = -1;
   
   Stat(&sz);
   
   if (sz<=0) return true;
   
   return fPosition>=sz;
}

// __________________________________________________________________

dabc::PosixFileIO::PosixFileIO(const char* fname, int option) :
   FileIO(),
   _fd(-1),
   fCanRead(false),
   fCanWrite(false)
{
   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
   if (option==0) option = ReadOnly;
   
   int flags = O_RDONLY;
   if (option == ReadOnly) flags = O_RDONLY; else
   if (option == ReadWrite) flags = O_RDWR; else
   if (option == WriteOnly) flags = O_WRONLY; else
   if (option == Create) flags = O_RDWR | O_CREAT; else 
   if (option == Recreate) flags = O_RDWR | O_CREAT | O_TRUNC; else 
   {
      EOUT(("Unsupported option:%s, do read-only open"));
   }
   
   _fd = ::open(fname, flags, mode);
   
   if (_fd<0) {
      EOUT(("Error open file %s", fname));   
   } else {
      DOUT1(("File opened fd:%d name:%s", _fd, fname)); 
      fCanRead = (flags & O_WRONLY) == 0;
      fCanWrite = (flags & O_RDONLY) == 0;
   }
}

dabc::PosixFileIO::~PosixFileIO()
{
   ::close(_fd); 
   _fd = -1;
}

void dabc::PosixFileIO::Close()
{
   ::close(_fd); 
   _fd = -1;
}

int dabc::PosixFileIO::Read(void* buf, int len)
{
   fPosition += len; 
    
   return ::read(_fd, buf, len); 
}

int dabc::PosixFileIO::Write(const void* buf, int len)
{
   fPosition += len; 
   return ::write(_fd, buf, len); 
}

int64_t dabc::PosixFileIO::Seek(int64_t offset, SeekArgs origin)
{
    int arg = SEEK_CUR;
    if (origin==seek_Begin) arg = SEEK_SET; else
    if (origin==seek_End) arg = SEEK_END;
    
    fPosition = ::lseek(_fd, offset, arg);
    
    return fPosition;
}

int dabc::PosixFileIO::Sync()
{
   return ::fsync(_fd); 
}

int dabc::PosixFileIO::Stat(int64_t* size)
{
   struct stat info;
   
   int res = ::fstat(_fd, &info);
   
   if (size) *size = info.st_size;
   
   return res; 
}


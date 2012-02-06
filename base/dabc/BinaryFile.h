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

#ifndef DABC_BinaryFile
#define DABC_BinaryFile

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#include <stdint.h>

namespace dabc {

   class FileIO;

#pragma pack(1)

   struct BinaryFileHeader {
      uint64_t magic;
      uint64_t version;
   };

   struct BinaryFileBufHeader {
      uint64_t datalength;
      uint64_t buftype;
   };

#pragma pack(0)

   class BinaryFileInput : public DataInput {
      public:
         BinaryFileInput(FileIO* io);
         virtual ~BinaryFileInput();

         virtual bool Read_Init(const WorkerRef& wrk, const Command& cmd) { return true; }

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(Buffer& buf);

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

         virtual bool Write_Init(const WorkerRef& wrk, const Command& cmd) { return true; }

         virtual bool WriteBuffer(const Buffer& buf);
      protected:
         void CloseIO();

         FileIO*    fIO;
         int64_t    fSyncCounter; // byte counter to perform regularly fsync operation
   };

}

#endif

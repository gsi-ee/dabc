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

#include "dabc/BinaryFile.h"

#include "dabc/FileIO.h"
#include "dabc/logging.h"
#include "dabc/MemoryPool.h"
#include "dabc/Pointer.h"

#define BinaryFileCurrentVersion 1
#define BinaryFileMagicValue 1234

dabc::BinaryFileInput::BinaryFileInput(FileIO* io) :
   DataInput(),
   fIO(io),
   fVersion(0),
   fReadBufHeader(false)
{
   if ((fIO==0) || !fIO->CanRead()) { CloseIO(); return; }

   BinaryFileHeader rec;

   fIO->Read(&rec, sizeof(rec));

   if (rec.magic != BinaryFileMagicValue) {
      EOUT(("Problem with magic value"));
   }

   fVersion = rec.version;
}

dabc::BinaryFileInput::~BinaryFileInput()
{
   CloseIO();
}

void dabc::BinaryFileInput::CloseIO()
{
   if (fIO) {
      delete fIO;
      fIO = 0;
   }
}

unsigned dabc::BinaryFileInput::Read_Size()
{
   if ((fIO==0) || fReadBufHeader) return di_Error;

   unsigned res = fIO->Read(&fBufHeader, sizeof(fBufHeader));

   if (res==sizeof(fBufHeader)) {
      fReadBufHeader = true;
      return fBufHeader.datalength;
   }
   // this is indication of EOF
   if (res==0) return di_EndOfStream;

   // this is indication of error
   return di_Error;
}

unsigned dabc::BinaryFileInput::Read_Complete(Buffer& buf)
{
   if ((fIO==0) || !fReadBufHeader || buf.null()) return di_Error;

   if (buf.GetTotalSize() < fBufHeader.datalength) return di_Error;
   buf.SetTotalSize(fBufHeader.datalength);
   buf.SetTypeId(fBufHeader.buftype);

   Pointer ptr = buf;
   while (!ptr.null()) {
      fIO->Read(ptr(), ptr.rawsize());
      ptr.shift(ptr.rawsize());
   }

   fReadBufHeader = false;

   return di_Ok;
}

// _________________________________________________________________

dabc::BinaryFileOutput::BinaryFileOutput(FileIO* io) :
   DataOutput(),
   fIO(io),
   fSyncCounter(0)
{
   if ((fIO==0) || !fIO->CanWrite()) { CloseIO(); return; }

   BinaryFileHeader rec;

   rec.version = 1;
   rec.magic = BinaryFileMagicValue;

   fIO->Write(&rec, sizeof(rec));
}

dabc::BinaryFileOutput::~BinaryFileOutput()
{
   CloseIO();
}

void dabc::BinaryFileOutput::CloseIO()
{
   if (fIO) {
      delete fIO;
      fIO = 0;
   }
}

bool dabc::BinaryFileOutput::WriteBuffer(const Buffer& buf)
{
   if ((fIO==0) || buf.null()) return false;

   BinaryFileBufHeader hdr;

   hdr.buftype = buf.GetTypeId();
   hdr.datalength = buf.GetTotalSize();

   fIO->Write(&hdr, sizeof(hdr));

   Pointer ptr = buf;
   while (!ptr.null()) {
      fIO->Write(ptr(), ptr.rawsize());
      ptr.shift(ptr.rawsize());
   }

   fSyncCounter += sizeof(hdr) + hdr.datalength;

   if (fSyncCounter>1e5) {
      fIO->Sync();
      fSyncCounter = 0;
   }

   return true;
}



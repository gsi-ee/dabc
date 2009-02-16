/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/BinaryFile.h"

#include "dabc/FileIO.h"

#include "dabc/logging.h"

#include "dabc/MemoryPool.h"

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

bool dabc::BinaryFileInput::Read_Init(Command* cmd, WorkingProcessor* port)
{
   dabc::ConfigSource cfg(cmd, port);

   return false;
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
      return fBufHeader.headerlength;
   }
   // this is indication of EOF
   if (res==0) return di_EndOfStream;

   // this is indication of error
   return di_Error;
}

unsigned dabc::BinaryFileInput::Read_Complete(Buffer* buf)
{
   if ((fIO==0) || !fReadBufHeader || (buf==0)) return di_Error;

   buf->SetHeaderSize(fBufHeader.headerlength);
   if (fBufHeader.headerlength>0)
      fIO->Read(buf->GetHeader(), fBufHeader.headerlength);

   fIO->Read(buf->GetDataLocation(), fBufHeader.datalength);
   buf->SetDataSize(fBufHeader.datalength);
   buf->SetTypeId(fBufHeader.buftype);

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

bool dabc::BinaryFileOutput::Write_Init(Command* cmd, WorkingProcessor* port)
{

   dabc::ConfigSource cfg(cmd, port);

   return false;
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

bool dabc::BinaryFileOutput::WriteBuffer(Buffer* buf)
{
   if ((fIO==0) || (buf==0)) return false;

   BinaryFileBufHeader hdr;

   hdr.datalength = buf->GetTotalSize();
   hdr.headerlength = buf->GetHeaderSize();
   hdr.buftype = buf->GetTypeId();

   fIO->Write(&hdr, sizeof(hdr));
   if (buf->GetHeaderSize() > 0)
      fIO->Write(buf->GetHeader(), buf->GetHeaderSize());

   for (unsigned n=0;n<buf->NumSegments();n++)
      fIO->Write(buf->GetDataLocation(n), buf->GetDataSize(n));

   fSyncCounter += sizeof(hdr) + hdr.headerlength + hdr.datalength;

   if (fSyncCounter>1e5) {
      fIO->Sync();
      fSyncCounter = 0;
   }

   return true;
}



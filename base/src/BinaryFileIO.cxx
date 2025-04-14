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

#include "dabc/BinaryFileIO.h"

dabc::BinaryFileInput::BinaryFileInput(const dabc::Url& url) :
   FileInput(url),
   fFile(),
   fCurrentBufSize(0),
   fCurrentBufType(0)
{
}

dabc::BinaryFileInput::~BinaryFileInput()
{
   CloseFile();
}

bool dabc::BinaryFileInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileInput::Read_Init(wrk, cmd))
      return false;

   return OpenNextFile();
}

bool dabc::BinaryFileInput::OpenNextFile()
{
   CloseFile();

   if (!TakeNextFileName())
      return false;

   if (!fFile.OpenReading(CurrentFileName().c_str())) {
      EOUT("Cannot open file %s for reading", CurrentFileName().c_str());
      return false;
   }

   DOUT1("Open bin file %s for reading", CurrentFileName().c_str());

   return true;
}


bool dabc::BinaryFileInput::CloseFile()
{
   fFile.Close();
   ClearCurrentFileName();
   return true;
}

unsigned dabc::BinaryFileInput::Read_Size()
{
   if (!fFile.isReading())
      return di_Error;

   fCurrentBufSize = 0;
   fCurrentBufType = 0;

   if (fFile.eof())
      if (!OpenNextFile())
         return dabc::di_EndOfStream;

   if (!fFile.ReadBufHeader(&fCurrentBufSize, &fCurrentBufType)) {
      DOUT1("Error reading buffer header from file %s close_on_error %s", CurrentFileName().c_str(), DBOOL(fCloseOnError));
      return fCloseOnError ? di_EndOfStream : di_Error;
   }

   return fCurrentBufSize;
}

unsigned dabc::BinaryFileInput::Read_Complete(Buffer& buf)
{
   if (!fFile.isReading())
      return di_Error;

   if (buf.GetTotalSize() < fCurrentBufSize) {
      EOUT("Not enough space in buffer, required is %u", (unsigned) fCurrentBufSize);
      return di_Error;
   }

   if (buf.NumSegments() != 1) {
      EOUT(("Segmented buffer not supported - can be easily done"));
      return di_Error;
   }

   if (!fFile.ReadBufPayload(buf.SegmentPtr(), fCurrentBufSize))
      return fCloseOnError ? di_EndOfStream : di_Error;

   buf.SetTotalSize(fCurrentBufSize);
   buf.SetTypeId(fCurrentBufType);

   return di_Ok;
}

// _________________________________________________________________

dabc::BinaryFileOutput::BinaryFileOutput(const dabc::Url& url) :
   FileOutput(url,".bin"),
   fFile()
{
}

dabc::BinaryFileOutput::~BinaryFileOutput()
{
   CloseFile();
}


bool dabc::BinaryFileOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileOutput::Write_Init(wrk, cmd))
      return false;

   fFile.SetIO(fIO, false);

   return StartNewFile();
}

bool dabc::BinaryFileOutput::StartNewFile()
{
   CloseFile();

   ProduceNewFileName();

   if (!fFile.OpenWriting(CurrentFileName().c_str())) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   ShowInfo(0, dabc::format("Open %s for writing", CurrentFileName().c_str()));

   return true;
}


bool dabc::BinaryFileOutput::CloseFile()
{
   if (fFile.isWriting()) {
      ShowInfo(0, dabc::format("Close file %s", CurrentFileName().c_str()));
      fFile.Close();
   }
   return true;
}

unsigned dabc::BinaryFileOutput::Write_Buffer(Buffer& buf)
{
   if (!fFile.isWriting() || buf.null())
      return do_Error;

   BufferSize_t fullsz = buf.GetTotalSize();

   if (CheckBufferForNextFile(fullsz))
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return do_Error;
      }

   if (!fFile.WriteBufHeader(fullsz, buf.GetTypeId()))
      return do_Error;

   for (unsigned n = 0; n < buf.NumSegments(); n++)
      if (!fFile.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n)))
         return do_Error;

   AccountBuffer(fullsz);

   return do_Ok;
}

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

#include "hadaq/HldInput.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/BinaryFile.h"

#include "hadaq/HadaqTypeDefs.h"


hadaq::HldInput::HldInput(const dabc::Url& url) :
   dabc::FileInput(url),
   fFile(),
   fCurrentRead(0),
   fLastBuffer(false)
{
}

hadaq::HldInput::~HldInput()
{
   CloseFile();
}

bool hadaq::HldInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileInput::Read_Init(wrk, cmd)) return false;

   return OpenNextFile();
}

bool hadaq::HldInput::OpenNextFile()
{
   CloseFile();

   if (!TakeNextFileName()) return false;

   if (!fFile.OpenRead(CurrentFileName().c_str())) {
      EOUT("Cannot open file %s for reading, errcode:%u", CurrentFileName().c_str(), fFile.LastError());
      return false;
   }

   DOUT1("Open hld file %s for reading", CurrentFileName().c_str());

   return true;
}


bool hadaq::HldInput::CloseFile()
{
   fFile.Close();
   ClearCurrentFileName();
   fCurrentRead = 0;
   fLastBuffer=false;
   return true;
}

unsigned hadaq::HldInput::Read_Size()
{
   // get size of the buffer which should be read from the file
   if(fLastBuffer) return dabc::di_DfltBufSize;

   if (!fFile.IsReadMode())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return dabc::di_DfltBufSize;
}

unsigned hadaq::HldInput::Read_Complete(dabc::Buffer& buf)
{
   if(fLastBuffer) return dabc::di_EndOfStream;
   buf.SetTypeId(hadaq::mbt_HadaqEvents);
   uint32_t readbytes = 0;
   uint32_t filestat = HLD__SUCCESS;
   char* dest = (char*) buf.SegmentPtr(0); // TODO: read into segmented buffer
   uint32_t bufsize = buf.SegmentSize(0);
   bool nextfile = false;
   do {

      if (!fFile.IsReadMode())
         return dabc::di_Error;
      readbytes = fFile.ReadBuffer(dest, bufsize);
      fCurrentRead += readbytes;
      filestat = fFile.LastError();
      if (filestat == HLD__FULLBUF) {
         DOUT3("File %s has filled dabc buffer, readbytes:%u, bufsize: %u, allbytes: %u", fCurrentFileName.c_str(), readbytes, buf.GetTotalSize(),fCurrentRead );
         break;
      } else if (filestat == HLD__EOFILE) {
         DOUT1("File %s has EOF for buffer, readbytes:%u, bufsize %u, allbytes: %u", CurrentFileName().c_str(), readbytes, buf.GetTotalSize(),fCurrentRead);
         if (!OpenNextFile()) {
            fLastBuffer=true; // delay end of stream to still get last read contents
            break;
         }
         nextfile = true;
      }
      dest += readbytes;
      bufsize -= readbytes;
      if (bufsize == 0)
         break;
   } while (nextfile);
   buf.SetTotalSize(readbytes);
   return dabc::di_Ok;
}

hadaq::Event* hadaq::HldInput::ReadEvent()
{
   while (true) {
      if (!fFile.IsReadMode()) return 0;

      hadaq::Event* hdr = fFile.ReadEvent();
      if (hdr!=0) return hdr;

      DOUT1("File %s return 0 - end of file", CurrentFileName().c_str());
      if (!OpenNextFile()) return 0;
   }

   return 0;
}

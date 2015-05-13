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

#include "mbs/LmdInput.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/BinaryFile.h"

#include "mbs/MbsTypeDefs.h"

mbs::LmdInput::LmdInput(const dabc::Url& url) :
   dabc::FileInput(url),
   fFile()
{
   if (url.HasOption("rfio"))
      fFile.SetIO((dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface"), true);
}

mbs::LmdInput::~LmdInput()
{
   CloseFile();
}

bool mbs::LmdInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileInput::Read_Init(wrk, cmd)) return false;

   return OpenNextFile();
}

bool mbs::LmdInput::OpenNextFile()
{
   CloseFile();

   if (!TakeNextFileName()) return false;

   if (!fFile.OpenReading(CurrentFileName().c_str())) {
      EOUT("Cannot open file %s for reading", CurrentFileName().c_str());
      return false;
   }

   DOUT1("Open lmd file %s for reading", CurrentFileName().c_str());

   return true;
}


bool mbs::LmdInput::CloseFile()
{
   fFile.Close();
   ClearCurrentFileName();
   return true;
}

unsigned mbs::LmdInput::Read_Size()
{
   // get size of the buffer which should be read from the file

   if (!fFile.isReading())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return dabc::di_DfltBufSize;
}

unsigned mbs::LmdInput::Read_Complete(dabc::Buffer& buf)
{
   uint64_t bufsize = 0;

   while (true) {

       if (!fFile.isReading()) return dabc::di_Error;

       // TODO: read into segmented buffer

       bufsize = buf.SegmentSize(0);

       if (!fFile.ReadBuffer(buf.SegmentPtr(0), &bufsize)) {
          DOUT3("File %s return 0 numev for buffer %u - end of file", CurrentFileName().c_str(), buf.GetTotalSize());
          if (!OpenNextFile()) return dabc::di_EndOfStream;
       }

       if (bufsize==0) return dabc::di_Error;
       break;
   }

   // DOUT0("Read buffer of size %u total %u", (unsigned) bufsize, (unsigned) buf.SegmentSize(0));

   buf.SetTotalSize(bufsize);
   buf.SetTypeId(mbs::mbt_MbsEvents);

   return dabc::di_Ok;
}

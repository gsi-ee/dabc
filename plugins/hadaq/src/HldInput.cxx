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
   fFile()
{
   if (url.HasOption("rfio"))
     fFile.SetIO((dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface"), true);
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
      EOUT("Cannot open file %s for reading", CurrentFileName().c_str());
      return false;
   }

   DOUT1("Open hld file %s for reading", CurrentFileName().c_str());

   return true;
}


bool hadaq::HldInput::CloseFile()
{
   fFile.Close();
   ClearCurrentFileName();
   return true;
}

unsigned hadaq::HldInput::Read_Size()
{
   if (!fFile.isReading()) return dabc::di_Error;

   if (fFile.eof())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return dabc::di_DfltBufSize;
}

unsigned hadaq::HldInput::Read_Complete(dabc::Buffer& buf)
{
   if (fFile.eof()) {
      EOUT("EOF should not happen when buffer reading should be started");
      return dabc::di_Error;
   }

   // only first segment can be used for reading
   uint32_t bufsize = buf.SegmentSize(0);

   if (!fFile.ReadBuffer(buf.SegmentPtr(0), &bufsize)) {
      // if by chance reading of buffer leads to eof, skip buffer and let switch file on the next turn
      if (fFile.eof()) return dabc::di_SkipBuffer;
      CloseFile();
      return dabc::di_Error;
   }

   buf.SetTypeId(hadaq::mbt_HadaqEvents);
   buf.SetTotalSize(bufsize);
   DOUT3("HLD file read %u bytes from %s file", (unsigned) bufsize, CurrentFileName().c_str());

   return dabc::di_Ok;
}

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

#include "dogma/DogmaInput.h"

#include <cstring>
#include <cstdlib>

#include "dabc/Manager.h"

#include "dogma/TypeDefs.h"


dogma::DogmaInput::DogmaInput(const dabc::Url& url) :
   dabc::FileInput(url)
{
   if (url.HasOption("rfio"))
     fFile.SetIO((dabc::FileInterface *) dabc::mgr.CreateAny("rfio::FileInterface"), true);
   else if (url.HasOption("ltsm"))
     fFile.SetIO((dabc::FileInterface *) dabc::mgr.CreateAny("ltsm::FileInterface"), true);
}

dogma::DogmaInput::~DogmaInput()
{
   CloseFile();
}

bool dogma::DogmaInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileInput::Read_Init(wrk, cmd)) return false;

   return OpenNextFile();
}

bool dogma::DogmaInput::OpenNextFile()
{
   CloseFile();

   if (!TakeNextFileName())
      return false;

   if (!fFile.OpenRead(CurrentFileName().c_str())) {
      EOUT("Cannot open file %s for reading", CurrentFileName().c_str());
      return false;
   }

   DOUT1("Open dogma file %s for reading", CurrentFileName().c_str());

   return true;
}


bool dogma::DogmaInput::CloseFile()
{
   fFile.Close();
   ClearCurrentFileName();
   return true;
}

unsigned dogma::DogmaInput::Read_Size()
{
   if (!fFile.isReading())
      return dabc::di_Error;

   if (fFile.eof() && !OpenNextFile())
       return dabc::di_EndOfStream;

   return dabc::di_DfltBufSize;
}

unsigned dogma::DogmaInput::Read_Complete(dabc::Buffer& buf)
{
   if (fFile.eof()) {
      EOUT("EOF should not happen when buffer reading should be started");
      return dabc::di_Error;
   }

   // only first segment can be used for reading
   uint32_t bufsize = ((uint32_t) (buf.SegmentSize(0) * fReduce) / 4) * 4;

   if (!fFile.ReadBuffer(buf.SegmentPtr(0), &bufsize)) {
      // if by chance reading of buffer leads to eof, skip buffer and let switch file on the next turn
      if (fFile.eof())
         return dabc::di_SkipBuffer;
      CloseFile();
      return dabc::di_Error;
   }

   buf.SetTypeId(dogma::mbt_DogmaEvents);
   buf.SetTotalSize(bufsize);
   DOUT3("DOGMA file read %u bytes from %s file", (unsigned) bufsize, CurrentFileName().c_str());

   return dabc::di_Ok;
}

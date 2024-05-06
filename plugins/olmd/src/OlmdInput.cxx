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

#include "olmd/OlmdInput.h"

#include <cstring>
#include <cstdlib>

#include "dabc/Manager.h"

#include "mbs/MbsTypeDefs.h"

olmd::OlmdInput::OlmdInput(const dabc::Url& url) :
   dabc::FileInput(url),
   fFile()
{
//   if (url.HasOption("rfio"))
//      fFile.SetIO((dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface"), true);
//   else if (url.HasOption("ltsm"))
//	  fFile.SetIO((dabc::FileInterface*) dabc::mgr.CreateAny("ltsm::FileInterface"), true);

}

olmd::OlmdInput::~OlmdInput()
{
   CloseFile();
}

bool olmd::OlmdInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   DOUT1("OlmdInput::Read_Init");

   if (!dabc::FileInput::Read_Init(wrk, cmd)) return false;

   return OpenNextFile();
}

bool olmd::OlmdInput::OpenNextFile()
{
   DOUT1("OlmdInput::Read_Init");
   CloseFile();

   if (!TakeNextFileName()) return false;

   // JAM24 todo: set tagfile, start event, stop event, event interval from config here
   if (!fFile.OpenReading(CurrentFileName().c_str())) {
      EOUT("Cannot open file %s for reading", CurrentFileName().c_str());
      return false;
   }

   DOUT1("Open lmd file %s for reading", CurrentFileName().c_str());

   return true;
}


bool olmd::OlmdInput::CloseFile()
{
   fFile.Close();
   ClearCurrentFileName();
   return true;
}

unsigned olmd::OlmdInput::Read_Size()
{
   // get size of the buffer which should be read from the file
   if (!fFile.IsOpen())
      if (!OpenNextFile()) return dabc::di_EndOfStream;
   DOUT5("OlmdInput::Read_Size() returns default size");
   return dabc::di_DfltBufSize;
}

unsigned olmd::OlmdInput::Read_Complete(dabc::Buffer& buf)
{
   uint64_t bufsize = 0;
   buf.SetTypeId(mbs::mbt_MbsEvents);
   // TODO: read into segmented buffer

   bufsize = ((uint64_t) (buf.SegmentSize(0) * fReduce))/8*8;

   if (!fFile.ReadBuffer(buf.SegmentPtr(0), &bufsize)) {
          DOUT5("File %s return 0 numev for buffer %u - end of file", CurrentFileName().c_str(), buf.GetTotalSize());
          if (!OpenNextFile()) return dabc::di_EndOfStream;
   }

   if (bufsize == 0) return dabc::di_Error;


  DOUT5("Read_Complete Read buffer of size %u total %u", (unsigned) bufsize, (unsigned) buf.SegmentSize(0));

   buf.SetTotalSize(bufsize);


   return dabc::di_Ok;
}

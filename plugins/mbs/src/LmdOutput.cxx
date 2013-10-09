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

#include "mbs/LmdOutput.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/Parameter.h"
#include "dabc/BinaryFile.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

mbs::LmdOutput::LmdOutput(const dabc::Url& url) :
   dabc::FileOutput(url, ".lmd"),
   fFile()
{
}

mbs::LmdOutput::~LmdOutput()
{
   CloseFile();
}

bool mbs::LmdOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileOutput::Write_Init(wrk, cmd)) return false;

   return StartNewFile();
}

bool mbs::LmdOutput::StartNewFile()
{
   CloseFile();

   ProduceNewFileName();

   if (!fFile.OpenWrite(CurrentFileName().c_str(), 0)) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing, errcode %u", CurrentFileName().c_str(), fFile.LastError()));
      return false;
   }

   ShowInfo(0, dabc::format("Open %s for writing", CurrentFileName().c_str()));

   return true;
}

bool mbs::LmdOutput::CloseFile()
{
   if (fFile.IsWriteMode()) {
      ShowInfo(0, dabc::format("Close file %s", CurrentFileName().c_str()));
      fFile.Close();
   }
   return true;
}

unsigned mbs::LmdOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (!fFile.IsWriteMode() || buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   if (buf.GetTypeId() != mbs::mbt_MbsEvents) {
      ShowInfo(-1, dabc::format("Buffer must contain mbs event(s) 10-1, but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   if (buf.NumSegments()>1) {
      ShowInfo(-1, "Segmented buffer not (yet) supported");
      return dabc::do_Error;
   }

   if (CheckBufferForNextFile(buf.GetTotalSize()))
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }

   unsigned numevents = mbs::ReadIterator::NumEvents(buf);

   if (!fFile.WriteEvents((mbs::EventHeader*) buf.SegmentPtr(0), numevents)) {
      EOUT("lmd write error");
      return dabc::do_Error;
   }

   AccountBuffer(buf.GetTotalSize(), numevents);

   return dabc::do_Ok;
}

// ================================================================================

mbs::LmdOutputNew::LmdOutputNew(const dabc::Url& url) :
   dabc::FileOutput(url, ".lmd"),
   fFile()
{
}

mbs::LmdOutputNew::~LmdOutputNew()
{
   CloseFile();
}

bool mbs::LmdOutputNew::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileOutput::Write_Init(wrk, cmd)) return false;

   return StartNewFile();
}

bool mbs::LmdOutputNew::StartNewFile()
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

bool mbs::LmdOutputNew::CloseFile()
{
   if (fFile.isWriting()) {
      ShowInfo(0, dabc::format("Close file %s", CurrentFileName().c_str()));
      fFile.Close();
   }
   return true;
}

unsigned mbs::LmdOutputNew::Write_Buffer(dabc::Buffer& buf)
{
   if (!fFile.isWriting() || buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   if (buf.GetTypeId() != mbs::mbt_MbsEvents) {
      ShowInfo(-1, dabc::format("Buffer must contain mbs event(s) 10-1, but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   if (buf.NumSegments()>1) {
      ShowInfo(-1, "Segmented buffer not (yet) supported");
      return dabc::do_Error;
   }

   if (CheckBufferForNextFile(buf.GetTotalSize()))
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }

   unsigned numevents = mbs::ReadIterator::NumEvents(buf);

   for (unsigned n=0;n<buf.NumSegments();n++)
      if (!fFile.WriteBuffer(buf.SegmentPtr(n), buf.SegmentSize(n))) {
         EOUT("lmd write error");
         return dabc::do_Error;
      }

   AccountBuffer(buf.GetTotalSize(), numevents);

   return dabc::do_Ok;
}


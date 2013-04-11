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

#include "hadaq/HldOutput.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/Parameter.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"


hadaq::HldOutput::HldOutput(const dabc::Url& url) :
   dabc::FileOutput(url,".hld"),
   fEpicsControl(false),
   fRunNumber(0),
   fFile()
{
}

hadaq::HldOutput::~HldOutput()
{
   CloseFile();
}

bool hadaq::HldOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileOutput::Write_Init(wrk, cmd)) return false;

   fRunidPar = dabc::mgr.FindPar("Combiner/Evtbuild_runId");
   if(fRunidPar.null())
      EOUT("HldOutput::Write_Init did not find runid parameter");

   fBytesWrittenPar = dabc::mgr.FindPar("Combiner/Evtbuild_bytesWritten");
   if(fBytesWrittenPar.null())
      EOUT("HldOutput::Write_Init did not find written bytes parameter");

   fEpicsControl = wrk.Cfg(hadaq::xmlExternalRunid, cmd).AsBool(fEpicsControl);
   if (fEpicsControl) {
      fRunNumber = GetRunId();
      ShowInfo(dabc::format("EPICS control is enabled, first runid:%d",fRunNumber), true);
   }

   return StartNewFile();
}


hadaq::RunId hadaq::HldOutput::GetRunId()
{
   hadaq::RunId nextrunid =0;
   unsigned counter=0;
   do{
      nextrunid=fRunidPar.AsUInt();
      if(nextrunid) break;
      dabc::Sleep(0.1);
      counter++;
      if(counter>100) {
         EOUT("HldOutput could not get run id from EPICS master within 10s. Use self generated id. Disable epics runid control.");
         nextrunid = hadaq::Event::CreateRunId(); // TODO: correct error handling here, shall we terminate instead?
         fEpicsControl=false;
      }
   } while (nextrunid==0);
   return nextrunid;

}


bool hadaq::HldOutput::StartNewFile()
{
   CloseFile();
   // new file will change run id for complete system:

   if (!fEpicsControl || fRunNumber == 0) {
      fRunNumber = hadaq::Event::CreateRunId();
      DOUT0("HldOutput Generates New Runid %d ", fRunNumber);
      fRunidPar.SetUInt(fRunNumber);
   }

   ProduceNewFileName();

   if (!fFile.OpenWrite(CurrentFileName().c_str(), fRunNumber)) {
      ShowError(dabc::format("%s cannot open file for writing, errcode %u", CurrentFileName().c_str(), fFile.LastError()));
      return false;
   }

   ShowInfo(dabc::format("%s open for writing", CurrentFileName().c_str()), true);

   return true;
}

bool hadaq::HldOutput::CloseFile()
{
   if (fFile.IsWriteMode()) {
      ShowInfo("HLD file is CLOSED", true);
   }
   fFile.Close();
   return true;
}

unsigned hadaq::HldOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (!fFile.IsWriteMode() || buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   if (buf.GetTypeId() != hadaq::mbt_HadaqEvents) {
      ShowError(dabc::format("Buffer must contain hadaq event(s), but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   if (buf.NumSegments()>1) {
      ShowError("Segmented buffer not (yet) supported");
      return dabc::do_Error;
   }

   bool startnewfile = CheckBufferForNextFile(buf.GetTotalSize());
   if (fEpicsControl) {
      // check if EPICS master has assigned a new run for us:
      RunId nextrunid = GetRunId();
      if (nextrunid > fRunNumber) {
         fRunNumber = nextrunid;
         startnewfile = true;
         DOUT0("HldOutput Gets New Runid %d from EPICS", fRunNumber);
      }
   }

   if(startnewfile)
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }

   unsigned numevents = hadaq::ReadIterator::NumEvents(buf);

   DOUT3("Write %u events to hld file", numevents);

   fBytesWrittenPar.SetUInt(fCurrentFileSize);

   if (!fFile.WriteEvents((hadaq::Event*) buf.SegmentPtr(0), numevents)) return dabc::do_Error;

   AccountBuffer(buf.GetTotalSize(), numevents);

   return dabc::do_Ok;
}

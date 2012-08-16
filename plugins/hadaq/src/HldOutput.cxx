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


hadaq::HldOutput::HldOutput(const char* fname,
                          unsigned sizelimit_mb) :
   dabc::DataOutput(),
   fFileName(fname ? fname : ""),
   fInfoName(),
   fInfoTime(),
   fSizeLimit(sizelimit_mb),
   fEpicsControl(false),
   fRunNumber(0),
   fCurrentFileName(),
   fFile(),
   fCurrentSize(0),
   fTotalSize(0),
   fTotalEvents(0)
{
}

hadaq::HldOutput::~HldOutput()
{
   Close();
}

bool hadaq::HldOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   fFileName = wrk.Cfg(hadaq::xmlFileName, cmd).AsStdStr(fFileName);
   fSizeLimit = wrk.Cfg(hadaq::xmlSizeLimit, cmd).AsInt(fSizeLimit);
   fInfoName = wrk.Cfg("InfoPar", cmd).AsStdStr();
   fInfoTime.GetNow();
   fRunidPar=dabc::mgr.FindPar("Combiner/Evtbuild_runId");
   if(fRunidPar.null())
       EOUT(("HldOutput::Write_Init did not find runid parameter"));

   fBytesWrittenPar=dabc::mgr.FindPar("Combiner/Evtbuild_bytesWritten");
   if(fBytesWrittenPar.null())
       EOUT(("HldOutput::Write_Init did not find written bytes parameter"));

   fEpicsControl=wrk.Cfg(hadaq::xmlExternalRunid, cmd).AsBool(fEpicsControl);
   if (fEpicsControl) {
         fRunNumber = GetRunId();
         ShowInfo(dabc::format("EPICS control is enabled, first runid:%d",fRunNumber));

   }
   else
      {
      ShowInfo(dabc::format("Set file size limit:%llu",fSizeLimit));
   }
   return Init();
}

/** Method to display information from the file output
 *  Parameter priority has following meaning:
 *   priority = 0  normal message, will be updated every second
 *   priority = 1  normal message will be displayed in any case
 *   priority = 2  error message will be displayed in any case
 */

void hadaq::HldOutput::ShowInfo(const std::string& info, int priority)
{
   if ((priority==0) && !fInfoTime.Expired(1.)) return;

   fInfoTime.GetNow();

   if (fInfoName.empty()) {
      switch (priority) {
         case 0:
         case 1:
            DOUT1((info.c_str()));
            break;
         case 2:
            EOUT((info.c_str()));
            break;
      }
   } else {
      dabc::Parameter par = dabc::mgr.FindPar(fInfoName);
      par.SetStr(info);
      if (priority>0) par.FireModified();
   }

}


std::string hadaq::HldOutput::FullFileName(std::string extens)
{
   std::string fname = fFileName;

   size_t len = fname.length();
   size_t pos = fname.rfind(".hld");
   if (pos==std::string::npos)
      pos = fname.rfind(".HLD");

   if (pos==len-4)
      fname.insert(pos, extens);
   else {
      fname += extens;
      fname += ".hld";
   }

   return fname;
}

bool hadaq::HldOutput::Init()
{
   if (!Close()) return false;

   if (fSizeLimit>0) fSizeLimit *= 1000000;
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
         if(counter>100)
            {
               EOUT(("HldOutput could not get run id from EPICS master within 10s. Use self generated id. Disable epics runid control."));
               nextrunid= hadaq::Event::CreateRunId(); // TODO: correct error handling here, shall we terminate instead?
               fEpicsControl=false;
            }
      } while (nextrunid==0);
      return nextrunid;

}


bool hadaq::HldOutput::StartNewFile()
{
       Close();
   // new file will change run id for complete system:
       if(fFileName.empty())
       {
          // do not open any file if user left filename blank
          DOUT1(("HldOutput with empty filename - disabled output!"));
          return true; // otherwise init of dabc will fail completely
       }

      if (!fEpicsControl || fRunNumber == 0) {
      fRunNumber = hadaq::Event::CreateRunId();
      DOUT0(("HldOutput Generates New Runid %d ", fRunNumber));
      fRunidPar.SetUInt(fRunNumber);
   }

   std::string fname = FullFileName(hadaq::Event::FormatFilename (fRunNumber));

   if (!fFile.OpenWrite(fname.c_str(), fRunNumber)) {
      ShowInfo(dabc::format("%s cannot open file for writing, errcode %u", fname.c_str(), fFile.LastError()), 2);
      return false;
   }

   fCurrentFileName = fname;
   fCurrentSize = 0;

   ShowInfo(dabc::format("%s open for writing", fCurrentFileName.c_str()), 1);


   return true;
}

bool hadaq::HldOutput::Close()
{
   if (fFile.IsWriteMode())
      {
         ShowWriteInfo();
         ShowInfo("------ File is CLOSED", 1);
      }
   fFile.Close();
   //fCurrentFileNumber = 0;
   fCurrentFileName = "";
   fCurrentSize = 0;
   return true;
}

bool hadaq::HldOutput::WriteBuffer(const dabc::Buffer& buf)
{
   if (!fFile.IsWriteMode() || buf.null()) return false;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      Close();
      return false;
   }

   if (buf.GetTypeId() != hadaq::mbt_HadaqEvents) {
      ShowInfo(dabc::format("Buffer must contain hadaq event(s), but has type %u", buf.GetTypeId()), 2);
      return false;
   }

   if (buf.NumSegments()>1) {
      ShowInfo("Segmented buffer not (yet) supported", 2);
      return false;
   }

   bool startnewfile = false;
   if (fEpicsControl) {
      // check if EPICS master has assigned a new run for us:
      RunId nextrunid = GetRunId();
      if (nextrunid > fRunNumber) {
         fRunNumber = nextrunid;
         startnewfile = true;
         DOUT0(("HldOutput Gets New Runid %d from EPICS", fRunNumber));
      }
   } else {
      // no EPICS control, use local size limits:
      if ((fSizeLimit > 0) && (fCurrentSize + buf.GetTotalSize() > fSizeLimit))
         startnewfile = true;
   }


   if(startnewfile)
   {
        if (!StartNewFile()) {
         EOUT(("Cannot start new file for writing"));
         return false;
      }
   }


   unsigned numevents = hadaq::ReadIterator::NumEvents(buf);
   DOUT3(("Write %u events to hld file", numevents));


   fCurrentSize += buf.GetTotalSize();
   fBytesWrittenPar.SetUInt(fCurrentSize);
   fTotalSize += buf.GetTotalSize();
   fTotalEvents += numevents;
   ShowWriteInfo();
   return fFile.WriteEvents((hadaq::Event*) buf.SegmentPtr(0), numevents);
}


void  hadaq::HldOutput::ShowWriteInfo()
{
   // this is a copy of lmd output info display. useful for us too
   std::string info = fCurrentFileName;
   size_t pos = info.rfind("/");
   if (pos != std::string::npos)
      info.erase(0, pos);

   if (fTotalSize < 1024 * 1024)
      info += dabc::format(" %3u kB", (unsigned) (fTotalSize / 1024));
   else if (fTotalSize < 1024 * 1024 * 1024)
      info += dabc::format(" %3.1f MB", fTotalSize / 1024. / 1024.);
   else
      info += dabc::format(" %4.2f GB", fTotalSize / 1024. / 1024. / 1024.);



   if (fTotalEvents < 1000000)
      info += dabc::format(" %u events", (unsigned) fTotalEvents);
   else
      info += dabc::format(" %8.3e events", fTotalEvents * 1.);

   if (fSizeLimit > 0)
      info += dabc::format(" (%3.1f %s)", 100. / fSizeLimit * fCurrentSize,
            "%");

   ShowInfo(info);

}


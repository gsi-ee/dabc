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
   //fCurrentFileNumber(0),
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

   ShowInfo(dabc::format("Set file size limit:%llu", fSizeLimit));

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

bool hadaq::HldOutput::StartNewFile()
{
   //if (fFile.IsWriteMode()) {
       Close();
   //}
   // new file will change run id for complete system:
   RunId runNumber= hadaq::Event::CreateRunId();
   dabc::Parameter par = dabc::mgr.FindPar("RunId");
   par.SetInt(runNumber);
   par.FireModified(); // inform eventbuilder about new runid: NOTE for exact sync we overwrite runid anyway. This one is only for some online monitor consistency...


   std::string fname = FullFileName(hadaq::Event::FormatFilename (runNumber));

   if (!fFile.OpenWrite(fname.c_str(), runNumber)) {
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

   if ((fSizeLimit > 0) && (fCurrentSize + buf.GetTotalSize() > fSizeLimit))
      if (!StartNewFile()) {
         EOUT(("Cannot start new file for writing"));
         return false;
      }

   unsigned numevents = hadaq::ReadIterator::NumEvents(buf);
   DOUT3(("Write %u events to hld file", numevents));


   fCurrentSize += buf.GetTotalSize();
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


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

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

mbs::LmdOutput::LmdOutput(const char* fname,
                          unsigned sizelimit_mb) :
   dabc::DataOutput(),
   fFileName(fname ? fname : ""),
   fInfoName(),
   fInfoTime(),
   fSizeLimit(sizelimit_mb),
   fCurrentFileNumber(0),
   fCurrentFileName(),
   fFile(),
   fCurrentSize(0),
   fTotalSize(0),
   fTotalEvents(0)
{
}

mbs::LmdOutput::~LmdOutput()
{
   Close();
}

bool mbs::LmdOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   fFileName = wrk.Cfg(mbs::xmlFileName, cmd).AsStdStr(fFileName);
   fSizeLimit = wrk.Cfg(mbs::xmlSizeLimit, cmd).AsInt(fSizeLimit);
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

void mbs::LmdOutput::ShowInfo(const std::string& info, int priority)
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


std::string mbs::LmdOutput::FullFileName(std::string extens)
{
   std::string fname = fFileName;

   size_t len = fname.length();
   size_t pos = fname.rfind(".lmd");
   if (pos==std::string::npos)
      pos = fname.rfind(".LMD");

   if (pos==len-4)
      fname.insert(pos, extens);
   else {
      fname += extens;
      fname += ".lmd";
   }

   return fname;
}

bool mbs::LmdOutput::Init()
{
   if (!Close()) return false;

   if (fSizeLimit>0) fSizeLimit *= 1000000;

   fCurrentFileNumber = 0;

   std::string mask = FullFileName("_*");

   dabc::Object *lst = dabc::mgr()->ListMatchFiles("", mask.c_str());

   if (lst!=0) {
      int maxnumber = -1;

      unsigned cnt = 0;

      while (cnt < lst->NumChilds()) {
         std::string fname = lst->GetChild(cnt++)->GetName();
         fname.erase(fname.length()-4, 4);
         size_t pos = fname.length() - 1;
         while ((fname[pos]>='0') && (fname[pos] <= '9')) pos--;
         fname.erase(0, pos+1);

         while ((fname.length()>1) && fname[0]=='0') fname.erase(0, 1);

         if (fname.length()>0) {
            int number = atoi(fname.c_str());
            if (number>maxnumber) maxnumber = number;
         }
      }

      if (fCurrentFileNumber <= maxnumber)
         fCurrentFileNumber = maxnumber + 1;

      ShowInfo(dabc::format("start with file number %d", fCurrentFileNumber));

      dabc::Object::Destroy(lst);
   }

   return StartNewFile();
}

bool mbs::LmdOutput::StartNewFile()
{
   if (fFile.IsWriteMode()) {
       fFile.Close();
       fCurrentFileName = "";
       fCurrentSize = 0;
       fCurrentFileNumber++;
   }

   std::string fname = FullFileName(dabc::format("_%04d", fCurrentFileNumber));

   if (!fFile.OpenWrite(fname.c_str(), 0)) {
      ShowInfo(dabc::format("%s cannot open file for writing, errcode %u", fname.c_str(), fFile.LastError()), 2);
      return false;
   }

   fCurrentFileName = fname;
   fCurrentSize = 0;

   ShowInfo(dabc::format("%s open for writing", fCurrentFileName.c_str()), 1);

   return true;
}

bool mbs::LmdOutput::Close()
{
   if (fFile.IsWriteMode())
      ShowInfo("File closed", 1);

   fFile.Close();
   fCurrentFileNumber = 0;
   fCurrentFileName = "";
   fCurrentSize = 0;
   return true;
}

bool mbs::LmdOutput::WriteBuffer(const dabc::Buffer& buf)
{
   if (!fFile.IsWriteMode() || buf.null()) return false;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      Close();
      return false;
   }

   if (buf.GetTypeId() != mbs::mbt_MbsEvents) {
      ShowInfo(dabc::format("Buffer must contain mbs event(s) 10-1, but has type %u", buf.GetTypeId()), 2);
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

   unsigned numevents = mbs::ReadIterator::NumEvents(buf);

   DOUT4(("Write %u events to lmd file", numevents));

   fCurrentSize += buf.GetTotalSize();
   fTotalSize += buf.GetTotalSize();

   std::string info = fCurrentFileName;
   size_t pos = info.rfind("/");
   if (pos!=std::string::npos) info.erase(0, pos);

   if (fTotalSize<1024*1024)
      info+=dabc::format(" %3u kB", (unsigned) (fTotalSize/1024));
   else if (fTotalSize<1024*1024*1024)
      info+=dabc::format(" %3.1f MB", fTotalSize/1024./1024.);
   else
      info+=dabc::format(" %4.2f GB", fTotalSize/1024./1024./1024.);

   fTotalEvents += numevents;

   if (fTotalEvents<1000000)
      info+=dabc::format(" %u events", (unsigned) fTotalEvents);
   else
      info+=dabc::format(" %8.3e events", fTotalEvents*1.);

   if (fSizeLimit>0)
      info += dabc::format(" (%3.1f %s)", 100./fSizeLimit*fCurrentSize, "%");

   ShowInfo(info);

   return fFile.WriteEvents((mbs::EventHeader*) buf.SegmentPtr(0), numevents);
}

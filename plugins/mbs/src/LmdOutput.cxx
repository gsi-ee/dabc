/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "mbs/LmdOutput.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

mbs::LmdOutput::LmdOutput(const char* fname,
                          unsigned sizelimit_mb) :
   dabc::DataOutput(),
   fFileName(fname ? fname : ""),
   fSizeLimit(sizelimit_mb),
   fCurrentFileNumber(0),
   fCurrentFileName(),
   fFile(),
   fCurrentSize(0),
   fInfoPort(0),
   fInfoPrefix(),
   fLastInfoTime(dabc::NullTimeStamp)
{
}

mbs::LmdOutput::~LmdOutput()
{
   Close();

   // destroy info parameter
   if (fInfoPort!=0) {
      dabc::Command* cmd = new dabc::Command("DestroyParameter");
      cmd->SetStr("ParName", "LmdFileInfo");
      fInfoPort->Execute(cmd);
      fInfoPort = 0;
   }
}

bool mbs::LmdOutput::Write_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
   dabc::ConfigSource cfg(cmd, port);

   fFileName = cfg.GetCfgStr(mbs::xmlFileName, fFileName);
   fSizeLimit = cfg.GetCfgInt(mbs::xmlSizeLimit, fSizeLimit);
   bool showinfo = (port!=0) && cfg.GetCfgBool(dabc::xmlShowInfo, true);

   if (showinfo) {
      dabc::Command* cmd = new dabc::Command("CreateInfoParameter");
      cmd->SetStr("ParName", "LmdFileInfo");
      if (port->Execute(cmd)==dabc::cmd_true) fInfoPort = port;

      fInfoPrefix = fFileName;
            // dabc::format("Port %s", port->GetFullName(dabc::mgr()).c_str());
   }


   ShowInfo(FORMAT(("limit:%llu", fSizeLimit)));

   return Init();
}

void mbs::LmdOutput::ShowInfo(const char* info, int force)
{
   if (force==1) DOUT2((info)); else
   if (force==2) EOUT((info));

   if (fInfoPort) {
      dabc::TimeStamp_t tm = TimeStamp();

      if ((force>0) || dabc::IsNullTime(fLastInfoTime) || (dabc::TimeDistance(fLastInfoTime, tm) > 5.)) {
         fLastInfoTime = tm;

         std::string s = fInfoPrefix;
         s += ": ";
         s += info;

         dabc::Command* cmd = new dabc::CmdSetParameter("LmdFileInfo", s.c_str());
         fInfoPort->Submit(cmd);
      }
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

   dabc::Folder *lst = dabc::Manager::Instance()->ListMatchFiles("", mask.c_str());

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

      ShowInfo(FORMAT(("start with file number %d", fCurrentFileNumber)));

      delete lst;
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

   fInfoPrefix = fname;

   if (!fFile.OpenWrite(fname.c_str(), 0)) {
      ShowInfo(FORMAT(("cannot open file for writing, errcode %u", fFile.LastError())), 2);
      return false;
   }

   ShowInfo("create for writing");

   fCurrentFileName = fname;
   fCurrentSize = 0;

   return true;
}

bool mbs::LmdOutput::Close()
{
   if (fFile.IsWriteMode())
      ShowInfo("Close file");

   fInfoPrefix = fFileName;

   fFile.Close();
   fCurrentFileNumber = 0;
   fCurrentFileName = "";
   fCurrentSize = 0;
   return true;
}

bool mbs::LmdOutput::WriteBuffer(dabc::Buffer* buf)
{
   if (!fFile.IsWriteMode() || (buf==0)) return false;

   if (buf->GetTypeId() == dabc::mbt_EOF) {
      Close();
      return false;
   }

   if (buf->GetTypeId() != mbs::mbt_MbsEvents) {
      ShowInfo(FORMAT(("Buffer must contain mbs event(s) 10-1, but has type %u", buf->GetTypeId())), 2);
      return false;
   }

   if (buf->NumSegments()>1) {
      EOUT(("Segmented buffer not (yet) supported"));
      return false;
   }

   if ((fSizeLimit > 0) && (fCurrentSize + buf->GetTotalSize() > fSizeLimit))
      if (!StartNewFile()) {
         EOUT(("Cannot start new file for writing"));
         return false;
      }

   unsigned numevents = mbs::ReadIterator::NumEvents(buf);

   DOUT4(("Write %u events to lmd file", numevents));

   fCurrentSize += buf->GetTotalSize();

   std::string info;
   info = dabc::format("written %3.1f MB", fCurrentSize/1024./1024.);
   if (fSizeLimit>0)
      info += dabc::format(" (%3.1f %s)", 100./fSizeLimit*fCurrentSize, "%");

   ShowInfo(info.c_str(), 0);

//   ShowInfo(FORMAT(("Info anything %lld", fCurrentSize)), 0);

//   return true;

   return fFile.WriteEvents((mbs::EventHeader*) buf->GetDataLocation(), numevents);
}

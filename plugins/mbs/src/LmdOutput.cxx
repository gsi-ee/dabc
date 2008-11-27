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
   fCurrentSize(0)
{
}

mbs::LmdOutput::~LmdOutput()
{
   Close();
}

bool mbs::LmdOutput::Write_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
   dabc::ConfigSource cfg(cmd, port);

   fFileName = cfg.GetCfgStr(mbs::xmlFileName, fFileName);
   fSizeLimit = cfg.GetCfgInt(mbs::xmlSizeLimit, fSizeLimit);

   DOUT1(("Create LmdOutput name:%s limit:%llu", fFileName.c_str(), fSizeLimit));

   return Init();
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

      DOUT1(("Find file %s with maximum number %d, start with %d", mask.c_str(), maxnumber, fCurrentFileNumber));

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

   if (!fFile.OpenWrite(fname.c_str(), 0)) {
       EOUT(("Cannot open file %s for writing, errcode %u", fname.c_str(), fFile.LastError()));
       return false;
   }

   fCurrentFileName = fname;
   fCurrentSize = 0;

   return true;
}

bool mbs::LmdOutput::Close()
{
   if (fFile.IsWriteMode())
      DOUT1(("Close output lmd file %s", fCurrentFileName.c_str()));

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
      EOUT(("Buffer must contain mbs event(s) 10-1, but has type %u", buf->GetTypeId()));
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

   return fFile.WriteEvents((mbs::EventHeader*) buf->GetDataLocation(), numevents);
}

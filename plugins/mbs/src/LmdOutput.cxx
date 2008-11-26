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

mbs::LmdOutput::LmdOutput(const char* fname,
                          unsigned sizelimit_mb,
                          int nmultiple,
                          int firstmultiple) :
   dabc::DataOutput(),
   fFileName(fname ? fname : ""),
   fSizeLimit(sizelimit_mb),
   fNumMultiple(nmultiple),
   fFirstMultiple(firstmultiple),
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
   fNumMultiple = cfg.GetCfgInt(mbs::xmlNumMultiple, fNumMultiple);
   fFirstMultiple = cfg.GetCfgInt(mbs::xmlFirstMultiple, fFirstMultiple);

   return Init();
}

bool mbs::LmdOutput::Init()
{
   if (!Close()) return false;

   if (fSizeLimit>0) fSizeLimit *= 1000000;


   if (fNumMultiple!=0) {
      fCurrentFileNumber = fFirstMultiple;

      std::string mask = fFileName;

      size_t pos = mask.rfind(".lmd");
      if (pos==std::string::npos)
         pos = mask.rfind(".LMD");

      if (pos==mask.length()-4)
         mask.insert(pos, "_*");
      else
         mask += "_*.lmd";

      dabc::Folder *lst = dabc::Manager::Instance()->ListMatchFiles("", mask.c_str());

      if (lst!=0) {
         int maxnumber = -1;

         while (lst->NumChilds() > 0) {
            std::string fname = lst->GetChild(0)->GetName();
            fname.erase(fname.length()-4, 4);
            pos = fname.length() - 1;
            while ((fname[pos]>='0') && (fname[pos] <= '9')) pos--;
            fname.erase(0, pos+1);

            while ((fname.length()>1) && fname[0]=='0') fname.erase(0, 1);

            if (fname.length()>0) {
               int number = atoi(fname.c_str());
               if (number>maxnumber) maxnumber = number;
            }

            delete lst->GetChild(0);
         }

         if (fCurrentFileNumber <= maxnumber)
            fCurrentFileNumber = maxnumber + 1;

         DOUT1(("Find file %s with maximum number %d, start with %d", mask.c_str(), maxnumber, fCurrentFileNumber));

         delete lst;
      }
   }

   return StartNewFile();
}

bool mbs::LmdOutput::StartNewFile()
{
   if (fFile.IsWriteMode()) {
       fFile.Close();
       fCurrentFileName = "";
       fCurrentSize = 0;
       if (!IsAllowedMultipleFiles()) return false;

       fCurrentFileNumber++;
       if ((fCurrentFileNumber>fNumMultiple) && (fNumMultiple>0)) return false;
   }

   std::string fname = fFileName;

   if (IsAllowedMultipleFiles()) {
      std::string extens;
      dabc::formats(extens, "_%04d", fCurrentFileNumber);

      unsigned len = fname.length();

      size_t pos = fname.rfind(".lmd");
      if (pos==std::string::npos)
         pos = fname.rfind(".LMD");

      if (pos==len-4)
         fname.insert(pos, extens);
      else {
         fname += extens;
         fname += ".lmd";
      }
   }

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


   if (buf->GetTypeId() != mbs::mbt_MbsEvs10_1) {
      EOUT(("Buffer must contain mbs event(s) 10-1, but has type %u", buf->GetTypeId()));
      return false;
   }

   if (buf->NumSegments()>1) {
      EOUT(("Segemented buffer not (yet) supported"));
      return false;
   }

   if (fSizeLimit > 0)
      if (fCurrentSize + buf->GetTotalSize() > fSizeLimit)
        if (!IsAllowedMultipleFiles()) {
           EOUT(("Size limit is reached, no more data can be written !!!"));
           return false;
        } else
        if (!StartNewFile()) {
           EOUT(("Cannot start new file for writing"));
           return false;
        }

   unsigned numevents = 0;

   if (buf->GetTypeId() == mbs::mbt_MbsEvs10_1) {
      dabc::BufferSize_t size = buf->GetTotalSize();
      mbs::EventHeader* hdr = (mbs::EventHeader*) buf->GetDataLocation();

      DOUT5(("WriteBuffer of size %u", size));

      while (size>0) {

         DOUT5(("Next event id = %u sz = %u",  hdr->EventNumber(), hdr->FullSize()));

         numevents++;
         size-= hdr->FullSize();
         hdr = (mbs::EventHeader*)((char*) hdr + hdr->FullSize());
      }
   } else
      numevents = 1;

   DOUT4(("Write %u events to lmd file", numevents));

   fCurrentSize += buf->GetTotalSize();

   return fFile.WriteEvents((mbs::EventHeader*) buf->GetDataLocation(), numevents);
}

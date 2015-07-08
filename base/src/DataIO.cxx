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

#include "dabc/DataIO.h"

#include "dabc/Buffer.h"

#include "dabc/Manager.h"

#include "dabc/BinaryFile.h"


dabc::Buffer dabc::DataInput::ReadBuffer()
{
   unsigned sz = Read_Size();

   dabc::Buffer buf;

   if (sz == di_DfltBufSize) sz = 0x10000;

   if (sz>di_ValidSize) return buf;

   buf = dabc::Buffer::CreateBuffer(sz);

   if (buf.null()) return buf;

   if (Read_Start(buf) != di_Ok) {
      buf.Release();
      return buf;
   }

   if (Read_Complete(buf) != di_Ok) {
      buf.Release();
      return buf;
   }

   return buf;
}

// ======================================================

dabc::DataOutput::DataOutput(const dabc::Url& url) :
   fInfoName()
{
}

void dabc::DataOutput::SetInfoParName(const std::string& name)
{
   fInfoName = name;
}


void dabc::DataOutput::ShowInfo(int lvl, const std::string& info)
{
   dabc::InfoParameter par;
   if (!fInfoName.empty())
      par = dabc::mgr.FindPar(fInfoName);

   if (par.null()) {
      if (lvl<0)
         EOUT(info.c_str());
      else
         DOUT1(info.c_str());
   } else {
      par.SetValue(info);
      par.FireModified();
   }
}


bool dabc::DataOutput::WriteBuffer(Buffer& buf)
{
   if (Write_Check() != do_Ok) return false;

   if (Write_Buffer(buf) != do_Ok) return false;

   return Write_Complete() == do_Ok;
}


// ========================================================


dabc::FileInput::FileInput(const dabc::Url& url) :
   DataInput(),
   fFileName(url.GetFullName()),
   fFilesList(),
   fIO(0),
   fCurrentName(),
   fLoop(url.HasOption("loop"))
{
}

dabc::FileInput::~FileInput()
{
   DOUT3("Destroy file input %p", this);

   if (fIO!=0) {
      delete fIO;
      fIO = 0;
   }
}

void dabc::FileInput::SetIO(dabc::FileInterface* io)
{
   if (fIO!=0) {
      EOUT("File interface object already assigned");
      delete io;
   } else {
      fIO = io;
   }
}

bool dabc::FileInput::InitFilesList()
{
   if (fFileName.find_first_of("*?") != std::string::npos) {
      fFilesList = fIO->fmatch(fFileName.c_str());
   } else {
      fFilesList = new dabc::Object(0, "FilesList");
      new dabc::Object(fFilesList(), fFileName);
   }

   fFilesList.SetAutoDestroy(true);

   return fFilesList.NumChilds() > 0;
}

bool dabc::FileInput::Read_Init(const WorkerRef& wrk, const Command& cmd)
{
   if (!dabc::DataInput::Read_Init(wrk,cmd)) return false;

   if (fFileName.empty()) return false;

   if (!fFilesList.null()) {
      EOUT("Files list already exists");
      return false;
   }

   if (fIO==0) fIO = new dabc::FileInterface;

   return InitFilesList();
}

bool dabc::FileInput::TakeNextFileName()
{
   fCurrentName.clear();
   if (fFilesList.NumChilds() == 0) {
      if (!fLoop || !InitFilesList()) return false;
   }
   const char* nextname = fFilesList.GetChild(0).GetName();
   if (nextname!=0) fCurrentName = nextname;
   fFilesList.GetChild(0).Destroy();
   return !fCurrentName.empty();
}

bool dabc::FileInput::Read_Stat(dabc::Command cmd)
{
   cmd.SetStr("InputFileName", fFileName);
   cmd.SetStr("InputCurrFileName", fCurrentName);
   return true;
}


// ================================================================

dabc::FileOutput::FileOutput(const dabc::Url& url, const std::string& ext) :
   DataOutput(url),
   fFileName(url.GetFullName()),
   fSizeLimitMB(url.GetOptionInt(dabc::xml_maxsize,0)),
   fFileExtens(ext),
   fIO(0),
   fCurrentFileNumber(url.GetOptionInt(dabc::xml_number,0)),
   fCurrentFileName(),
   fCurrentFileSize(0),
   fTotalFileSize(0),
   fTotalNumBufs(0),
   fTotalNumEvents(0)
{
}

dabc::FileOutput::~FileOutput()
{
   if (fIO!=0) {
      delete fIO;
      fIO = 0;
   }
}

void dabc::FileOutput::SetIO(dabc::FileInterface* io)
{
   if (fIO!=0) {
      EOUT("File interface object already assigned");
      delete io;
   } else {
      fIO = io;
   }
}


bool dabc::FileOutput::Write_Init()
{
   if (!DataOutput::Write_Init()) return false;

   if (fIO==0) fIO = new FileInterface;

   std::string mask = ProduceFileName("*");

   dabc::Reference lst = fIO->fmatch(mask.c_str());

   int maxnumber = -1;

   for (unsigned cnt=0; cnt < lst.NumChilds(); cnt++) {

      std::string fname = lst.GetChild(cnt).GetName();

//      DOUT0("Test file %s", fname.c_str());

      fname.erase(fname.length()-fFileExtens.length(), fFileExtens.length());
      size_t pos = fname.length() - 1;
      while ((fname[pos]>='0') && (fname[pos] <= '9')) pos--;
      fname.erase(0, pos+1);

      while ((fname.length()>1) && fname[0]=='0') fname.erase(0, 1);

      int number(0);
      if ((fname.length()>0) && dabc::str_to_int(fname.c_str(), &number)) {
         if (number>maxnumber) maxnumber = number;
      }
   }

   lst.Destroy();

   if (fCurrentFileNumber <= maxnumber) {
      fCurrentFileNumber = maxnumber + 1;
      ShowInfo(0, dabc::format("start with file number %d", fCurrentFileNumber));
   }

   return true;
}

std::string dabc::FileOutput::ProduceFileName(const std::string& suffix)
{
   std::string fname = fFileName;

   size_t len = fname.length();
   size_t pos = fname.rfind(fFileExtens);

   if (pos == (len-fFileExtens.length())) {
      fname.insert(pos, suffix);
   } else {
      fname += suffix;
      fname += fFileExtens;
   }

   return fname;
}


void dabc::FileOutput::ProduceNewFileName()
{
   fCurrentFileName = ProduceFileName(dabc::format("_%04d", fCurrentFileNumber++));
   fCurrentFileSize = 0;
}


bool dabc::FileOutput::CheckBufferForNextFile(BufferSize_t sz)
{
   if (fSizeLimitMB > 0)
      return (fCurrentFileSize + sz)/1024./1024. > fSizeLimitMB;

   return false;
}


void dabc::FileOutput::AccountBuffer(BufferSize_t sz, int numev)
{
   fCurrentFileSize += sz;
   fTotalFileSize += sz;
   fTotalNumBufs++;
   fTotalNumEvents += numev;
}


std::string dabc::FileOutput::ProvideInfo()
{
   std::string info = fCurrentFileName;
   size_t pos = info.rfind("/");
   if (pos!=std::string::npos) info.erase(0, pos);

   info.append(" ");
   info.append(dabc::size_to_str(fCurrentFileSize));

   if (fSizeLimitMB>0)
      info.append(dabc::format(" (%3.1f %s)", 100./(fSizeLimitMB*1024.*1024.)*fCurrentFileSize, "%"));

   if (fTotalNumEvents > 0) {
      if (fTotalNumEvents<1000000)
         info.append(dabc::format(" %ld ev", fTotalNumEvents));
      else
         info.append(dabc::format(" %8.3e ev", 1.*fTotalNumEvents));
   } else {
      if (fTotalNumBufs<1000000)
         info.append(dabc::format(" %ld bufs", fTotalNumBufs));
      else
         info.append(dabc::format(" %8.3e bufs", 1.*fTotalNumBufs));
   }

   return info;
}

bool dabc::FileOutput::Write_Stat(dabc::Command cmd)
{
   cmd.SetStr("OutputFileName", fFileName);
   cmd.SetInt("OutputFileEvents", fTotalNumEvents);
   cmd.SetInt("OutputFileSize", fTotalFileSize);

   cmd.SetStr("OutputCurrFileName", fCurrentFileName);
   cmd.SetInt("OutputCurrFileSize", fCurrentFileSize);

   return true;
}

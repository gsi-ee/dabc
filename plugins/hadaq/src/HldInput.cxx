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

#include "hadaq/HldInput.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/FileIO.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"

#include "hadaq/HadaqTypeDefs.h"


hadaq::HldInput::HldInput(const char* fname, uint32_t bufsize) :
   dabc::DataInput(),
   fFileName(fname ? fname : ""),
   fBufferSize(bufsize),
   fFilesList(0),
   fFile(),
   fCurrentFileName(),
   fCurrentRead(0)
{
}

hadaq::HldInput::~HldInput()
{
   // FIXME: cleanup should be done much earlier
   CloseFile();
   if (fFilesList) {
      dabc::Object::Destroy(fFilesList);
      fFilesList = 0;
   }
}

bool hadaq::HldInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   fFileName = wrk.Cfg(hadaq::xmlFileName, cmd).AsStdStr(fFileName);
   //fFileName ="/data.local1/adamczew/hld/xx12153171835.hld";
   fBufferSize = wrk.Cfg(dabc::xmlBufferSize, cmd).AsInt(fBufferSize);
   fBufferSize /=2; // use half of pool size for raw input buffers, provides headroom for mbs wrappers
   DOUT1(("BufferSize = %d, filename=%s", fBufferSize,fFileName.c_str()));

   return Init();
}

bool hadaq::HldInput::Init()
{
   if (fFileName.length()==0) return false;

   if (fFilesList!=0) {
      EOUT(("Files list already exists"));
      return false;
   }

   if (fBufferSize==0) {
      EOUT(("Buffer size not specified !!!!"));
      return false;
   }

   if (strpbrk(fFileName.c_str(),"*?")!=0)
      fFilesList = dabc::mgr()->ListMatchFiles("", fFileName.c_str());
   else {
      fFilesList = new dabc::Object(0, "FilesList", true);
      new dabc::Object(fFilesList, fFileName.c_str());
   }

   return OpenNextFile();
}

bool hadaq::HldInput::OpenNextFile()
{
   CloseFile();

   if ((fFilesList==0) || (fFilesList->NumChilds()==0)) return false;

   const char* nextfilename = fFilesList->GetChild(0)->GetName();

   bool res = fFile.OpenRead(nextfilename);

   if (!res)
      EOUT(("Cannot open file %s for reading, errcode:%u", nextfilename, fFile.LastError()));
   else {
      fCurrentFileName = nextfilename;
      DOUT1(("Open hld file %s for reading", fCurrentFileName.c_str()));
   }

   fFilesList->DeleteChild(0);

   return res;
}


bool hadaq::HldInput::CloseFile()
{
   fFile.Close();
   fCurrentFileName = "";
   fCurrentRead = 0;
   return true;
}

unsigned hadaq::HldInput::Read_Size()
{
   // get size of the buffer which should be read from the file

   if (!fFile.IsReadMode())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return fBufferSize;
}

unsigned hadaq::HldInput::Read_Complete(dabc::Buffer& buf)
{
   buf.SetTypeId(hadaq::mbt_HadaqEvents);
   uint32_t readbytes = 0;
   uint32_t filestat = HLD__SUCCESS;
   char* dest = (char*) buf.SegmentPtr(0); // TODO: read into segmented buffer
   uint32_t bufsize = buf.SegmentSize(0);
   if(bufsize>fBufferSize ) bufsize=fBufferSize;
   bool nextfile = false;
   do {

      if (!fFile.IsReadMode())
         return dabc::di_Error;
      readbytes = fFile.ReadBuffer(dest, bufsize);
      fCurrentRead += readbytes;
      filestat = fFile.LastError();
      if (filestat == HLD__FULLBUF) {
         DOUT3(("File %s has filled dabc buffer, readbytes:%u, bufsize: %u, allbytes: %u", fCurrentFileName.c_str(), readbytes, buf.GetTotalSize(),fCurrentRead ));
         break;
      } else if (filestat == HLD__EOFILE) {
         DOUT1(("File %s has EOF for buffer, readbytes:%u, bufsize %u", fCurrentFileName.c_str(), readbytes, buf.GetTotalSize()));
         if (!OpenNextFile()) {
            buf.SetTotalSize(readbytes);
            return dabc::di_EndOfStream;
         }
         nextfile = true;
      }
      dest += readbytes;
      bufsize -= readbytes;
      if (bufsize == 0)
         break;
   } while (nextfile);
   buf.SetTotalSize(readbytes);
   return dabc::di_Ok;
}

hadaq::Event* hadaq::HldInput::ReadEvent()
{
   while (true) {
       if (!fFile.IsReadMode()) return 0;

       hadaq::Event* hdr = fFile.ReadEvent();
       if (hdr!=0) return hdr;

       DOUT1(("File %s return 0 - end of file", fCurrentFileName.c_str()));
       if (!OpenNextFile()) return 0;
   }

   return 0;
}

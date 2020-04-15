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

#include "hadaq/HldOutput.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#if defined(__MACH__) /* Apple OSX section */
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#include "dabc/Manager.h"

#include "hadaq/Iterator.h"


hadaq::HldOutput::HldOutput(const dabc::Url& url) :
   dabc::FileOutput(url,".hld"),
   fRunSlave(false),
   fLastRunNumber(0),
   fRunNumber(0),
   fEBNumber(0),
   fUseDaqDisk(false),
   fRfio(false),
   fLtsm(false),
   fUrlOptions(),
   fLastPrefix(),
   fFile()
{
   fRunSlave = url.HasOption("slave");
   fEBNumber = url.GetOptionInt("ebnumber",0); // default is single eventbuilder
   fRunNumber = url.GetOptionInt("runid", 0); // if specified, use runid from url
   fUseDaqDisk = url.GetOptionInt("diskdemon", 0); // if specified, use number of /data partition from daq_disk demon
   fRfio = url.HasOption("rfio");
   fLtsm = url.HasOption("ltsm");
   if (fRfio) {
      dabc::FileInterface* io = (dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface");

      if (io) {
         fUrlOptions = url.GetOptions();
         fFile.SetIO(io, true);
         // set default protocol and node name, can only be used in GSI
         //if (fFileName.find("rfiodaq:gstore:")== std::string::npos)
         fFileName = std::string("rfiodaq:gstore:") + fFileName;
      } else {
         EOUT("Cannot create RFIO object, check if libDabcRfio.so loaded");
      }
   } else if(fLtsm) {
	   dabc::FileInterface* io = (dabc::FileInterface*) dabc::mgr.CreateAny("ltsm::FileInterface");
	   if (io!=0) {
		   fUrlOptions = url.GetOptions();
		   fFile.SetIO(io, true);
	   } else {
		   EOUT("Cannot create LTSM object, check if libDabcLtsm.so loaded");
	   }
   }
}

hadaq::HldOutput::~HldOutput()
{
   DOUT3(" hadaq::HldOutput::DTOR");
   CloseFile();
}

bool hadaq::HldOutput::Write_Init()
{
   if (!dabc::FileOutput::Write_Init()) return false;

   if (fRunSlave) {
      // use parameters only in slave mode
      fRunNumber = 0;
      ShowInfo(0, dabc::format("%s slave mode is enabled, waiting for runid", (fRunSlave ? "RUN" : "EPICS")));
      return true;
   }

   return StartNewFile();
}


bool hadaq::HldOutput::StartNewFile()
{
   CloseFile();

   if (fRunNumber == 0) {
      if (fRunSlave) {
         EOUT("Cannot start new file without valid RUNID");
         return false;
      }

      fRunNumber = hadaq::CreateRunId();
      //std::cout <<"HldOutput Generates New Runid"<<fRunNumber << std::endl;
      ShowInfo(0, dabc::format("HldOutput Generates New Runid %d (0x%x)", fRunNumber, fRunNumber));
   }

   if(fUseDaqDisk) {
      dabc::Parameter fDiskNumberPar = dabc::mgr.FindPar("Combiner/Evtbuild-diskNum");
      if(!fDiskNumberPar.null()) {
         std::string prefix;
         size_t prepos = fFileName.rfind("/");
         if (prepos == std::string::npos)
            prefix = fFileName;
         else
            prefix = fFileName.substr(prepos+1);

         unsigned disknumber = fDiskNumberPar.Value().AsUInt();
         fFileName = dabc::format("/data%02d/data/%s",disknumber,prefix.c_str());
         DOUT1("Set filename from daq_disks to %s, disknumber was %d, prefix=%s",
               fFileName.c_str(), disknumber, prefix.c_str());

         dabc::CmdSetParameter cmd("Evtbuild-diskNumEB", disknumber);
         dabc::mgr.FindModule("Combiner").Submit(cmd);
      } else {
         EOUT("Could not find daq_disk parameter although disk demon mode is on!");
      }
   }
   // change file names according hades style:
   std::string extens = hadaq::FormatFilename(fRunNumber, fEBNumber);
   std::string fname = fFileName;

   if (!fLastPrefix.empty() && fRunSlave) {
      // when run in BNet mode, only file path used
      size_t slash = fname.rfind("/");
      if (slash == std::string::npos)
         fname = "";
      else
         fname.erase(slash+1);
      fname.append(fLastPrefix);
   }

   size_t pos = fname.rfind(".hld");
   if (pos == std::string::npos)
      pos = fname.rfind(".HLD");

   if ((pos != std::string::npos) && (pos == fname.length()-4)) {
      fname.insert(pos, extens);
   } else {
      fname += extens;
      fname += ".hld";
   }
   fCurrentFileName = fname;

   if (fRunSlave && fRfio)
      DOUT1("Before open file %s for writing", CurrentFileName().c_str());

   if (!fFile.OpenWrite(CurrentFileName().c_str(), fRunNumber, fUrlOptions.c_str())) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   // JAM2020: here we have to update the real filename in case that implementation changes it
   // this can happen for ltsm io where we may add subfolders for year and day
   char tmp[1024];
   if(fFile.GetStrPar("RealFileName", tmp, 1024))
     {
       std::string previous=CurrentFileName();
       fCurrentFileName=tmp;
       DOUT0("Note: Original file name %s was changed by implementation to %s", previous.c_str(), CurrentFileName().c_str());
     }
   
   if (fRunSlave && fRfio)
      DOUT1("File %s is open for writing", CurrentFileName().c_str());

   ShowInfo(0, dabc::format("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber));
   DOUT0("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber);

   fLastRunNumber = fRunNumber;

   return true;
}

bool hadaq::HldOutput::Write_Retry()
{
   // HLD output supports retry option

   CloseFile();
   if (!fRunSlave) fRunNumber = 0;
   return true;
}

bool hadaq::HldOutput::CloseFile()
{
   if (fFile.isOpened()) {
      ShowInfo(0, "HLD file is CLOSED");
      fFile.Close();
   }
   fCurrentFileSize = 0;
   fCurrentFileName = "";
   return true;
}

bool hadaq::HldOutput::Write_Restart(dabc::Command cmd)
{
   if (cmd.GetBool("only_prefix")) {
      // command used by BNet, prefix is not directly stored by the master
      std::string prefix = cmd.GetStr("prefix");
      if (!prefix.empty()) fLastPrefix = prefix;
   } else if (fFile.isWriting()) {
      CloseFile();
      fRunNumber = 0;
      StartNewFile();
      cmd.SetStr("FileName", fCurrentFileName);
   }
   return true;
}

bool hadaq::HldOutput::Write_Stat(dabc::Command cmd)
{
   bool res = dabc::FileOutput::Write_Stat(cmd);

   cmd.SetUInt("RunId", fRunNumber);
   cmd.SetUInt("RunSize", fCurrentFileSize);
   cmd.SetStr("RunName", fCurrentFileName);
   cmd.SetStr("RunPrefix", fLastPrefix);

   return res;
}


unsigned hadaq::HldOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   bool is_eol = (buf.GetTypeId() == hadaq::mbt_HadaqStopRun);

   if ((buf.GetTypeId() != hadaq::mbt_HadaqEvents) && !is_eol) {
      ShowInfo(-1, dabc::format("Buffer must contain hadaq event(s), but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   unsigned cursor(0);
   bool startnewfile(false);
   if (is_eol) {
      // just reset number
      fRunNumber = 0;
      startnewfile = true;
      DOUT2("HLD output process EOL");
   } else if (fRunSlave) {

      // scan event headers in buffer for run id change/consistency
      hadaq::ReadIterator bufiter(buf);
      unsigned numevents(0), payload(0);

      while (bufiter.NextEvent()) {
         uint32_t nextrunid = bufiter.evnt()->GetRunNr();
         if (nextrunid == fRunNumber) {
            numevents++;
            payload += bufiter.evnt()->GetPaddedSize();// remember current position in buffer:
            continue;
         }

//          ShowInfo(0, dabc::format("HldOutput Finds New Runid %d (0x%x) from EPICS in event header (previous:%d (0x%x))",
//                     nextrunid, nextrunid, fRunNumber,fRunNumber));
         DOUT1("HldOutput Finds New Runid %d or 0x%x from EPICS in event header (previous: %d or 0x%x)",
                  nextrunid, nextrunid, fRunNumber, fRunNumber);
         fRunNumber = nextrunid;
         startnewfile = true;
         break;

      } // while bufiter

      // if current runid is still 0, just ignore buffer
      if (!startnewfile && (fRunNumber == 0)) return dabc::do_Ok;

      if(startnewfile) {
         // first flush rest of previous run to old file:
         cursor = payload;

         // only if file opened for writing, write rest buffers
         if (fFile.isWriting())
         for (unsigned n=0;n<buf.NumSegments();n++) {

            if (payload==0) break;

            unsigned write_size = buf.SegmentSize(n);
            if (write_size > payload) write_size = payload;

            if (fRfio)
               DOUT1("HldOutput write %u bytes from buffer with old runid", write_size);

            if (!fFile.WriteBuffer(buf.SegmentPtr(n), write_size)) return dabc::do_Error;

            DOUT1("HldOutput did flushes %d bytes (%d events) of old runid in buffer segment %d to file",
                  write_size, numevents, n);

            payload -= write_size;
         }// for
      }

   } else {
      if (CheckBufferForNextFile(buf.GetTotalSize())) {
         fRunNumber = 0;
         startnewfile = true;
      }
   } // epicsslave

   if(startnewfile) {

      if (fRunSlave && (fRunNumber == 0)) {
         // in slave mode 0 runnumber means do nothing
         CloseFile();
         DOUT0("CLOSE FILE WRITING in slave mode");
         return dabc::do_Ok;
      }

      if ((fLastRunNumber != 0) && (fLastRunNumber == fRunNumber)) {
         DOUT0("Saw same runid %d 0x%u as previous - skip buffer", fLastRunNumber, fLastRunNumber);
         return dabc::do_Ok;
      }

      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }
      ShowInfo(0, dabc::format("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber));
   }

   if (!fFile.isWriting()) return dabc::do_Error;

   unsigned total_write_size(0);

   for (unsigned n=0;n<buf.NumSegments();n++) {

      unsigned write_size = buf.SegmentSize(n);

      if (cursor>=write_size) {
         // skip segment completely
         cursor -= write_size;
         continue;
      }

      char* write_ptr = (char*) buf.SegmentPtr(n);

      if(startnewfile)
      {
         DOUT2("Wrote to %s at segment %d, cursor %d, size %d", CurrentFileName().c_str(), n, cursor,  write_size-cursor);
      }

      if (cursor>0) {
         write_ptr += cursor;
         write_size -= cursor;
         cursor = 0;
      }

      if (fRunSlave && fRfio && startnewfile)
         DOUT1("HldOutput write %u bytes after new file was started", write_size);

      if (!fFile.WriteBuffer(write_ptr, write_size)) return dabc::do_Error;

      if (fRunSlave && fRfio && startnewfile)
         DOUT1("HldOutput did write %u bytes after new file was started", write_size);

      total_write_size += write_size;
   }

   // TODO: in case of partial written buffer, account sizes to correct file
   AccountBuffer(total_write_size, hadaq::ReadIterator::NumEvents(buf));

   if (fRunSlave && fRfio && startnewfile)
      DOUT1("HldOutput write complete first buffer after new file was started");

   return dabc::do_Ok;
}

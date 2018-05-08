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
   fEpicsSlave(false),
   fRunNumber(0),
   fEBNumber(0),
   fUseDaqDisk(false),
   fRfio(false),
   fLtsm(false),
   fUrlOptions(),
   fLastUpdate(),
   fFile()
{
   fEpicsSlave = url.HasOption("epicsctrl");
   fEBNumber = url.GetOptionInt("ebnumber",0); // default is single eventbuilder
   fRunNumber = url.GetOptionInt("runid", 0); // if specified, use runid from url
   fUseDaqDisk = url.GetOptionInt("diskdemon", 0); // if specified, use number of /data partition from daq_disk demon
   fRfio = url.HasOption("rfio");
   fLtsm = url.HasOption("ltsm");
   if (fRfio) {

      dabc::FileInterface* io = (dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface");

      if (io!=0) {
         fUrlOptions = url.GetOptions();
         fFile.SetIO(io, true);
         // set default protocol and node name, can only be used in GSI
         //if (fFileName.find("rfiodaq:gstore:")== std::string::npos)
         fFileName = std::string("rfiodaq:gstore:") + fFileName;
      } else {
         EOUT("Cannot create RFIO object, check if libDabcRfio.so loaded");
      }
   }
   else if(fLtsm)
   {
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

//static int gggcnt = 0;

bool hadaq::HldOutput::Write_Init()
{
//   gggcnt = 0;

   if (!dabc::FileOutput::Write_Init()) return false;

   if (fEpicsSlave) {
      // use parameters only in slave mode

      fLastUpdate.GetNow();

      fRunNumber = 0;

      ShowInfo(0, "EPICS slave mode is enabled, waiting for runid");
      return true;
   }

   return StartNewFile();
}


bool hadaq::HldOutput::StartNewFile()
{
   CloseFile();

   if (fRunNumber == 0) {
      fRunNumber = hadaq::CreateRunId();
      //std::cout <<"HldOutput Generates New Runid"<<fRunNumber << std::endl;
      ShowInfo(0, dabc::format("HldOutput Generates New Runid %d (0x%x)", fRunNumber, fRunNumber));
   }
   if(fUseDaqDisk)
   {
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
      }
      else
      {
         EOUT("Could not find daq_disk parameter although disk demon mode is on!");
      }
   }
   // change file names according hades style:
   std::string extens = hadaq::FormatFilename(fRunNumber,fEBNumber);
   std::string fname = fFileName;

   size_t pos = fname.rfind(".hld");
   if (pos == std::string::npos)
      pos = fname.rfind(".HLD");

   if (pos == fname.length()-4) {
      fname.insert(pos, extens);
   } else {
      fname += extens;
      fname += ".hld";
   }
   fCurrentFileName = fname;

   if (fEpicsSlave && fRfio)
      DOUT1("Before open file %s for writing", CurrentFileName().c_str());

   if (!fFile.OpenWrite(CurrentFileName().c_str(), fRunNumber, fUrlOptions.c_str())) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   if (fEpicsSlave && fRfio)
      DOUT1("File %s is open for writing", CurrentFileName().c_str());

   if (fEpicsSlave && (fRfio || fLtsm)) {
      // use parameters only in slave mode

      int indx = fFile.GetIntPar("DataMoverIndx");// get actual number of data mover from file interface

      char sbuf[100];
      if (fFile.GetStrPar("DataMoverName", sbuf, sizeof(sbuf))); // can use data mover name here

      dabc::CmdSetParameter cmd("Evtbuild-dataMover", indx);
      dabc::mgr.FindModule("Combiner").Submit(cmd);
      DOUT0("Connected to %s %s, Number:%d", (fRfio ? "Datamover" : "TSM Server") , sbuf, indx);
  }

   ShowInfo(0, dabc::format("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber));
   DOUT0("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber);

   return true;

}

bool hadaq::HldOutput::Write_Retry()
{
   // HLD output supports retry option

   CloseFile();
   fRunNumber = 0;
   return true;
}


bool hadaq::HldOutput::CloseFile()
{
   DOUT3(" hadaq::HldOutput::CloseFile()");
   if (fFile.isWriting()) ShowInfo(0, "HLD file is CLOSED");
   fFile.Close();
   fCurrentFileSize = 0;
   //std::cout <<"Close File resets file size." << std::endl;
   return true;
}

bool hadaq::HldOutput::Write_Restart(dabc::Command cmd)
{
   if (fFile.isWriting()) {
      CloseFile();
      fRunNumber = 0;
      StartNewFile();
      cmd.SetStr("FileName", fCurrentFileName);
   }
   return true;
}


unsigned hadaq::HldOutput::Write_Buffer(dabc::Buffer& buf)
{
//   if (gggcnt++ > 100) { ::sleep(10); return dabc::do_Error; }

   if (buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   if (buf.GetTypeId() != hadaq::mbt_HadaqEvents) {
      ShowInfo(-1, dabc::format("Buffer must contain hadaq event(s), but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   unsigned cursor(0);
   bool startnewfile(false);
   if (fEpicsSlave) {

      // scan event headers in buffer for run id change/consistency
      hadaq::ReadIterator bufiter(buf);
      unsigned numevents(0), payload(0);

      while (bufiter.NextEvent())
      {
         uint32_t nextrunid = bufiter.evnt()->GetRunNr();
         if (nextrunid == fRunNumber) {
            numevents++;
            payload += bufiter.evnt()->GetPaddedSize();// remember current position in buffer:
            continue;
         }

//          ShowInfo(0, dabc::format("HldOutput Finds New Runid %d (0x%x) from EPICS in event header (previous:%d (0x%x))",
//                     nextrunid, nextrunid, fRunNumber,fRunNumber));
         DOUT1("HldOutput Finds New Runid %d (0x%x) from EPICS in event header (previous:%d (0x%x))",
                  nextrunid, nextrunid, fRunNumber,fRunNumber);
         fRunNumber = nextrunid;
         startnewfile = true;
         break;

      } // while bufiter

      // if current runid is still 0, just ignore buffer
      if (!startnewfile && (fRunNumber==0)) return dabc::do_Ok;

      if(startnewfile) {
         // first flush rest of previous run to old file:
         cursor = payload;

         // only if file opened for writing, write rest buffers
         if (fFile.isWriting())
         for (unsigned n=0;n<buf.NumSegments();n++) {

            if (payload==0) break;

            unsigned write_size = buf.SegmentSize(n);
            if (write_size > payload) write_size = payload;

            if (fEpicsSlave && fRfio)
               DOUT1("HldOutput write %u bytes from buffer with old runid", write_size);

            if (!fFile.WriteBuffer(buf.SegmentPtr(n), write_size)) return dabc::do_Error;

            DOUT1("HldOutput did flushes %d bytes (%d events) of old runid in buffer segment %d to file",
                  write_size, numevents, n);

            payload -= write_size;
         }// for
      }

      //#endif // oldmode

      if (fLastUpdate.Expired(0.2)) {
         dabc::CmdSetParameter cmd("Evtbuild-bytesWritten", (int)fCurrentFileSize);
         dabc::mgr.FindModule("Combiner").Submit(cmd);
         fLastUpdate.GetNow();
      }

   } else {
      if (CheckBufferForNextFile(buf.GetTotalSize())) {
         fRunNumber = 0;
         startnewfile = true;
      }

   } // epicsslave

   if(startnewfile) {
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }
      ShowInfo(0, dabc::format("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber));
   }

   if (!fFile.isWriting()) return dabc::do_Error;

   unsigned total_write_size(0);

   for (unsigned n=0;n<buf.NumSegments();n++)
   {
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

      if (fEpicsSlave && fRfio && startnewfile)
         DOUT1("HldOutput write %u bytes after new file was started", write_size);

      if (!fFile.WriteBuffer(write_ptr, write_size)) return dabc::do_Error;

      if (fEpicsSlave && fRfio && startnewfile)
         DOUT1("HldOutput did write %u bytes after new file was started", write_size);

      total_write_size += write_size;
   }

   // TODO: in case of partial written buffer, account sizes to correct file
   AccountBuffer(total_write_size, hadaq::ReadIterator::NumEvents(buf));

   if (fEpicsSlave && fRfio && startnewfile)
      DOUT1("HldOutput write complete first buffer after new file was started");

   return dabc::do_Ok;
}

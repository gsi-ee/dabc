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
   fUrlOptions(),
   fRunidPar(),
   fBytesWrittenPar(),
   fDiskNumberPar(),
   fDiskNumberGuiPar(),
   fDataMoverPar(),
   fFile()
{
   fEpicsSlave = url.HasOption("epicsctrl");
   fEBNumber = url.GetOptionInt("ebnumber",0); // default is single eventbuilder
   fRunNumber = url.GetOptionInt("runid", 0); // if specified, use runid from url
   fUseDaqDisk = url.GetOptionInt("diskdemon", 0); // if specified, use number of /data partition from daq_disk demon
   fRfio = url.HasOption("rfio");
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
}

hadaq::HldOutput::~HldOutput()
{
   CloseFile();
}

bool hadaq::HldOutput::Write_Init()
{
   if (!dabc::FileOutput::Write_Init()) return false;

   if (fEpicsSlave) {
      // use parameters only in slave mode

      fRunidPar = dabc::mgr.FindPar("Combiner/Evtbuild_runId");
      fBytesWrittenPar = dabc::mgr.FindPar("Combiner/Evtbuild_bytesWritten");

      if(fRunidPar.null()) {
         ShowInfo(-1, "HldOutput::Write_Init did not find runid parameter");
         return false;
      }

      if(fBytesWrittenPar.null()) {
         ShowInfo(-1, "HldOutput::Write_Init did not find written bytes parameter");
         return false;
      }

      fRunNumber = fRunidPar.Value().AsUInt();

      if (fRunNumber == 0) {
         ShowInfo(0, "EPICS slave mode is enabled, waiting for runid");
         return true;
      }

      ShowInfo(0, dabc::format("EPICS slave mode is enabled, first runid:%d (0x%x)",fRunNumber, fRunNumber));


   }

   return StartNewFile();
}


bool hadaq::HldOutput::StartNewFile()
{
   CloseFile();

   if (fRunNumber == 0) {
      fRunNumber = hadaq::RawEvent::CreateRunId();
      //std::cout <<"HldOutput Generates New Runid"<<fRunNumber << std::endl;
      ShowInfo(0, dabc::format("HldOutput Generates New Runid %d (0x%x)", fRunNumber, fRunNumber));
   }
   if(fUseDaqDisk)
   {
      fDiskNumberPar = dabc::mgr.FindPar("Combiner/Evtbuild_diskNum");
      if(!fDiskNumberPar.null()) {
         std::string prefix;
         size_t prepos= fFileName.rfind("/");
         if (prepos == std::string::npos)
            prefix=fFileName;
         else
            prefix=fFileName.substr(prepos+1);

         unsigned disknumber = fDiskNumberPar.Value().AsUInt();
         fFileName = dabc::format("/data%02d/data/%s",disknumber,prefix.c_str());
         DOUT0("Set filename from daq_disks to %s, disknumber was %d, prefix=%s",
               fFileName.c_str(), disknumber, prefix.c_str());
         fDiskNumberGuiPar = dabc::mgr.FindPar("Combiner/Evtbuild_diskNumEB");
         if(!fDiskNumberGuiPar.null()) {
            fDiskNumberGuiPar.SetValue(disknumber);
            DOUT0("Updated disknumber %d for gui",disknumber);
         }
      }
      else
      {
         EOUT("Could not find daq_disk parameter although disk demon mode is on!");
      }	
   }
   // change file names according hades style:
   std::string extens = hadaq::RawEvent::FormatFilename(fRunNumber,fEBNumber);
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

   if (!fFile.OpenWrite(CurrentFileName().c_str(), fRunNumber, fUrlOptions.c_str())) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   if (fEpicsSlave && fRfio) {
      // use parameters only in slave mode

      fDataMoverPar = dabc::mgr.FindPar("Combiner/Evtbuild_dataMover");

      int indx = fFile.GetIntPar("DataMoverIndx");

      // char sbuf[100];
      // if (fFile.GetStrPar("DataMoverName", sbuf, sizeof(sbuf))); // can use data mover name here

      if(!fDataMoverPar.null()) {
         fDataMoverPar.SetValue(indx); // TODO: get here actual number of data mover from file interface!
      }
   }

   ShowInfo(0, dabc::format("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber));
   DOUT0("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber);
   return true;
}

bool hadaq::HldOutput::CloseFile()
{
   if (fFile.isWriting())
      ShowInfo(0, "HLD file is CLOSED");
   fFile.Close();
   fCurrentFileSize=0;
   //std::cout <<"Close File resets file size." << std::endl;
   return true;
}

unsigned hadaq::HldOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (buf.null()) return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   if (buf.GetTypeId() != hadaq::mbt_HadaqEvents) {
      ShowInfo(-1, dabc::format("Buffer must contain hadaq event(s), but has type %u", buf.GetTypeId()));
      return dabc::do_Error;
   }

   unsigned cursor=0;
   unsigned startsegment=0;
   bool startnewfile = false;
   if (fEpicsSlave) {

      // #ifdef OLDMODE
      //       // check if EPICS master has assigned a new run for us:
      //       uint32_t nextrunid = fRunidPar.Value().AsUInt();
      //       if (nextrunid != fRunNumber) {
      //          fRunNumber = nextrunid;
      //          startnewfile = true;
      //          ShowInfo(0, dabc::format("HldOutput Gets New Runid %d (0x%x)from EPICS", fRunNumber,fRunNumber));
      // 	 DOUT0("HldOutput Gets New Runid %d (0x%x)from EPICS", fRunNumber,fRunNumber);
      //       } else {
      //
      //          if ((nextrunid == 0) && (fRunNumber==0)) {
      //             // ignore buffer while run number is not yet known
      //             return dabc::do_Ok;
      //          }
      //       }
      // #else



      if(fRunNumber==0)
      {
         // runid might not be available on initializeation.
         // check here if it was already delivered from epics
         fRunNumber = fRunidPar.Value().AsUInt();
         if (fRunNumber == 0) {
            //ShowInfo(2, "EPICS slave mode: still have no runid, skip buffer!");
            DOUT2("EPICS slave mode: still have no runid, skip buffer!");
            return dabc::do_Skip;
         }
         if (!StartNewFile()) {
            EOUT("Cannot start new file for writing");
            return dabc::do_Error;

         }
      }

      // scan event headers in buffer for run id change/consistency
      hadaq::ReadIterator bufiter(buf);
      uint32_t nextrunid=0;
      unsigned numevents=0;
      uint32_t payload=0;

      while (bufiter.NextEvent())
      {
         numevents++;
         payload+=bufiter.evnt()->GetPaddedSize();// remember current position in buffer:
         nextrunid=bufiter.evnt()->GetRunNr();
         if (nextrunid == 0) {
            // ignore entire buffer while run number is not yet known
            return dabc::do_Ok;
         }
         else if (nextrunid && nextrunid != fRunNumber) {
            ShowInfo(0, dabc::format("HldOutput Finds New Runid %d (0x%x) from EPICS in event header (previous:%d (0x%x))",
                  nextrunid, nextrunid, fRunNumber,fRunNumber));
            DOUT0("HldOutput Finds New Runid %d (0x%x) from EPICS in event header (previous:%d (0x%x))",
                  nextrunid, nextrunid, fRunNumber,fRunNumber);
            fRunNumber = nextrunid;
            payload-=bufiter.evnt()->GetPaddedSize(); // the first event with new runid belongs to next file
            startnewfile = true;
            break;
         }


      } // while bufiter

      if(startnewfile) {
         // first flush rest of previous run to old file:
         for (unsigned n=0;n<buf.NumSegments();n++)
         {
            if(buf.SegmentSize(n)<payload){
               if (!fFile.WriteBuffer(buf.SegmentPtr(n), buf.SegmentSize(n)))
                  return dabc::do_Error;
               payload-=buf.SegmentSize(n);

            }
            else
            {
               //ShowInfo(0, dabc::format("HldOutput flushes %d bytes (%d events) of old runid in buffer segment %d to file",
               //		    payload, numevents, n));
               DOUT0("HldOutput flushes %d bytes (%d events) of old runid in buffer segment %d to file",
                     payload, numevents, n);
               if(payload)
               {
                  if (!fFile.WriteBuffer(buf.SegmentPtr(n), payload))
                     return dabc::do_Error;
               }
               cursor=payload;
               startsegment=n;
            }


         }// for

      }



      //#endif // oldmode

      if (!fBytesWrittenPar.null())
         fBytesWrittenPar.SetValue((int)fCurrentFileSize);



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

   for (unsigned n=startsegment;n<buf.NumSegments();n++)
   {
      if(n>startsegment) cursor=0;
      if(cursor>=buf.SegmentSize(n))
      {
         DOUT2("Cursor %d  bigger than segment size %d, do not write to segment %d", cursor,  buf.SegmentSize(n),n);
         continue;
      }
      if (!fFile.WriteBuffer((char*) buf.SegmentPtr(n) + cursor, buf.SegmentSize(n)-cursor))
         return dabc::do_Error;

      if(startnewfile)
      {
         DOUT2("Wrote to %s at segment %d, cursor %d, size %d", CurrentFileName().c_str(), n, cursor,  buf.SegmentSize(n)-cursor);

      }

   }

   // TODO: in case of partial written buffer, account sizes to correct file 
   AccountBuffer(buf.GetTotalSize(), hadaq::ReadIterator::NumEvents(buf));

   return dabc::do_Ok;
}

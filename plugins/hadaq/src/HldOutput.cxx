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
   fRunidPar(),
   fBytesWrittenPar(),
   fFile()
{
   fEpicsSlave = url.HasOption("epicsctrl");
   fEBNumber = url.GetOptionInt("ebnumber",0); // default is single eventbuilder
   fRunNumber = url.GetOptionInt("runid", 0); // if specified, use runid from url

   if (url.HasOption("rfio")) {

      dabc::FileInterface* io = (dabc::FileInterface*) dabc::mgr.CreateAny("rfio::FileInterface");

      if (io!=0) {
         fFile.SetIO(io, true);
         // set default protocol and node name, can only be used in GSI
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

   if (!fFile.OpenWrite(CurrentFileName().c_str(), fRunNumber)) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   ShowInfo(0, dabc::format("%s open for writing runid %d", CurrentFileName().c_str(), fRunNumber));

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

   bool startnewfile = false;
   if (fEpicsSlave) {
      // check if EPICS master has assigned a new run for us:
      uint32_t nextrunid = fRunidPar.Value().AsUInt();
      if (nextrunid != fRunNumber) {
         fRunNumber = nextrunid;
         startnewfile = true;
         ShowInfo(0, dabc::format("HldOutput Gets New Runid %d (0x%x)from EPICS", fRunNumber,fRunNumber));
      } else {

         if ((nextrunid == 0) && (fRunNumber==0)) {
            // ignore buffer while run number is not yet known
            return dabc::do_Ok;
         }
      }

      if (!fBytesWrittenPar.null())
         fBytesWrittenPar.SetValue((int)fCurrentFileSize);

   } else {
      if (CheckBufferForNextFile(buf.GetTotalSize())) {
         fRunNumber = 0;
         startnewfile = true;
      }
   }

   if(startnewfile) {
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }
   }

   if (!fFile.isWriting()) return dabc::do_Error;

   for (unsigned n=0;n<buf.NumSegments();n++)
      if (!fFile.WriteBuffer(buf.SegmentPtr(n), buf.SegmentSize(n)))
         return dabc::do_Error;

   AccountBuffer(buf.GetTotalSize(), hadaq::ReadIterator::NumEvents(buf));

   return dabc::do_Ok;
}

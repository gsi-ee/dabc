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

#include "dogma/DogmaOutput.h"

#include "dogma/TypeDefs.h"
#include "dogma/Iterator.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#include "dabc/Manager.h"


dogma::DogmaOutput::DogmaOutput(const dabc::Url& url) :
   dabc::FileOutput(url, ".dld")
{
   fEBNumber = url.GetOptionInt("ebnumber",0); // default is single eventbuilder
   fUseDaqDisk = url.GetOptionInt("diskdemon", 0); // if specified, use number of /data partition from daq_disk demon
   fRfio = url.HasOption("rfio");
   fLtsm = url.HasOption("ltsm");
   fPlainName = url.HasOption("plain") && (GetSizeLimitMB() <= 0);
   if (fRfio) {
      dabc::FileInterface *io = (dabc::FileInterface *) dabc::mgr.CreateAny("rfio::FileInterface");

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
	   auto io = (dabc::FileInterface *) dabc::mgr.CreateAny("ltsm::FileInterface");
	   if (io) {
		   fUrlOptions = url.GetOptions();
		   fFile.SetIO(io, true);
	   } else {
		   EOUT("Cannot create LTSM object, check if libDabcLtsm.so loaded");
	   }
   }
}

dogma::DogmaOutput::~DogmaOutput()
{
   DOUT3(" dogma::DogmaOutput::DTOR");
   CloseFile();
}

bool dogma::DogmaOutput::Write_Init()
{
   if (!dabc::FileOutput::Write_Init())
      return false;

   return StartNewFile();
}


bool dogma::DogmaOutput::StartNewFile()
{
   CloseFile();

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
   std::string fname = fFileName;

   size_t pos = fname.rfind(".dld");
   if (pos == std::string::npos)
      pos = fname.rfind(".DLD");

   if (pos == std::string::npos)
      fname += ".dld";

   fCurrentFileName = fname;

   if (!fFile.OpenWrite(CurrentFileName().c_str(), fUrlOptions.c_str())) {
      ShowInfo(-1, dabc::format("%s cannot open file for writing", CurrentFileName().c_str()));
      return false;
   }

   // JAM2020: here we have to update the real filename in case that implementation changes it
   // this can happen for ltsm io where we may add subfolders for year and day
   char tmp[1024];
   if(fFile.GetStrPar("RealFileName", tmp, 1024)) {
       std::string previous = CurrentFileName();
       fCurrentFileName = tmp;
       DOUT0("Note: Original file name %s was changed by implementation to %s", previous.c_str(), CurrentFileName().c_str());
     }

   std::string info = dabc::format("%s open for writing", CurrentFileName().c_str());
   if (!ShowInfo(0, info))
      DOUT0("%s", info.c_str());

   return true;
}

bool dogma::DogmaOutput::Write_Retry()
{
   CloseFile();
   return true;
}

bool dogma::DogmaOutput::CloseFile()
{
   if (fFile.isOpened()) {
      ShowInfo(0, "DOGMA file is CLOSED");
      fFile.Close();
   }
   fCurrentFileSize = 0;
   fCurrentFileName = "";
   return true;
}

bool dogma::DogmaOutput::Write_Restart(dabc::Command cmd)
{
   if (cmd.GetBool("only_prefix")) {
      // command used by BNet, prefix is not directly stored by the master
      std::string prefix = cmd.GetStr("prefix");
      if (!prefix.empty()) fLastPrefix = prefix;
   } else if (fFile.isWriting()) {
      CloseFile();
      StartNewFile();
      cmd.SetStr("FileName", fCurrentFileName);
   }
   return true;
}

bool dogma::DogmaOutput::Write_Stat(dabc::Command cmd)
{
   bool res = dabc::FileOutput::Write_Stat(cmd);

   cmd.SetUInt("RunSize", fCurrentFileSize);
   cmd.SetStr("RunName", fCurrentFileName);
   cmd.SetStr("RunPrefix", fLastPrefix);

   return res;
}


unsigned dogma::DogmaOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (buf.null())
      return dabc::do_Error;

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      CloseFile();
      return dabc::do_Close;
   }

   bool is_eol = (buf.GetTypeId() == dogma::mbt_DogmaStopRun);

   bool is_subev = (buf.GetTypeId() == dogma::mbt_DogmaSubevents);

   bool is_events = (buf.GetTypeId() == dogma::mbt_DogmaEvents);

   if (!is_events && !is_eol && !is_subev) {
      std::string info = dabc::format("Buffer must contain dogma event(s), but has type %u", buf.GetTypeId());
      if (!ShowInfo(-1, info.c_str()))
         EOUT("%s", info.c_str());

      return dabc::do_Error;
   }

   unsigned cursor = 0;
   bool startnewfile = false;
   if (is_eol) {
      // just reset number
      startnewfile = true;
      DOUT2("HLD output process EOL");
   } else if (CheckBufferForNextFile(buf.GetTotalSize())) {
      startnewfile = true;
   } // epicsslave

   if(startnewfile) {
      if (!StartNewFile()) {
         EOUT("Cannot start new file for writing");
         return dabc::do_Error;
      }
      ShowInfo(0, dabc::format("%s open for writing", CurrentFileName().c_str()));
   }

   if (!fFile.isWriting())
      return dabc::do_Error;

   unsigned total_write_size = 0, num_events = 0;

   if (is_subev) {
      // this is list of subevents in the buffer, one need to add artificial events headers for each subevents

/*      hadaq::RawEvent evnt;

      hadaq::ReadIterator iter(buf);
      while (iter.NextSubeventsBlock()) {

         if (!iter.NextSubEvent())
            return dabc::do_Error;

         char* write_ptr = (char*) iter.subevnt();
         unsigned write_size = iter.subevnt()->GetPaddedSize();

         evnt.Init(fEventNumber++, fRunNumber);
         evnt.SetSize(write_size + sizeof(hadaq::RawEvent));

         if (!fFile.WriteBuffer(&evnt, sizeof(hadaq::RawEvent)))
            return dabc::do_Error;

         if (!fFile.WriteBuffer(write_ptr, write_size))
            return dabc::do_Error;

         total_write_size += sizeof(hadaq::RawEvent) + write_size;
         num_events ++;
      }
*/

   } else if (is_events) {

      for (unsigned n = 0; n < buf.NumSegments(); n++) {

         unsigned write_size = buf.SegmentSize(n);

         if (cursor >= write_size) {
            // skip segment completely
            cursor -= write_size;
            continue;
         }

         char *write_ptr = (char *)buf.SegmentPtr(n);

         if (cursor > 0) {
            write_ptr += cursor;
            write_size -= cursor;
            cursor = 0;
         }

         if (!fFile.WriteBuffer(write_ptr, write_size))
            return dabc::do_Error;

         total_write_size += write_size;
      }

      num_events = dogma::ReadIterator::NumEvents(buf);
   }

   // TODO: in case of partial written buffer, account sizes to correct file
   AccountBuffer(total_write_size, num_events);

   return dabc::do_Ok;
}

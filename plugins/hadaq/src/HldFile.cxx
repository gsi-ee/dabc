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

#include "hadaq/HldFile.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

hadaq::HldFile::HldFile() :
   dabc::BasicFile(),
   fRunNumber(0),
   fEOF(true)
{
}

hadaq::HldFile::~HldFile()
{
   Close();
}

bool hadaq::HldFile::OpenWrite(const char* fname, uint32_t runid)
{
   if (isOpened()) return false;

   if (fname==0 || *fname==0) {
      fprintf(stderr, "file name not specified\n");
      return false;
   }

   CheckIO();

   fd = io->fopen(fname, "w");
   if (fd==0) {
      fprintf(stderr, "File open failed %s for writing\n", fname);
      return false;
   }

   // put here a dummy event into file:

   hadaq::RawEvent evnt;
   evnt.Init(0, runid, EvtId_runStart);
   if(!WriteBuffer(&evnt, sizeof(evnt))) {
      CloseBasicFile();
      return false;
   }

   fReadingMode = false;
   fRunNumber = runid;

   return true;
}

bool hadaq::HldFile::OpenRead(const char* fname)
{
   if (isOpened()) return false;

   if (fname==0 || *fname==0) {
      fprintf(stderr, "file name not specified\n");
      return false;
   }

   CheckIO();

   fd = io->fopen(fname,  "r");
   if (fd==0) {
      fprintf(stderr, "File open failed %s for reading\n", fname);
      return false;
   }

//   DOUT0("Open HLD file %s for reading", fname);

   hadaq::RawEvent evnt;
   uint32_t size = sizeof(hadaq::RawEvent);

   if (!ReadBuffer(&evnt, &size, true)) {
      fprintf(stderr,"Cannot read starting event from file");
      CloseBasicFile();
      return false;
   }

   if ((size!=sizeof(hadaq::RawEvent)) || (evnt.GetId() != EvtId_runStart)) {
      fprintf(stderr,"Did not found start event at the file beginning");
      CloseBasicFile();
      return false;
   }

//   DOUT0("Find start event at the file begin");

   fRunNumber = evnt.GetRunNr();
   fReadingMode = true;
   fEOF = false;

   return true;
}

void hadaq::HldFile::Close()
{
  if (isWriting()) {
      // need to add empty terminating event:
      hadaq::RawEvent evnt;
      evnt.Init(0, fRunNumber, EvtId_runStop);
      WriteBuffer(&evnt, sizeof(evnt));
   }

  CloseBasicFile();

  fRunNumber=0;
  fEOF = true;
}



bool hadaq::HldFile::WriteBuffer(void* buf, uint32_t bufsize)
{
   if (!isWriting() || (buf==0) || (bufsize==0)) return false;

   if (io->fwrite(buf, bufsize, 1, fd)!=1) {
      fprintf(stderr, "fail to write buffer payload of size %u\n", (unsigned) bufsize);
      CloseBasicFile();
      return false;
   }

   return true;
}

bool hadaq::HldFile::ReadBuffer(void* ptr, uint32_t* sz, bool onlyevent)
{
   if (!isReading() || (ptr==0) || (sz==0) || (*sz < sizeof(hadaq::HadTu))) return false;

   uint64_t maxsz = *sz; *sz = 0;

   size_t readsz = io->fread(ptr, 1, maxsz, fd);

//   DOUT0("Read HLD portion of data %u", (unsigned) readsz);

   if (readsz < sizeof(hadaq::HadTu)) {
      if (io->feof(fd)) fEOF = true;
      return false;
   }

   size_t checkedsz = 0;

   hadaq::HadTu* hdr = (hadaq::HadTu*) ptr;

   while (checkedsz < readsz) {
      // special case when event was read not completely
      // or we want to provide only event

      size_t restsize = readsz - checkedsz;
      if (restsize >= sizeof(hadaq::HadTu)) restsize = hdr->GetPaddedSize();

      if ((restsize == sizeof(hadaq::RawEvent)) && (((hadaq::RawEvent*)hdr)->GetId() == EvtId_runStop)) {
         // we are not deliver such stop event to the top

         // printf("Find stop event at the file end\n");
         fEOF = true;
         break;
      }

      bool not_enough_place_for_next_event = checkedsz + restsize > readsz;

      if (not_enough_place_for_next_event || (onlyevent && (checkedsz>0))) {

         if (not_enough_place_for_next_event && (readsz<maxsz)) fEOF = true;

         // return file pointer to the begin of event
         io->fseek(fd, -(readsz - checkedsz), true);

         break;
      }
      checkedsz += hdr->GetPaddedSize();
      hdr = (hadaq::HadTu*) ((char*) hdr + hdr->GetPaddedSize());
   }

   // detect end of file by such method
   if ((readsz<maxsz) && (checkedsz == readsz) && !fEOF) fEOF = true;

   *sz = checkedsz;

//   DOUT0("Return size %u", (unsigned) checkedsz);

   return checkedsz>0;
}

/*
std::string hadaq::HldFile::FormatFilename (uint32_t id)
{
   // same
   char buf[128];
   time_t iocTime;
   iocTime = id + HADAQ_TIMEOFFSET;
   strftime(buf, 128, "%y%j%H%M%S", localtime(&iocTime));
   return std::string(buf);
}
*/

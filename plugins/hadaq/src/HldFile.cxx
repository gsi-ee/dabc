// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)     q           *
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
#include "dabc/logging.h"

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

bool hadaq::HldFile::OpenWrite(const char *fname, uint32_t runid, const char *opt)
{
   if (isOpened()) return false;

   if (!fname || *fname == 0) {
      fprintf(stderr, "file name not specified\n");
      return false;
   }

   CheckIO();

   fd = io->fopen(fname, "w", opt);
   if (!fd) {
      fprintf(stderr, "File open failed %s for writing\n", fname);
      return false;
   }

   fReadingMode = false;

   // put here a dummy event into file:
   hadaq::RawEvent evnt;
   evnt.Init(0, runid, EvtId_runStart);
   if(!WriteBuffer(&evnt, sizeof(evnt))) {
      CloseBasicFile();
      return false;
   }

   fRunNumber = runid;

   return true;
}

bool hadaq::HldFile::OpenRead(const char *fname, const char *opt)
{
   if (isOpened()) return false;

   if (!fname || *fname == 0) {
      fprintf(stderr, "file name not specified\n");
      return false;
   }

   CheckIO();

   fd = io->fopen(fname,  "r", opt);
   if (!fd) {
      fprintf(stderr, "File open failed %s for reading\n", fname);
      return false;
   }
   fReadingMode = true;

//   DOUT0("Open HLD file %s for reading", fname);

   hadaq::RawEvent evnt;
   uint32_t size = sizeof(hadaq::RawEvent);

   // printf("starts reading into buf %u isreading %u \n", (unsigned) size, (unsigned)isReading());

   if (!ReadBuffer(&evnt, &size, true)) {
      fprintf(stderr,"Cannot read starting event from file\n");
      CloseBasicFile();
      return false;
   }

   if ((size!=sizeof(hadaq::RawEvent)) || (evnt.GetId() != EvtId_runStart)) {
      fprintf(stderr,"Did not found start event at the file beginning\n");
      CloseBasicFile();
      return false;
   }

//   DOUT0("Find start event at the file begin");

   fRunNumber = evnt.GetRunNr();
   fEOF = false;

   return true;
}

void hadaq::HldFile::Close()
{
   DOUT3("hadaq::HldFile::Close()... ");
  if (isWriting()) {
      // need to add empty terminating event:
      hadaq::RawEvent evnt;
      evnt.Init(0, fRunNumber, EvtId_runStop);
      WriteBuffer(&evnt, sizeof(evnt));
   }

  CloseBasicFile();

  fRunNumber = 0;
  fEOF = true;
}



bool hadaq::HldFile::WriteBuffer(void* buf, uint32_t bufsize)
{
   if (!isWriting() || !buf || (bufsize == 0)) return false;

   if (io->fwrite(buf, bufsize, 1, fd) != 1) {
      EOUT("fail to write buffer payload of size %u", (unsigned) bufsize);
      CloseBasicFile();
      return false;
   }

   return true;
}

bool hadaq::HldFile::ReadBuffer(void* ptr, uint32_t* sz, bool onlyevent)
{
   if (!isReading() || !ptr || !sz || (*sz < sizeof(hadaq::HadTu))) return false;

   uint64_t maxsz = *sz; *sz = 0;

   size_t readsz = io->fread(ptr, 1, (onlyevent ? sizeof(hadaq::HadTu) : maxsz), fd);

   //printf("Read HLD portion of data %u max %u\n", (unsigned) readsz, (unsigned) maxsz);

   if (readsz < sizeof(hadaq::HadTu)) {
      if (!io->feof(fd)) fprintf(stderr, "Fail to read next portion while no EOF detected\n");
      fEOF = true;
      return false;
   }

   hadaq::HadTu* hdr = (hadaq::HadTu*) ptr;


   if (onlyevent) {

      if (hdr->GetPaddedSize() > maxsz) {
         fprintf(stderr, "Buffer %u too small to read next event %u from hld file\n", (unsigned) maxsz, (unsigned)hdr->GetPaddedSize());
         return false;
      }

      // printf("Expect next event of size %u\n", (unsigned) hdr->GetPaddedSize());

      readsz = io->fread((char*) ptr + sizeof(hadaq::HadTu), 1, hdr->GetPaddedSize() - sizeof(hadaq::HadTu), fd);

      // printf("Read size %u expects %u \n", (unsigned) readsz, (unsigned) (hdr->GetPaddedSize() - sizeof(hadaq::HadTu)));

      // not possible to read event completely
      if ( readsz != (hdr->GetPaddedSize() - sizeof(hadaq::HadTu))) {
         fprintf(stderr, "Reading problem\n");
         fEOF = true;
         return false;
      }

      if ((hdr->GetPaddedSize() == sizeof(hadaq::RawEvent)) && (((hadaq::RawEvent*)hdr)->GetId() == EvtId_runStop)) {
         // we are not deliver such stop event to the top

         // printf("Find stop event at the file end\n");
         fEOF = true;
         return false;
      }

      *sz = hdr->GetPaddedSize();
      return true;
   }


   size_t checkedsz = 0;

   while (checkedsz < readsz) {
      // special case when event was read not completely
      // or we want to provide only event

      bool not_enough_place_for_next_event = false;

      size_t restsize = readsz - checkedsz;
      if (restsize < sizeof(hadaq::HadTu))
         not_enough_place_for_next_event = true;
      else {
         restsize = hdr->GetPaddedSize();
         not_enough_place_for_next_event = (checkedsz + restsize) > readsz;
      }

      if ((restsize == sizeof(hadaq::RawEvent)) && (((hadaq::RawEvent*)hdr)->GetId() == EvtId_runStop)) {
         // we are not deliver such stop event to the top
         fEOF = true;
         break;
      }

      if (not_enough_place_for_next_event) {
         if (readsz < maxsz)
            fEOF = true;
         // return file pointer to the begin of event
         io->fseek(fd, -(readsz - checkedsz), true);
         break;
      }

      checkedsz += hdr->GetPaddedSize();
      hdr = (hadaq::HadTu*) ((char*) hdr + hdr->GetPaddedSize());
   }

   // detect end of file by such method
   if ((readsz < maxsz) && (checkedsz == readsz) && !fEOF)
      fEOF = true;

   *sz = checkedsz;

   return checkedsz > 0;
}

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

#include "dogma/DogmaFile.h"

bool dogma::DogmaFile::OpenWrite(const char *fname, const char *opt)
{
   if (isOpened())
      return false;

   if (!fname || !*fname) {
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

   return true;
}

bool dogma::DogmaFile::OpenRead(const char *fname, const char *opt)
{
   if (isOpened()) return false;

   if (!fname || !*fname) {
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

   fEOF = false;

   return true;
}

void dogma::DogmaFile::Close()
{
   CloseBasicFile();

   fEOF = true;
}

bool dogma::DogmaFile::WriteBuffer(void* buf, uint32_t bufsize)
{
   if (!isWriting() || !buf || (bufsize == 0))
     return false;

   if (io->fwrite(buf, bufsize, 1, fd) != 1) {
      fprintf(stderr, "fail to write dogma buffer payload of size %u", (unsigned) bufsize);
      CloseBasicFile();
      return false;
   }

   return true;
}

bool dogma::DogmaFile::ReadBuffer(void* ptr, uint32_t* sz, bool onlyevent)
{
   if (!isReading() || !ptr || !sz || (*sz < sizeof(dogma::DogmaEvent)))
      return false;

   uint64_t maxsz = *sz;
   *sz = 0;

   size_t readsz = io->fread(ptr, 1, (onlyevent ? sizeof(dogma::DogmaEvent) : maxsz), fd);

   if (readsz < sizeof(dogma::DogmaEvent)) {
      if (!io->feof(fd))
         fprintf(stderr, "Fail to read next portion but no EOF detected\n");
      fEOF = true;
      return false;
   }

   auto hdr = (dogma::DogmaEvent *) ptr;

   if (onlyevent) {

      if (hdr->GetEventLen() > maxsz) {
         fprintf(stderr, "Buffer %u too small to read next event %u from dogma file\n", (unsigned) maxsz, (unsigned) hdr->GetEventLen());
         return false;
      }

      readsz = io->fread((char *) ptr + sizeof(dogma::DogmaEvent), 1, hdr->GetPayloadLen()*4, fd);

      // not possible to read event completely
      if (readsz != hdr->GetPayloadLen()*4) {
         fprintf(stderr, "DOGMA reading problem\n");
         fEOF = true;
         return false;
      }

      *sz = hdr->GetEventLen();
      return true;
   }


   size_t checkedsz = 0;

   while (checkedsz < readsz) {
      // special case when event was read not completely
      // or we want to provide only event

      bool not_enough_place_for_next_event = false;
      size_t restsize = readsz - checkedsz;

      if (restsize < sizeof(dogma::DogmaEvent)) {
         not_enough_place_for_next_event = true;
      } else {
         restsize = hdr->GetEventLen();
         not_enough_place_for_next_event = (checkedsz + restsize) > readsz;
      }

      if (not_enough_place_for_next_event) {
         if (readsz < maxsz)
            fEOF = true;

         // return file pointer to the begin of event
         io->fseek(fd, -(readsz - checkedsz), true);

         break;
      }
      checkedsz += hdr->GetEventLen();
      hdr = (dogma::DogmaEvent *) ((char*) hdr + hdr->GetEventLen());
   }

   // detect end of file by such method
   if ((readsz < maxsz) && (checkedsz >= readsz) && !fEOF)
      fEOF = true;

   *sz = checkedsz;

   return checkedsz > 0;
}

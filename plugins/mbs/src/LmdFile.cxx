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

#include "LmdFile.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

extern "C" {
  #include "sMbs.h"
  #include "fLmd.h"
}


mbs::LmdFile::LmdFile()
{
   fMode = mNone;
   fControl = fLmdAllocateControl();
   fLastError = LMD__SUCCESS;
}

mbs::LmdFile::~LmdFile()
{
   Close();

   free((sLmdControl*)fControl);
   fControl = 0;
   fLastError = LMD__SUCCESS;
}

bool mbs::LmdFile::OpenWrite(const char* fname, uint32_t buffersize)
{
   Close();

   fLastError = fLmdPutOpen((sLmdControl*) fControl,
                            (char*) fname,
                            LMD__INTERNAL_HEADER,
                            buffersize,
                            LMD__OVERWRITE,
                            LMD__NO_INDEX,
                            LMD__LARGE_FILE);

   if (fLastError != LMD__SUCCESS) return false;
   fMode = mWrite;
   return true;
}

bool mbs::LmdFile::OpenRead(const char* fname, uint32_t buffersize)
{
   Close();

   if (buffersize < 1024) {
      printf("Buffer size %u too small, use 1024\n", buffersize);
      buffersize = 1024;
   }

   fLastError = fLmdGetOpen((sLmdControl*) fControl,
                            (char*) fname,
                            LMD__INTERNAL_HEADER,
                            buffersize,
                            LMD__NO_INDEX);

   if (fLastError != LMD__SUCCESS) return false;
   fMode = mRead;
   return true;
}

void mbs::LmdFile::Close()
{
   if (IsWriteMode()) {
      fLastError = fLmdPutClose((sLmdControl*) fControl);
   } else
   if (IsReadMode()) {
      fLastError = fLmdGetClose((sLmdControl*) fControl);
   } else
      fLastError = LMD__SUCCESS;

   fMode = mNone;
}

bool mbs::LmdFile::WriteElements(Header* hdr, unsigned num)
{
   if (!IsWriteMode() || (num==0)) {
      fLastError = LMD__FAILURE;
      return false;
   }

   if (num==1)
      fLastError = fLmdPutElement((sLmdControl*) fControl, (sMbsHeader*) hdr);
   else
      fLastError = fLmdPutBuffer((sLmdControl*) fControl, (sMbsHeader*) hdr, num);

   return (fLastError == LMD__SUCCESS);
}

mbs::Header* mbs::LmdFile::ReadElement()
{
   if (!IsReadMode()) {
      fLastError = LMD__FAILURE;
      return 0;
   }

   sMbsHeader* hdr = fLmdGetElement((sLmdControl*) fControl, LMD__NO_INDEX);

   if (hdr==0) Close();

   return (mbs::Header*) hdr;
}

unsigned int mbs::LmdFile::ReadBuffer(void* buf, uint32_t& bufsize)
{
   if (!IsReadMode() || (buf==0) || (bufsize==0)) {
      fLastError = LMD__FAILURE;
      return 0;
   }

   uint32_t numev = 0;

   fLastError = fLmdGetBuffer((sLmdControl*) fControl,
                              (sMbsHeader*) buf, bufsize,
                              &numev, &bufsize);

   if (fLastError == GETLMD__EOFILE) Close();

   return numev;
}

bool mbs::LmdFile::WriteEvents(EventHeader* hdr, unsigned num)
{
   return WriteElements(hdr, num);
}

mbs::EventHeader* mbs::LmdFile::ReadEvent()
{
   return (mbs::EventHeader*) ReadElement();
}

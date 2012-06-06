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




hadaq::HldFile::HldFile()
{
   fMode = mNone;
//   fControl = fLmdAllocateControl();
//   fLastError = HLD__SUCCESS;
}

hadaq::HldFile::~HldFile()
{
   Close();

//   free((sLmdControl*)fControl);
//   fControl = 0;
//   fLastError = HLD__SUCCESS;
}

bool hadaq::HldFile::OpenWrite(const char* fname, uint32_t buffersize)
{
   Close();

//   fLastError = fLmdPutOpen((sLmdControl*) fControl,
//                            (char*) fname,
//                            LMD__INTERNAL_HEADER,
//                            buffersize,
//                            LMD__OVERWRITE,
//                            LMD__NO_INDEX,
//                            LMD__LARGE_FILE);
//
   if (fLastError != HLD__SUCCESS) return false;
   fMode = mWrite;
   return true;
}

bool hadaq::HldFile::OpenRead(const char* fname, uint32_t buffersize)
{
   Close();

   if (buffersize < 1024) {
      printf("Buffer size %u too small, use 1024\n", buffersize);
      buffersize = 1024;
   }

//   fLastError = fLmdGetOpen((sLmdControl*) fControl,
//                            (char*) fname,
//                            LMD__INTERNAL_HEADER,
//                            buffersize,
//                            LMD__NO_INDEX);
//
//   if (fLastError != HLD__SUCCESS) return false;
   fMode = mRead;
   return true;
}

void hadaq::HldFile::Close()
{
   if (IsWriteMode()) {
      //fLastError = fLmdPutClose((sLmdControl*) fControl);
   } else
   if (IsReadMode()) {
      //fLastError = fLmdGetClose((sLmdControl*) fControl);
   } else
      fLastError = HLD__SUCCESS;

   fMode = mNone;
}

bool hadaq::HldFile::WriteElements(hadaq::Event* hdr, unsigned num)
{
   if (!IsWriteMode() || (num==0)) {
      fLastError = HLD__FAILURE;
      return false;
   }

//   if (num==1)
//      //fLastError = fLmdPutElement((sLmdControl*) fControl, (sMbsHeader*) hdr);
//   else
      //fLastError = fLmdPutBuffer((sLmdControl*) fControl, (sMbsHeader*) hdr, num);

   return (fLastError == HLD__SUCCESS);
}

hadaq::Event* hadaq::HldFile::ReadElement()
{
   if (!IsReadMode()) {
      fLastError = HLD__FAILURE;
      return 0;
   }

   //sMbsHeader* hdr = fLmdGetElement((sLmdControl*) fControl, LMD__NO_INDEX);

   //if (hdr==0) Close();

   return 0;// (hadaq::Header*) hdr;
}

unsigned int hadaq::HldFile::ReadBuffer(void* buf, uint32_t& bufsize)
{
   if (!IsReadMode() || (buf==0) || (bufsize==0)) {
      fLastError = HLD__FAILURE;
      return 0;
   }

   uint32_t numev = 0;

//   fLastError = fLmdGetBuffer((sLmdControl*) fControl,
//                              (sMbsHeader*) buf, bufsize,
//                              &numev, &bufsize);

   //if (fLastError == GETHLD__EOFILE) Close();

   return numev;
}

bool hadaq::HldFile::WriteEvents(hadaq::Event* hdr, unsigned num)
{
   return false; //WriteElements(hdr, num);
}

hadaq::Event* hadaq::HldFile::ReadEvent()
{
   return (hadaq::Event*) ReadElement();
}

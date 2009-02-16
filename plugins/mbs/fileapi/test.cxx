/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "LmdFile.h"

#include <stdio.h>

int main(int numc, char* args[])
{
   const char* fname = "test.lmd";

   if (numc>1) fname = args[1];

   printf("Try to open lmd file %s\n", fname);

   mbs::LmdFile f;

   if (!f.OpenRead(fname)) {
      printf("Cannot open file for reading - error %d\n", f.LastError());
      return 1;
   }

   mbs::EventHeader* hdr = 0;

   while ((hdr = f.ReadEvent()) != 0) {

      printf("Event %u length %u\n", hdr->EventNumber(), hdr->SubEventsSize());
   }

}


// $Id$

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

#include <stdio.h>
#include <string.h>

#include "hadaq/api.h"
#include "dabc/string.h"

int usage(const char* errstr = 0)
{
   if (errstr!=0) {
      printf("Error: %s\n\n", errstr);
   }

   printf("utility for printing HLD events\n");
   printf("mbsprint source [args]\n\n");
   printf("Following source kinds are supported:\n");
   printf("   hld://path/file.hld      - HLD file reading\n");
   printf("   mbs://dabcnode/Stream    - DABC stream server\n");
   printf("   mbs://dabcnode/Transport - DABC transport server\n");
   printf("   lmd://path/file.lmd      - LMD file reading\n");
   printf("Additional arguments:\n");
   printf("   -tmout value            - maximal time in second for waiting next event\n");
   printf("   -num number             - number of events to print\n");

   return errstr ? 1 : 0;
}


int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   int number = 10;
   double tmout = 1.;

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &number); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect(argv[1]);

   if (ref.null()) return 1;

   hadaq::RawEvent* evnt(0);

   int cnt = 0;

   while ((evnt = ref.NextEvent(tmout)) != 0) {

      printf("HLD event %p\n", evnt);

      if (++cnt >= number) break;
   }

   ref.Disconnect();

   return 0;
}

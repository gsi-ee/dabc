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
   printf("hldprint source [args]\n\n");
   printf("Following source kinds are supported:\n");
   printf("   hld://path/file.hld         - HLD file reading\n");
   printf("   //path/file.hld             - HLD file reading (file extension MUST be '.hld')\n");
   printf("   mbs://dabcnode/Stream       - DABC stream server\n");
   printf("   mbs://dabcnode:port/Stream  - DABC stream server with custom port\n");
   printf("   mbs://dabcnode/Transport    - DABC transport server\n");
   printf("   lmd://path/file.lmd         - LMD file reading\n");
   printf("Additional arguments:\n");
   printf("   -tmout value            - maximal time in second for waiting next event\n");
   printf("   -num number             - number of events to print (default 10)\n");
   printf("   -sub                    - try to scan for subsub events (default false)\n");
   printf("   -raw                    - printout of raw data (default false)\n");

   return errstr ? 1 : 0;
}


int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   int number = 10;
   double tmout = 1.;
   bool printraw = false;
   bool printsub = false;

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &number); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if (strcmp(argv[n],"-raw")==0) { printraw = true; } else
      if (strcmp(argv[n],"-sub")==0) { printsub = true; } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   std::string src = argv[1];
   if ((src.find(".hld") != std::string::npos) && (src.find("hld://") != 0))
      src = std::string("hld://") + src;

   printf("Try to open %s\n", src.c_str());

   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect(src.c_str());

   if (ref.null()) return 1;

   hadaq::RawEvent* evnt(0);

   int cnt = 0;

   while ((evnt = ref.NextEvent(tmout)) != 0) {

      evnt->Dump();
      hadaq::RawSubevent* sub = 0;
      while ((sub=evnt->NextSubevent(sub))!=0) {
         sub->Dump(printraw && !printsub);

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;
         unsigned ix = 0;

         while ((ix < trbSubEvSize) && printsub) {
            unsigned data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned datakind = data & 0xFFFF;

            printf("      *** Subsubevent size %3u id 0x%04x full %08x\n", datalen, datakind, data);

            if (printraw) sub->PrintRawData(ix,datalen,9);

            ix+=datalen;
         }
      }

      if (++cnt >= number) break;
   }

   ref.Disconnect();

   return 0;
}

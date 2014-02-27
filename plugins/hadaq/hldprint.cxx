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
#include "dabc/Url.h"

int usage(const char* errstr = 0)
{
   if (errstr!=0) {
      printf("Error: %s\n\n", errstr);
   }

   printf("utility for printing HLD events\n");
   printf("hldprint source [args]\n\n");
   printf("Following source kinds are supported:\n");
   printf("   hld://path/file.hld         - HLD file reading\n");
   printf("   file.hld                    - HLD file reading (file extension MUST be '.hld')\n");
   printf("   mbs://dabcnode/Stream       - DABC stream server\n");
   printf("   mbs://dabcnode:port/Stream  - DABC stream server with custom port\n");
   printf("   mbs://dabcnode/Transport    - DABC transport server\n");
   printf("   lmd://path/file.lmd         - LMD file reading\n");
   printf("Additional arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -num number             - number of events to print (default 10)\n");
   printf("   -sub                    - try to scan for subsub events (default false)\n");
   printf("   -raw                    - printout of raw data (default false)\n");
   printf("   -rate                   - display only events rate\n");

   return errstr ? 1 : 0;
}


int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   long number = 10;
   double tmout = 5.;
   bool printraw(false), printsub(false), showrate(false), reconnect(false);

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &number); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if (strcmp(argv[n],"-raw")==0) { printraw = true; } else
      if (strcmp(argv[n],"-sub")==0) { printsub = true; } else
      if (strcmp(argv[n],"-rate")==0) { showrate = true; reconnect = true; } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   printf("Try to open %s\n", argv[1]);

   bool ishld = false;
   std::string src = argv[1];
   if ((src.find(".hld") != std::string::npos) && (src.find("hld://") != 0)) {
      src = std::string("hld://") + src;
      ishld = true;
   }

   if ((src.find("hld://") == 0) || (src.find(".hld") != std::string::npos)) ishld = true;

   if (!ishld) {

      dabc::Url url(src);

      if (url.IsValid()) {
         if (url.GetProtocol().empty())
            src = std::string("mbs://") + src;

         if (url.GetFileName().empty())
            src += "/Stream";

         if (reconnect && !url.HasOption("reconnect")) {
           if (url.GetOptions().empty())
              src+="?reconnect";
           else
              src+="&reconnect";
         }
      }
   }


   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect(src.c_str());

   if (ref.null()) return 1;

   hadaq::RawEvent* evnt(0);

   long cnt(0), lastcnt(0);
   dabc::TimeStamp last = dabc::Now();
   dabc::TimeStamp first = last;

   while ((evnt = ref.NextEvent(tmout)) != 0) {

      cnt++;

      if (showrate) {

         dabc::TimeStamp curr = dabc::Now();
         double tm = curr - last;

         if (tm>=0.3) {
            printf("\rTm:%6.1fs  Ev:%8ld  Rate:%8.2f Ev/s", first.SpentTillNow(), cnt, (cnt-lastcnt)/tm);
            fflush(stdout);
            last = curr;
            lastcnt = cnt;
         }

         continue;
      }

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

      if (cnt >= number) break;
   }

   if (showrate) {
      printf("\n");
      fflush(stdout);
   }

   ref.Disconnect();

   return 0;
}

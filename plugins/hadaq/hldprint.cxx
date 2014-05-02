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
   printf("   mbs://dabcnode              - DABC stream server\n");
   printf("   mbs://dabcnode:port         - DABC stream server with custom port\n");
   printf("   mbs://dabcnode/Transport    - DABC transport server\n");
   printf("   lmd://path/file.lmd         - LMD file reading\n");
   printf("Additional arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -num number             - number of events to print (default 10)\n");
   printf("   -skip number            - number of events to skip before start printing\n");
   printf("   -sub                    - try to scan for subsub events (default false)\n");
   printf("   -raw                    - printout of raw data (default false)\n");
   printf("   -onlyraw subsubid       - printout of raw data only for specified subsubevent\n");
   printf("   -tdc mask               - printout raw data of tdc subsubevents (default none) \n");
   printf("   -onlytdc tdcid          - printout raw data only of specified tdc subsubevent (default none) \n");
   printf("   -fullid value           - printout only events with specified fullid (default all) \n");
   printf("   -hub value              - identify hub inside subevent to printout raw data inside (default none) \n");
   printf("   -rate                   - display only events rate\n");

   return errstr ? 1 : 0;
}

enum TdcMessageKind {
   tdckind_Reserved = 0x00000000,
   tdckind_Header   = 0x20000000,
   tdckind_Debug    = 0x40000000,
   tdckind_Epoch    = 0x60000000,
   tdckind_Mask     = 0xe0000000,
   tdckind_Hit      = 0x80000000,
   tdckind_Hit1     = 0xa0000000,
   tdckind_Hit2     = 0xc0000000,
   tdckind_Hit3     = 0xe0000000
};


void PrintTdcData(hadaq::RawSubevent* sub, unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((sub->GetSize() - sizeof(hadaq::RawSubevent)) / sub->Alignment());

   if (ix>=sz) return;
   if ((len==0) || (ix + len > sz)) len = sz - ix;

   unsigned wlen = 2;
   if (len>99) wlen = 3; else
   if (len>999) wlen = 4;

   unsigned epoch(0);
   double tm;

   for (unsigned cnt=0;cnt<len;cnt++,ix++) {
      unsigned msg = sub->Data(ix);
      printf("%*s[%*u] %08x  ",  prefix, "", wlen, ix, msg);

      switch (msg & tdckind_Mask) {
         case tdckind_Reserved:
            printf("reserved\n");
            break;
         case tdckind_Header:
            printf("tdc header\n");
            break;
         case tdckind_Debug:
            printf("tdc debug\n");
            break;
         case tdckind_Epoch:
            epoch = msg & 0xFFFFFFF;
            tm = (epoch << 11) *5.;
            printf("epoch %u tm %6.3f ns\n", msg & 0xFFFFFFF, tm);
            break;
         case tdckind_Hit:
            tm = ((epoch << 11) + (msg & 0x7FF)) *5.; // coarse time
            tm += (((msg >> 12) & 0x3FF) - 20)/470.*5.; // approx fine time 20-490
            printf("hit ch:%2u isrising:%u tc:0x%03x tf:0x%03x tm:%6.3f ns\n",
                    (msg >> 22) & 0x7F, (msg >> 11) & 0x1, (msg & 0x7FF), (msg >> 12) & 0x3FF, tm);
            break;
         default:
            printf("undefined\n");
            break;
      }
   }
}


int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   long number(10), skip(0);
   double tmout = 5.;
   bool printraw(false), printsub(false), showrate(false), reconnect(false);
   unsigned tdcmask(0), onlytdc(0), onlyraw(0), hubmask(0), fullid(0);

   int n = 1;
   while (++n<argc) {
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &number); } else
      if ((strcmp(argv[n],"-skip")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &skip); } else
      if ((strcmp(argv[n],"-tdc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &tdcmask); } else
      if ((strcmp(argv[n],"-onlytdc")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &onlytdc); } else
      if ((strcmp(argv[n],"-onlyraw")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &onlyraw); } else
      if ((strcmp(argv[n],"-fullid")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &fullid); } else
      if ((strcmp(argv[n],"-hub")==0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &hubmask); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if (strcmp(argv[n],"-raw")==0) { printraw = true; } else
      if (strcmp(argv[n],"-sub")==0) { printsub = true; } else
      if (strcmp(argv[n],"-rate")==0) { showrate = true; reconnect = true; } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   if ((tdcmask!=0) || (onlytdc!=0) || (onlyraw!=0)) { printsub = true; printraw = true; }

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
            src = std::string("mbss://") + src;

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

   long cnt(0), lastcnt(0), printcnt(0);
   dabc::TimeStamp last = dabc::Now();
   dabc::TimeStamp first = last;
   dabc::TimeStamp lastevtm = last;

   while (true) {

      evnt = ref.NextEvent(1.);

      dabc::TimeStamp curr = dabc::Now();

      if (evnt!=0) {
         cnt++;
         lastevtm = curr;
      } else
      if (curr - lastevtm > tmout) break;

      if (showrate) {

         double tm = curr - last;

         if (tm>=0.3) {
            printf("\rTm:%6.1fs  Ev:%8ld  Rate:%8.2f Ev/s", first.SpentTillNow(), cnt, (cnt-lastcnt)/tm);
            fflush(stdout);
            last = curr;
            lastcnt = cnt;
         }

         continue;
      }

      if (evnt==0) continue;

      if ((fullid!=0) && (evnt->GetId()!=fullid)) continue;

      if (skip>0) { skip--; continue; }

      printcnt++;

      evnt->Dump();
      hadaq::RawSubevent* sub = 0;
      while ((sub=evnt->NextSubevent(sub))!=0) {

         bool print_sub_header(false);

         if ((onlytdc==0) && (onlyraw==0)) {
            sub->Dump(printraw && !printsub);
            print_sub_header = true;
         }

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;
         unsigned ix = 0;

         while ((ix < trbSubEvSize) && printsub) {
            unsigned data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned datakind = data & 0xFFFF;

            bool as_raw(false), as_tdc(false);

            if ((onlytdc!=0) && (datakind==onlytdc)) {
               as_tdc = true;
            } else
            if ((tdcmask!=0) && (datakind & 0xff & tdcmask) && ((datakind & 0xff00) == (tdcmask & 0xff00))) {
               as_tdc = true;
            } else
            if ((hubmask!=0) && (datakind==hubmask)) {
               // this is hack - skip hub header, inside is normal subsub events structure
               continue;
            } else
            if ((onlyraw!=0) && (datakind==onlyraw)) {
               as_raw = true;
            } else
            if (printraw) {
               as_raw = (onlytdc==0) && (onlyraw==0);
            }

            if (as_raw || as_tdc) {
               if (!print_sub_header) {
                  sub->Dump(false);
                  print_sub_header = true;
               }

               printf("      *** Subsubevent size %3u id 0x%04x full %08x\n", datalen, datakind, data);
            }

            if (as_tdc) PrintTdcData(sub, ix, datalen,9); else
            if (as_raw) sub->PrintRawData(ix,datalen,9);

            ix+=datalen;
         }
      }

      if (printcnt >= number) break;
   }

   if (showrate) {
      printf("\n");
      fflush(stdout);
   }

   ref.Disconnect();

   return 0;
}

// $Id$

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

#include <cstdio>
#include <cstring>
#include <ctime>

#include "dabc/Url.h"
#include "dabc/api.h"

#include "mbs/api.h"

int usage(const char *errstr = nullptr)
{
   if (errstr) {
      printf("Error: %s\n\n", errstr);
   }

   printf("utility for printing MBS events\n");
   printf("mbsprint source [args]\n\n");
   printf("Following source kinds are supported:\n");
   printf("   lmd://path/file.lmd     - LMD file reading\n");
   printf("   mbs://mbsnode/Transport - MBS transport server\n");
   printf("   mbs://mbsnode/Stream    - MBS stream server\n");
   printf("Additional arguments:\n");
   printf("   -tmout value            - maximal time in seconds for waiting next event (default 5)\n");
   printf("   -maxage value           - maximal age time for events, if expired queue are cleaned (default off)\n");
   printf("   -buf sz                 - buffer size in MB, default 4\n");
   printf("   -num number             - number of events to print\n");
   printf("   -hex                    - print raw data in hex form\n");
   printf("   -dec                    - print raw data in decimal form\n");
   printf("   -long                   - print raw data in 4-bytes format\n");
   printf("   -short                  - print raw data in 2-bytes format\n");
   printf("   -slow subevid           - print subevents with id as slow control record (see mbs/SlowControlData.h file)\n");

   return errstr ? 1 : 0;
}

void PrintSlowSubevent(mbs::SubeventHeader* sub)
{
   mbs::SlowControlData rec;
   if (!rec.Read(sub->RawData(), sub->RawDataSize())) {
      printf("   SlowControl record format failure\n");
      return;
   }

   time_t tm = rec.GetEventTime();

   printf("    SlowControl data evid:%u longs:%u doubles:%u time:%s",
         (unsigned) rec.GetEventId(), rec.NumLongs(), rec.NumDoubles(), ctime(&tm));

   for (unsigned num=0;num<rec.NumLongs();num++) {
      printf("      %s = %ld\n", rec.GetLongName(num).c_str(), (long int) rec.GetLongValue(num));
   }

   for (unsigned num=0;num<rec.NumDoubles();num++) {
      printf("      %s = %f\n", rec.GetDoubleName(num).c_str(), rec.GetDoubleValue(num));
   }
}



int main(int argc, char* argv[])
{
   if (argc < 2)
      return usage();

   long number = 10;
   double tmout = 5., maxage = -1., debug_delay = -1.;
   unsigned slowsubevid = 0;
   int buffer_size = 4;

   bool printdata = false, ashex = true, aslong = true, showrate = false, reconnect = false;

   int n = 1;
   while (++n<argc) {
      if (strcmp(argv[n],"-hex") == 0) { printdata = true; ashex = true; } else
      if (strcmp(argv[n],"-dec") == 0) { printdata = true; ashex = false; } else
      if (strcmp(argv[n],"-long") == 0) { printdata = true; aslong = true; } else
      if (strcmp(argv[n],"-short") == 0) { printdata = true; aslong = false; } else
      if (strcmp(argv[n],"-rate") == 0) { showrate = true; reconnect = true; } else
      if ((strcmp(argv[n],"-num") == 0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &number); } else
      if ((strcmp(argv[n],"-buf") == 0) && (n+1<argc)) { dabc::str_to_int(argv[++n], &buffer_size); } else
      if ((strcmp(argv[n],"-tmout") == 0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if ((strcmp(argv[n],"-maxage") == 0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &maxage); } else
      if ((strcmp(argv[n],"-delay") == 0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &debug_delay); } else
      if ((strcmp(argv[n],"-slow") == 0) && (n+1<argc)) { dabc::str_to_uint(argv[++n], &slowsubevid); } else
      if ((strcmp(argv[n],"-help") == 0) || (strcmp(argv[n],"?") == 0)) return usage(); else
      return usage("Unknown option");
   }

   std::string src = argv[1];
   dabc::Url url(src);

   if (url.IsValid()) {
      if (url.GetProtocol().empty()) {

         if (src.find(".lmd") != std::string::npos)
            src = std::string("lmd://") + src;
         else
            src = std::string("mbss://") + src;
      }

      if (reconnect && !url.HasOption("reconnect")) {
        if (url.GetOptions().empty())
           src+="?reconnect";
        else
           src+="&reconnect";
      }
   }

   mbs::ReadoutHandle ref = mbs::ReadoutHandle::Connect(src, buffer_size);

   if (ref.null()) return 1;

   mbs::EventHeader* evnt = nullptr;

   long cnt = 0, lastcnt = 0;

   dabc::TimeStamp last = dabc::Now();
   dabc::TimeStamp first = last;
   dabc::TimeStamp lastevtm = last;

   dabc::InstallSignalHandlers();

   while (!dabc::CtrlCPressed()) {

      evnt = ref.NextEvent(maxage > 0 ? maxage/2. : 1., maxage);
      if (debug_delay > 0) dabc::Sleep(debug_delay);

      dabc::TimeStamp curr = dabc::Now();

      if (evnt) {
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

      if (!evnt) continue;

      evnt->PrintHeader();
      mbs::SubeventHeader* sub = nullptr;
      while ((sub = evnt->NextSubEvent(sub)) != nullptr) {
         sub->PrintHeader();
         if ((slowsubevid != 0) && (sub->fFullId == slowsubevid)) {
            PrintSlowSubevent(sub);
         } else if (printdata) {
            sub->PrintData(ashex, aslong);
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

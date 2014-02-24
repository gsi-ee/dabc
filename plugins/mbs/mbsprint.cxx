#include <stdio.h>

#include <string.h>

#include "mbs/api.h"
#include "dabc/string.h"
#include "dabc/Url.h"

int usage(const char* errstr = 0)
{
   if (errstr!=0) {
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
   printf("   -num number             - number of events to print\n");
   printf("   -hex                    - print raw data in hex form\n");
   printf("   -dec                    - print raw data in decimal form\n");
   printf("   -long                   - print raw data in 4-bytes format\n");
   printf("   -short                  - print raw data in 2-bytes format\n");

   return errstr ? 1 : 0;
}


int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   long number = 10;
   double tmout = 5.;


   bool printdata(false), ashex(true), aslong(true), showrate(false);

   int n = 1;
   while (++n<argc) {
      if (strcmp(argv[n],"-hex")==0) { printdata = true; ashex = true; } else
      if (strcmp(argv[n],"-dec")==0) { printdata = true; ashex = false; } else
      if (strcmp(argv[n],"-long")==0) { printdata = true; aslong = true; } else
      if (strcmp(argv[n],"-short")==0) { printdata = true; aslong = false; } else
      if (strcmp(argv[n],"-rate")==0) { showrate = true; } else
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { dabc::str_to_lint(argv[++n], &number); } else
      if ((strcmp(argv[n],"-tmout")==0) && (n+1<argc)) { dabc::str_to_double(argv[++n], &tmout); } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }

   std::string src = argv[1];
   dabc::Url url(src);

   if (url.IsValid()) {
      if (url.GetProtocol().empty())
         src = std::string("mbs://") + src;

      if (url.GetFileName().empty())
         src += "/Stream";
   }

   mbs::ReadoutHandle ref = mbs::ReadoutHandle::Connect(src);

   if (ref.null()) return 1;

   mbs::EventHeader* evnt(0);

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

      evnt->PrintHeader();
      mbs::SubeventHeader* sub = 0;
      while ((sub = evnt->NextSubEvent(sub)) != 0) {
         sub->PrintHeader();
         if (printdata) sub->PrintData(ashex, aslong);
      }

      if (cnt >= number) break;
   }

   ref.Disconnect();

   return 0;
}

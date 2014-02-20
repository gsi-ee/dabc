#include "mbs/api.h"

#include <stdio.h>

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

   int number = 10;

   bool printdata(false), ashex(true), aslong(true);

   int n = 1;
   while (++n<argc) {
      if (strcmp(argv[n],"-hex")==0) { printdata = true; ashex = true; } else
      if (strcmp(argv[n],"-dec")==0) { printdata = true; ashex = false; } else
      if (strcmp(argv[n],"-long")==0) { printdata = true; aslong = true; } else
      if (strcmp(argv[n],"-short")==0) { printdata = true; aslong = false; } else
      if ((strcmp(argv[n],"-num")==0) && (n+1<argc)) { number = atoi(argv[n+1]); n++; } else
      if ((strcmp(argv[n],"-help")==0) || (strcmp(argv[n],"?")==0)) return usage(); else
      return usage("Unknown option");
   }


   mbs::ReadoutHandle ref = mbs::ReadoutHandle::Connect(argv[1]);

   if (ref.null()) return 1;


   mbs::EventHeader* evnt(0);

   int cnt = 0;

   while ((evnt = ref.NextEvent()) != 0) {

      evnt->PrintHeader();
      mbs::SubeventHeader* sub = 0;
      while ((sub = evnt->NextSubEvent(sub)) != 0) {
         sub->PrintHeader();
         if (printdata) sub->PrintData(ashex, aslong);
      }

      if (++cnt >= number) break;
   }

   ref.Disconnect();

   return 0;
}

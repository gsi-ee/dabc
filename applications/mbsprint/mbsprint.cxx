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
   printf("   -hex                    - print data in hex format\n");
   printf("   -long                   - print data in long format\n");
   printf("   -raw                    - print data in raw format\n");

   return errstr ? 1 : 0;
}


int main(int argc, char* argv[])
{
   if (argc<2) return usage();

   int number = 10;
   int printkind = 0;

   int n = 1;
   while (++n<argc) {
      if (strcmp(argv[n],"-hex")==0) printkind = 1; else
      if (strcmp(argv[n],"-long")==0) printkind = 2; else
      if (strcmp(argv[n],"-raw")==0) printkind = 3; else
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
         switch (printkind) {
            case 1: sub->PrintHex(); break;
            case 2: sub->PrintLong(); break;
            case 3: sub->PrintData(); break;
         }
      }

      if (++cnt >= number) break;
   }

   ref.Disconnect();

   return 0;
}

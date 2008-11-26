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


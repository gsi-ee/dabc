// this is an example of reading lmd file with nxyter::Data

// One should have libKnut.so and libLmdFile.so libraries compiled
// (with ROOT configured) and LD_LIBRARY_PATH should point to their location
// These libraries should be automatically loaded when script starts
// (if not, uncomment gSystem->Load() lines in script beginning)


// uncomment includes to use script with ACLiC
//#include "LmdFile.h"
//#include "nxyter/Data.h"
//#include <stdio.h>



void readfile(const char* fname = "test.lmd")
{
   fname = "/misc/linev/rawdata/

// gSystem->Load("libKnut.so");
// gSystem->Load("libLmdFile.so");

   mbs::LmdFile f;
   if (!f.OpenRead(fname)) return;

   mbs::EventHeader* hdr = 0;
   mbs::SubeventHeader* subev = 0;
   nxyter::Data* data = 0;
   unsigned numdata = 0;

   long totalsz = 0;
   unsigned numevents = 0;

   long numhits = 0;
   long numaux = 0;

   // this is the loop over all events in the file
   while ((hdr = f.ReadEvent()) != 0) {
      numevents++;
      totalsz += hdr->FullSize();

      subev = 0;

      // this is the loop over all subevents in one event
      while ((subev = hdr->NextSubEvent(subev)) != 0) {

         data = (nxyter::Data*) subev->RawData();
         numdata = subev->RawDataSize() / 6;

         // this is the loop over all messages in subevent
         while (numdata--) {
            // uncomment this line to print all data
            // data->printData(3);

            if (data->isHitMsg()) numhits++; else
            if (data->isAuxMsg()) numaux++;

            data++;
         }

      }

   }

   printf("Overall events: %u total size:%ld\n", numevents, totalsz);
   printf("Found : %ld hits, %ld aux \n", numhits, numaux);

   f.Close();
}

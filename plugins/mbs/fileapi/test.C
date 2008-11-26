// this is test ROOT macro, which shows how to use lmd file in ROOT script
// this is way how one can open file and iterate over all event in it

// uncomment this line to be able compile macro with ACLiC, calling root -l test.C+ -q
//#include "LmdFile.h"

void test(const char* fname = "test_0000.lmd")
{
   mbs::LmdFile f;
   if (!f.OpenRead(fname)) return;

   mbs::EventHeader* hdr = 0;

   long totalsz = 0;
   unsigned numevents = 0;

   while ((hdr = f.ReadEvent()) != 0) {
      numevents++;
      totalsz += hdr->FullSize();
//      printf("Event %u size %u\n", hdr->EventNumber(), hdr->FullSize());
   }

   printf("Overall events: %u total size:%ld\n", numevents, totalsz);

   f.Close();

   cout << "File closed" << endl;
}

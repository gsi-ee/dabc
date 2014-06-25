#include "hadaq/api.h"

int main(int argc, char** argv) {

   const char* filename = "file.hld";
   if (argc>1) filename = argv[1];

   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect(filename);
   hadaq::RawEvent* evnt = 0;
   while((evnt = ref.NextEvent(1.)) != 0) {
      // any user code here
      evnt->Dump();

      hadaq::RawSubevent* sub = 0;
      while ((sub=evnt->NextSubevent(sub))!=0) {

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;
         unsigned ix = 0;

         while (ix < trbSubEvSize) {
            unsigned data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned datakind = data & 0xFFFF;

            ix+=datalen;
         }
      }
   }

   return 0;

}

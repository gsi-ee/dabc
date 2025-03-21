#include "hadaq/api.h"

int main(int argc, char** argv) {

   const char *filename = "file.hld";
   if (argc>1) filename = argv[1];

   hadaq::ReadoutHandle ref = hadaq::ReadoutHandle::Connect(filename);
   while(auto evnt = ref.NextEvent(1.)) {
      // any user code here
      evnt->Dump();

      hadaq::RawSubevent* sub = nullptr;
      while ((sub = evnt->NextSubevent(sub)) != nullptr) {

         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;
         unsigned ix = 0;

         while (ix < trbSubEvSize) {
            unsigned data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            // unsigned datakind = data & 0xFFFF;

            ix+=datalen;
         }
      }
   }

   return 0;

}

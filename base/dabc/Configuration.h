#ifndef DABC_Configuration
#define DABC_Configuration

#ifndef DABC_XmlEngine
#include "dabc/XmlEngine.h"
#endif

namespace dabc {

   class XdaqConfiguration {
      public:
         XdaqConfiguration(const char* fname, bool showerr = true);
         ~XdaqConfiguration();

         bool IsOk() const { return fDoc!=0; }

         XMLNodePointer_t FindContext(unsigned instance = 0, bool showerr = true);
         unsigned NumNodes();
         String NodeName(unsigned instance = 0);
         bool LoadLibs(unsigned instance = 0);
         bool ReadPars(unsigned instance = 0);

      protected:
         XmlEngine         fXml;
         XMLDocPointer_t   fDoc;
   };


}

#endif

#ifndef DABC_RootTreeOutput
#define DABC_RootTreeOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

// This is class to access TTree interface from the DABC

class TTree;

namespace dabc_root {

   class RootTreeOutput : public dabc::DataOutput {
      public:

         RootTreeOutput();
         virtual ~RootTreeOutput();

         virtual bool WriteBuffer(dabc::Buffer* buf);

      protected:

         TTree*       fTree;
   };

}

#endif

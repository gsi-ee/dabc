#ifndef DABC_RootTreeOutput
#define DABC_RootTreeOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

// This is class to access TTree intreface from the DABC

class TTree;   

namespace dabc {
   
   class RootTreeOutput : public DataOutput {
      public:
      
         RootTreeOutput();
         virtual ~RootTreeOutput();
         
         virtual bool WriteBuffer(dabc::Buffer* buf);

      protected: 
      
         TTree*       fTree;
   };
   
}

#endif

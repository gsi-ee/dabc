#ifndef MBS_MbsDataInput
#define MBS_MbsDataInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

namespace mbs {
   
   class MbsDataInput : public dabc::DataInput {
      public:
         MbsDataInput();
         virtual ~MbsDataInput();
         
         void SetTimeout(int timeout = -1) { fTimeout = timeout; }
         
         bool Open(const char* server, int port = 6000);
         bool Close();
         
         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer* buf);
         
      protected:  
         int    fSocket;  // -1 - not connected
         int    fTimeout;
         bool   fSwapping;
      
         sMbsTransportInfo   fInfo;   
         sMbsBufferHeader    fBufHdr;
   };
}

#endif

#ifndef DABC_DataIOTransport
#define DABC_DataIOTransport

#ifndef DABC_DataTransport
#include "dabc/DataTransport.h"
#endif

namespace dabc {
   
   class DataIOTransport : public DataTransport {
      
      public:  
         DataIOTransport(Device* dev, Port* port, DataInput* inp, DataOutput* out);
         virtual ~DataIOTransport();

      protected:
      
         virtual unsigned Read_Size();
         virtual unsigned Read_Start(Buffer* buf);
         virtual unsigned Read_Complete(Buffer* buf);
         virtual double Read_Timeout();
         
         virtual bool WriteBuffer(Buffer* buf);
         virtual void FlushOutput();
         
         // these are extra methods for handling inputs/outputs
         virtual void CloseInput();
         virtual void CloseOutput();
      
         DataInput         *fInput; 
         DataOutput        *fOutput;
   };

};

#endif

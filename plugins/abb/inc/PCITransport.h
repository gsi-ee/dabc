#ifndef DABC_PCITransport
#define DABC_PCITransport

#include "dabc/DataTransport.h"

namespace mprace {
class DMABuffer;

}

namespace dabc {
   class PCIBoardDevice; 
   class AbbDevice;  

   class PCITransport : public DataTransport {
      friend class PCIBoardDevice;  
      
      public:  
         PCITransport(PCIBoardDevice* dev, Port* port);
         virtual ~PCITransport();
         
      protected:    
         
         virtual unsigned Read_Size(); 
         
         virtual unsigned Read_Start(Buffer* buf); 
         
         virtual unsigned Read_Complete(Buffer* buf); 
         
         virtual bool WriteBuffer(Buffer* buf); 
         
         virtual void ProcessPoolChanged(MemoryPool* pool);
         

           PCIBoardDevice* fPCIDevice;
              
   };

}

#endif

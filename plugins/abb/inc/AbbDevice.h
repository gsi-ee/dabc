#ifndef DABC_AbbDevice
#define DABC_AbbDevice

#include "dabc/PCIBoardDevice.h"
#include "mprace/DMAEngineWG.h"

namespace dabc {
   
  
   
   
   class AbbDevice : public PCIBoardDevice {
       
           
      public:
         
         AbbDevice(Basic* parent, const char* name, bool enabledma, unsigned int devid);
         virtual ~AbbDevice();
         
         
         virtual int ReadPCI(dabc::Buffer* buf);
         
        virtual bool ReadPCIStart(dabc::Buffer* buf);
        
        virtual int ReadPCIComplete(dabc::Buffer* buf);
        
        virtual const char* ClassName() const { return "AbbDevice"; }
        
        
         
         unsigned int GetDeviceNumber(){return fDeviceNum;}
         
         void SetEventCount(uint64_t val){fEventCnt=val;}
         void SetID(uint64_t id){fUniqueId=id;}
         
      protected:   
         virtual bool DoDeviceCleanup(bool full = false);
         
         bool WriteBnetHeader(dabc::Buffer* buf);

      private:
      
      /** number X of abb device (/dev/fpgaX) */  
      unsigned int fDeviceNum;
      
      /** access the dma engine directly for wait */
      mprace::DMAEngineWG* fDMAengine;
      
      /** current dma read buffer*/  
      mprace::DMABuffer* fReadDMAbuf;
      
            
      /** event counter for emulated event headers*/
      uint64_t                  fEventCnt;
      /** id number for emulated event headers*/
      uint64_t                  fUniqueId;
        
   };

}

#endif

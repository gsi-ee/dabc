#ifndef ROC_RocTransport
#define ROC_RocTransport

#include "dabc/DataTransport.h"

class SysCoreBoard;

namespace roc {
   class RocDevice; 

   class RocTransport : public dabc::DataTransport {
      friend class RocDevice;  
      
      public:  
         RocTransport(roc::RocDevice* dev, dabc::Port* port, int bufsize, const char* boardIp, int trWindow);
         virtual ~RocTransport();
         
         SysCoreBoard* GetBoard() const { return fxBoard; }
         
         void ComplteteBufferReading();
         
      protected:    
         
         virtual void StartTransport();

         virtual void StopTransport();
         
         
         virtual unsigned Read_Size(); 
         
         virtual unsigned Read_Start(dabc::Buffer* buf); 
         
         virtual unsigned Read_Complete(dabc::Buffer* buf); 
         
         virtual double GetTimeout() { return 0.01; }
                  
      private:

         /** backpointer to owning device*/   
         RocDevice* fxRocDevice;

         /** shortcut pointer to board object for this transport */
         SysCoreBoard* fxBoard;
         
         /** index of board in controller */
         unsigned fuBoardId;
         
         int      fBufferSize; // configured buffer size
         int      fTransWidnow; // transport window
         int      fReqNumMsgs; // required number of messages
         
         // for integrity checks
         bool     fIntegrityChecks;
         unsigned fLastEvent;
         bool     fIsLastEvent;
         unsigned fLastEventStep;
         unsigned fLastEpoch;
   };

}

#endif

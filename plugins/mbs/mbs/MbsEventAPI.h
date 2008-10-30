#ifndef MBS_MbsEventAPI
#define MBS_MbsEventAPI

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

// This is wrapper for standard MBS event API

namespace mbs {
   
   class MbsEventInput : public dabc::DataInput {
      public:
         MbsEventInput();
         virtual ~MbsEventInput();
         
         void SetTimeout(int timeout = -1) { fTimeout = timeout; }
         
         bool OpenFile(const char* fname, const char* tagfile = 0);
         bool OpenRemoteEventServer(const char* name, int port);
         bool OpenStreamServer(const char* name);
         bool OpenEventServer(const char* name);
         bool OpenTransportServer(const char* name);
         bool OpenRFIOFile(const char* name);
         bool Close();
         
         void* GetEventBuffer();
         unsigned GetEventBufferSize();
         
         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer* buf);
      
      protected:  

         bool Open(int mode, const char* fname);
        
         int fMode;  //   Mode of operation, -1 - not active
      
         /** Event channel structure used by event source. */
         void* fChannel; // s_evt_channel*
        
         void* fEventHeader;  // s_ve10_1*

         /** Points to the current gsi buffer structure filled by the event source. */
         void* fBufferHeader; // s_bufhe*

         /** Reference to header info delivered by source. */
         char* fInfoHeader; //  s_filhe*

         /** Timeout in seconds for mbs getevent. If -1, no timeout (default)  */
         int fTimeout;
         
         /** Indicates, if input file stream is normally finished
           * One can only close input at the end */
         bool fFinished; 
   };
   
   class MbsEventOutput : public dabc::DataOutput {
      public:
      
         MbsEventOutput();
         virtual ~MbsEventOutput();
         
         bool CreateOutputFile(const char* name);
         bool Close();
      
         virtual bool WriteBuffer(dabc::Buffer* buf);

      protected: 
      
         int fMode; // current working mode, -1 - not active
      
         void* fChannel; // s_evt_channel*

   };
   
}

#endif

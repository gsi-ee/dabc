#ifndef MBS_LmdFile
#define MBS_LmdFile

#include "LmdTypeDefs.h"

namespace mbs {
    
   class LmdFile {
      protected:  
         enum EMode { mNone, mWrite, mRead, mError };
      
         EMode      fMode;
         void      *fControl;
         uint32_t   fLastError;

      public:
         LmdFile();
         virtual ~LmdFile();
         
         uint32_t LastError() const { return fLastError; }

         bool OpenWrite(const char* fname, uint32_t buffersize = 0x10000);

         /** Opened file for reading. Internal buffer required 
           * when data read partialy and must be kept there. */
         bool OpenRead(const char* fname, uint32_t buffersize = 0x10000);
         
         void Close();
         
         bool WriteEvents(EventHeader* hdr, unsigned num = 1);

         /** Read next event from file. File must be opened by method OpenRead(),
           * Data will be copied first in internal buffer and than provided to user. */
         mbs::EventHeader* ReadEvent();
         
         /** Read one or several events to provided user buffer
           * When called, bufsize should has availible buffer size,
           * after call contains actual size read.
           * Returns read number of events. */
         unsigned int ReadBuffer(void* buf, uint32_t& bufsize); 
         
         
         bool IsWriteMode() const { return fMode == mWrite; }
         bool IsReadMode() const { return fMode == mRead; }
   };    
    
} // end of namespace

#endif

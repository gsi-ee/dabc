#ifndef MBS_LmdOutput
#define MBS_LmdOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef MBS_LmdFile
#include "mbs/LmdFile.h"
#endif

namespace mbs {

   class LmdOutput : public dabc::DataOutput {
      public:
         LmdOutput(const char* fname = 0,
                   unsigned sizelimit_mb = 0,   // maximum size of the file in MB
                   int nmultiple = 0,           // number of multriple instances 0 - no, <0 - any, >0 - exact number
                   int firstmultiple = 0);      // first id of multiple files
         virtual ~LmdOutput();

         virtual bool Write_Init(dabc::Command* cmd = 0, dabc::WorkingProcessor* port = 0);

         virtual bool WriteBuffer(dabc::Buffer* buf);

         bool Init();

         bool IsAllowedMultipleFiles() const { return fNumMultiple!=0; }

      protected:

         bool Close();
         bool StartNewFile();

         std::string         fFileName;
         uint64_t            fSizeLimit;
         int                 fNumMultiple;
         int                 fFirstMultiple;

         int                 fCurrentFileNumber;
         std::string         fCurrentFileName;

         mbs::LmdFile        fFile;
         uint64_t            fCurrentSize;
   };
}

#endif

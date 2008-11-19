#ifndef MBS_LmdInput
#define MBS_LmdInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef MBS_LmdFile
#include "mbs/LmdFile.h"
#endif

namespace mbs {
   
   class LmdInput : public dabc::DataInput {
      public:
         LmdInput(const char* fname, 
                  int nummulti = 0,
                  int firstmulti = 0);
         virtual ~LmdInput();
         
         bool Init();
         
         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer* buf);
         
      protected:  
         bool CloseFile();
         
         bool OpenNextFile();

         std::string        fFileName;
         int                 fNumMultiple;
         int                 fFirstMultiple;
         
         dabc::Folder*       fFilesList;
         
         mbs::LmdFile        fFile;
         std::string        fCurrentFileName;
   };
   
}

#endif

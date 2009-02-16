/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
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
         LmdInput(const char* fname = 0, uint32_t bufsize = 0x10000);
         virtual ~LmdInput();

         virtual bool Read_Init(dabc::Command* cmd = 0, dabc::WorkingProcessor* port = 0);

         bool Init();

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer* buf);

      protected:
         bool CloseFile();

         bool OpenNextFile();

         std::string         fFileName;
         uint32_t            fBufferSize;

         dabc::Folder*       fFilesList;

         mbs::LmdFile        fFile;
         std::string         fCurrentFileName;
         uint64_t            fCurrentRead;
   };

}

#endif

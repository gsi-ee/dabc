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
#ifndef MBS_LmdOutput
#define MBS_LmdOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef MBS_LmdFile
#include "mbs/LmdFile.h"
#endif

namespace mbs {

   class LmdOutput : public dabc::DataOutput {
      public:
         LmdOutput(const char* fname = 0, unsigned sizelimit_mb = 0);
         virtual ~LmdOutput();

         virtual bool Write_Init(dabc::Command* cmd = 0, dabc::WorkingProcessor* port = 0);

         virtual bool WriteBuffer(dabc::Buffer* buf);

         bool Init();

      protected:

         std::string FullFileName(std::string extens);

         bool Close();
         bool StartNewFile();
         void ShowInfo(const char* info, int force = 1);

         std::string         fFileName;
         uint64_t            fSizeLimit;

         int                 fCurrentFileNumber;
         std::string         fCurrentFileName;

         mbs::LmdFile        fFile;
         uint64_t            fCurrentSize;

         dabc::WorkingProcessor* fInfoPort;     // port used to display information about file storage
         std::string         fInfoPrefix;   // prefix used for info field
         dabc::TimeStamp_t   fLastInfoTime; // last time info was updated
   };
}

#endif

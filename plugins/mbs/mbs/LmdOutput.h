/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

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
      protected:

         std::string         fFileName;
         std::string         fInfoName;  // parameter name for info settings
         dabc::TimeStamp     fInfoTime;  // time when last info was shown
         uint64_t            fSizeLimit;

         int                 fCurrentFileNumber;
         std::string         fCurrentFileName;

         mbs::LmdFile        fFile;
         uint64_t            fCurrentSize;

         uint64_t            fTotalSize;
         uint64_t            fTotalEvents;

         std::string FullFileName(std::string extens);

         bool Close();
         bool StartNewFile();
         void ShowInfo(const std::string& info, int priority = 0);

      public:

         LmdOutput(const char* fname = 0, unsigned sizelimit_mb = 0);
         virtual ~LmdOutput();

         virtual bool Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual bool WriteBuffer(const dabc::Buffer& buf);

         bool Init();
   };
}

#endif

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

#ifndef HADAQ_HldInput
#define HADAQ_HldInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef HADAQ_HldFile
#include "hadaq/HldFile.h"
#endif

namespace hadaq {

   class HldInput : public dabc::DataInput {
      public:
         HldInput(const char* fname = 0, uint32_t bufsize = 0x10000);
         virtual ~HldInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         bool Init();

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);

         // alternative way to read hadaq events from HldInput - no any dabc buffer are used
         hadaq::Event* ReadEvent();

      protected:
         bool CloseFile();

         bool OpenNextFile();

         std::string         fFileName;
         uint32_t            fBufferSize;

         dabc::Object*       fFilesList;

         hadaq::HldFile        fFile;
         std::string         fCurrentFileName;
         uint64_t            fCurrentRead;

         bool fLastBuffer; // flag to remember that we reached last buffer after EOF
   };

}

#endif

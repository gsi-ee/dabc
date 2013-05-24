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

#ifndef MBS_LmdInput
#define MBS_LmdInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef MBS_LmdFile
#include "mbs/LmdFile.h"
#endif


#ifndef MBS_LmdFileNew
#include "mbs/LmdFileNew.h"
#endif

namespace mbs {

   /** \brief Input for LMD files (lmd:) */

   class LmdInput : public dabc::FileInput {
      protected:

         mbs::LmdFile        fFile;

         bool CloseFile();

         bool OpenNextFile();

      public:
         LmdInput(const dabc::Url& url);
         virtual ~LmdInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);

         // alternative way to read mbs events from LmdInput - no any dabc buffer are used
         mbs::EventHeader* ReadEvent();

   };

   // ===============================================================================

   /** \brief Input for new LMD files (lmd2:) */

   class LmdInputNew : public dabc::FileInput {
       protected:

          mbs::LmdFileNew      fFile;

          bool CloseFile();

          bool OpenNextFile();

       public:
          LmdInputNew(const dabc::Url& url);
          virtual ~LmdInputNew();

          virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

          virtual unsigned Read_Size();
          virtual unsigned Read_Complete(dabc::Buffer& buf);
    };

}

#endif

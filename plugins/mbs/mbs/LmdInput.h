// $Id$

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

#ifndef MBS_LmdFile
#include "mbs/LmdFile.h"
#endif

namespace mbs {

   /** \brief Input for LMD files (lmd:) */

   class LmdInput : public dabc::FileInput {
       protected:

          mbs::LmdFile   fFile;

          bool CloseFile();

          bool OpenNextFile();

          std::string GetListFileExtension() override { return ".lml"; }

       public:
          LmdInput(const dabc::Url& url);
          virtual ~LmdInput();

          bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd) override;

          unsigned Read_Size() override;
          unsigned Read_Complete(dabc::Buffer& buf) override;
    };

}

#endif

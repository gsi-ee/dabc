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

#ifndef OLMD_LmdInput
#define OLMD_LmdInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif


#ifndef OLMD_OlmdFile
#include "olmd/OlmdFile.h"
#endif


namespace olmd {

   /** \brief Input for LMD files (lmd:) */

   class OlmdInput : public dabc::FileInput {
      protected:

         olmd::OlmdFile        fFile;

         bool CloseFile();

         bool OpenNextFile();

         std::string GetListFileExtension() override { return ".lml"; }

      public:
         OlmdInput(const dabc::Url& url);
         virtual ~OlmdInput();

         bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd) override;

         unsigned Read_Size() override;
         unsigned Read_Complete(dabc::Buffer& buf) override;
   };


}

#endif

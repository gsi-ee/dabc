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

#ifndef DOGMA_DogmaInput
#define DOGMA_DogmaInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DOGMA_DogmaFile
#include "dogma/DogmaFile.h"
#endif

namespace dogma {

   /** \brief Implementation of file input for DOGMA files */

   class DogmaInput : public dabc::FileInput {
      protected:

         dogma::DogmaFile   fFile;

         bool CloseFile();
         bool OpenNextFile();

         std::string GetListFileExtension() override { return ".dogmall"; }

      public:
         DogmaInput(const dabc::Url& ulr);
         ~DogmaInput() override;

         bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd) override;

         unsigned Read_Size() override;
         unsigned Read_Complete(dabc::Buffer& buf) override;
   };

}

#endif

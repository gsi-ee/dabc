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

#ifndef HADAQ_HldInput
#define HADAQ_HldInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef HADAQ_HldFile
#include "hadaq/HldFile.h"
#endif

namespace hadaq {

   /** \brief Implementation of file input for HLD files */

   class HldInput : public dabc::FileInput {
      protected:

         hadaq::HldFile   fFile;

         bool CloseFile();
         bool OpenNextFile();

      public:
         HldInput(const dabc::Url& ulr);
         virtual ~HldInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);
   };

}

#endif

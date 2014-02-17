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

#ifndef HADAQ_HldOutput
#define HADAQ_HldOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef HADAQ_HldFile
#include "hadaq/HldFile.h"
#endif

namespace hadaq {

   /** \brief Implementation of file output for HLD files */

   class HldOutput : public dabc::FileOutput {
      protected:

         bool                fEpicsSlave;     // true if run id is controlled by epics master
         bool                fHadesFileNames; // if true, use hades filename stye (prefix + run number), otherwise plain dabc style (direct filename + cycle number)
         uint32_t            fRunNumber;      // id number of current run
         uint16_t	           fEBNumber;       // id of parent event builder process

         dabc::Parameter     fRunidPar;
         dabc::Parameter     fBytesWrittenPar;

         hadaq::HldFile      fFile;

         bool CloseFile();
         bool StartNewFile();

         uint32_t GetRunId();

         /* evaluate file name with hades timestamp*/
         void SetFullHadesFileName();

         bool Init();

      public:

         HldOutput(const dabc::Url& url);
         virtual ~HldOutput();

         virtual bool Write_Init();

         virtual unsigned Write_Buffer(dabc::Buffer& buf);
   };
}

#endif

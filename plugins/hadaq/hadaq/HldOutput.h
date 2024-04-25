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

#ifndef HADAQ_HldOutput
#define HADAQ_HldOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef HADAQ_HldFile
#include "hadaq/HldFile.h"
#endif

namespace hadaq {

   /** \brief Implementation of file output for HLD files */

   class HldOutput : public dabc::FileOutput {
      protected:

         bool                fRunSlave{false};       ///< true if run id is controlled by combiner
         uint32_t            fLastRunNumber{0};      ///< id number of last written run
         uint32_t            fRunNumber{0};          ///< id number of current run (can be 0 when data are ignored)
         uint32_t            fEventNumber{0};        ///< used to count event numbers when writing plain subevents
         uint16_t            fEBNumber{0};           ///< id of parent event builder process
         bool                fUseDaqDisk{false};     ///< true if /data number is taken from daq_disk (HADES setup)
         bool                fRfio{false};           ///< true if we write to rfio
         bool                fLtsm{false};           ///< true if we write to ltsm
         bool                fPlainName{false};      ///< if true no any runid extensions appended to file name
         std::string         fUrlOptions;            ///< remember URL options, may be used for RFIO file open
         std::string         fLastPrefix;            ///< last prefix submitted from BNet master

         hadaq::HldFile      fFile;

         bool CloseFile();
         bool StartNewFile();

      public:

         HldOutput(const dabc::Url& url);
         virtual ~HldOutput();

         bool Write_Init() override;

         bool Write_Retry() override;

         unsigned Write_Buffer(dabc::Buffer& buf) override;

         bool Write_Stat(dabc::Command cmd) override;

         bool Write_Restart(dabc::Command cmd) override;
   };
}

#endif

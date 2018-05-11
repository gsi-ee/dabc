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

         bool                fEpicsSlave;     ///< true if run id is controlled by epics master
         uint32_t            fRunNumber;      ///< id number of current run
         uint16_t            fEBNumber;       ///< id of parent event builder process
         bool                fUseDaqDisk;     ///< true if /data number is taken from daq_disk (HADES setup)
         bool                fDisabled;       ///< when true, all buffers are discarded
         bool                fRfio;           ///< true if we write to rfio
         bool                fLtsm;           ///< true if we write to ltsm
         std::string         fUrlOptions;     ///< remember URL options, may be used for RFIO file open

         std::string         fRunInfoToOraFilename;

         dabc::TimeStamp     fLastUpdate;     ///< time when last parameter update was performed

         hadaq::HldFile      fFile;

         bool CloseFile();
         bool StartNewFile();

         /* Methods to export run begin to oracle via text file*/
         void StoreRunInfoStart();

         /* Methods to export run end and statistics to oracle via text file*/
         void StoreRunInfoStop();

         /* stolen from daqdata/hadaq/logger.c to keep oracle export output format of numbers*/
         char* Unit(unsigned long v);

      public:

         HldOutput(const dabc::Url& url);
         virtual ~HldOutput();

         virtual bool Write_Init();

         virtual bool Write_Retry();

         virtual unsigned Write_Buffer(dabc::Buffer& buf);

         virtual bool Write_Stat(dabc::Command cmd);

         virtual bool Write_Restart(dabc::Command cmd);
   };
}

#endif

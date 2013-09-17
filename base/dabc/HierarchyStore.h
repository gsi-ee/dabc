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

#ifndef DABC_HierarchyStore
#define DABC_HierarchyStore

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

namespace dabc {

   class HierarchyStore {
      protected:
         std::string fBasePath;
         FileInterface* fIO;      // file interface
         dabc::BinaryFile fFile; // file used to write data
         TimeStamp   fLastStoreTm;   ///! time when last store was done
         TimeStamp   fLastFlushTm;   ///! time when last store flush was done

         bool      fDoStore;   ///! indicate if store diff should be done
         bool      fDoFlush;   ///! indicate if store flush should be done
         uint64_t  fLastVersion; ///! last stored version

         Buffer    fStoreBuf;
         Buffer    fFlushBuf;

      public:
         HierarchyStore();
         virtual ~HierarchyStore();

         /** \brief Set base path for data storage, can only be changed when all files are closed */
         bool SetBasePath(const std::string& path);


         bool StartFile(dabc::Buffer buf);

         bool WriteDiff(dabc::Buffer buf);

         bool CloseFile();

         /** \brief Returns true if any new store action should be performed */
         bool CheckForNextStore(TimeStamp& now, double store_period, double flush_period);

         /** \brief Extract data which is can be stored, must be called under hierarchy mutex */
         bool ExtractData(dabc::Hierarchy& h);

         /** \brief Write extracted data to files, performed outside hierarchy mutex */
         bool WriteExtractedData();

   };

}

#endif

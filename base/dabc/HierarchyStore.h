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

namespace dabc {

   class HierarchyStore {
      protected:
         std::string fBasePath;      ///! base directory for data store

         FileInterface* fIO{nullptr};    ///! file interface
         dabc::BinaryFile fFile;         ///! file used to write data
         dabc::DateTime   fLastStoreTm;  ///! time when last store was done
         dabc::DateTime   fLastFlushTm;  ///! time when last store flush was done

         bool      fDoStore{false};      ///! indicate if store diff should be done
         bool      fDoFlush{false};      ///! indicate if store flush should be done
         uint64_t  fLastVersion{0};      ///! last stored version
         Buffer    fStoreBuf;
         Buffer    fFlushBuf;

      public:
         HierarchyStore();
         virtual ~HierarchyStore();

         /** \brief Set base path for data storage, can only be changed when all files are closed */
         bool SetBasePath(const std::string &path);


         bool StartFile(dabc::Buffer buf);

         bool WriteDiff(dabc::Buffer buf);

         bool CloseFile();

         /** \brief Returns true if any new store action should be performed */
         bool CheckForNextStore(DateTime& now, double store_period, double flush_period);

         /** \brief Extract data which is can be stored, must be called under hierarchy mutex */
         bool ExtractData(dabc::Hierarchy& h);

         /** \brief Write extracted data to files, performed outside hierarchy mutex */
         bool WriteExtractedData();
   };


   // =====================================================================


   class HierarchyReading {
      protected:
         std::string fBasePath;        ///! base directory

         FileInterface* fIO{nullptr};  ///! file interface

         dabc::Hierarchy  fTree;       ///! scanned files tree

         bool ScanTreeDir(dabc::Hierarchy& h, const std::string &dirname);

         bool ScanFiles(const std::string &dirname, const DateTime& onlydate, std::vector<uint64_t>& vect);

         std::string MakeFileName(const std::string &fpath, const DateTime& dt);

         bool ProduceStructure(Hierarchy& tree, const DateTime& from, const DateTime& till, const std::string &entry, Hierarchy& tgt);

         dabc::Buffer ReadBuffer(dabc::BinaryFile& f);

      public:

         HierarchyReading();
         virtual ~HierarchyReading();

         /** Set top directory for all recorded data */
         void SetBasePath(const std::string &path);

         /** Scan only directories, do not open any files */
         bool ScanTree();

         /** Get full structure at given point of time */
         bool GetStrucutre(Hierarchy& h, const DateTime& dt = 0);

         /** Get entry with history for specified time interval */
         Hierarchy GetSerie(const std::string &entry, const DateTime& from, const DateTime& till);

   };

}

#endif

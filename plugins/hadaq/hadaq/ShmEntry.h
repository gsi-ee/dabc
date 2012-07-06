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

#ifndef HADAQ_ShmEntry
#define HADAQ_ShmEntry

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif

extern "C" {
   #include "hadaq/worker.h"
}

namespace hadaq {



   class ShmEntry {
      public:


      protected:
         std::string fStatsName;      //!< statistics parameter name in hadaq "worker" segment
         std::string fShmName;        //!< name of shared memory file
         ::Worker* fWorker;           // reference to shmem handle

        unsigned long* fShmPtr;       //!<points to value in shared memory

        dabc::Parameter fPar;        // backreference to parameter


      public:

         /** ctor will create service from given dabc parameter*/
         ShmEntry(const std::string& dabcname , const std::string& shmemname, ::Worker* handle, dabc::Parameter& par);

         virtual ~ShmEntry();

         bool IsStatsName(const std::string& name) const { return fStatsName == name; }
         bool IsShmemName(const std::string& name) const { return fShmName == name; }


         /** Update shm value from parameter */
         void UpdateValue(const std::string& value);

         /** Update backpointed parameter  from shm */
         void UpdateParameter();

         /* Retrieve back value from shmem*/
         unsigned long GetValue()
         {
            if(fShmPtr==0) return 0;
            return *fShmPtr;
         }

   };

}

#endif

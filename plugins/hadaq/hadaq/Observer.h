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

#ifndef HADAQ_Observer
#define HADAQ_Observer

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

extern "C" {
#include "hadaq/worker.h"
}

#include <list>

namespace hadaq {

   class ShmEntry;

   /** \brief Class registers for parameter event from the DABC and export these events to SHM/hadaq "Worker" mechanism
    *
    */

   class Observer : public dabc::Worker  {
      protected:

         bool fEnabled;  ///< true when HADAQ observer is enabled
         int fNodeId;
         double fFlushTimeout;

         /* handle for shmem worker structure for netmem emulation*/
         ::Worker* fNetmemWorker;

         /* handle for shmem worker structure for evtbuild emulation*/
         ::Worker* fEvtbuildWorker;

         typedef std::list<ShmEntry*> ShmEntriesList;

         ShmEntriesList fEntries;
         dabc::Mutex        fEntryMutex;

         ShmEntry*  FindEntry(const std::string& parname)
         {
            return FindEntry(ReducedName(parname), ShmName(parname));
         }

         ShmEntry*  FindEntry(const std::string& statsname, const std::string& shmemname);

         /* strip netmem or evtbuild prefix from parameter name*/
         std::string ReducedName(const std::string& dabcname);

         /* evaluate name of shm file from parameter name prefix*/
         std::string ShmName(const std::string& dabcname);

         bool CreateShmEntry(const std::string& parname);

         void RemoveEntry(ShmEntry* entry);

         virtual double ProcessTimeout(double lastdiff);

      public:
         Observer(const std::string& name);

         virtual ~Observer();

         virtual const char* ClassName() const { return "Observer"; }

         virtual void ProcessParameterEvent(const dabc::ParameterEvent& evnt);

         static const char* RunStatusName() { return "RunStatus"; }

         bool IsEnabled() const { return fEnabled; }

         /* converts prefix text to number for shmem export*/
         static int Args_prefixCode(const char* prefix);
   };


  void sigHandler(int sig);


}

#endif

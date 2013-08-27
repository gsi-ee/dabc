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

#ifndef FESA_Monitor
#define FESA_Monitor

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

class rdaDeviceHandle;
class rdaRDAService;
class rdaDabcHandler;
class rdaData;

namespace fesa {

   /** \brief Player of FESA data
    *
    * Module builds hierarchy for connected FESA classes,
    * which could be served via DABC web server in any browser
    *
    **/

   class Monitor : public dabc::ModuleAsync {
      protected:
         virtual void BeforeModuleStart() {}

         virtual void AfterModuleStop() {}

         dabc::Mutex     fHierarchyMutex;
         dabc::Hierarchy fHierarchy;

         std::string fServerName;    ///< FESA server name
         std::string fDeviceName;    ///< FESA device name
         std::string fCycle;         ///< cycle parameter

         rdaRDAService* fRDAService;
         rdaDeviceHandle* fDevice;
         std::vector<rdaDabcHandler*> fHandlers;

         double doGet(const std::string& service, const std::string& field);

      public:

         Monitor(const std::string& name, dabc::Command cmd = 0);
         virtual ~Monitor();

         virtual void ModuleCleanup() {}

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void BuildWorkerHierarchy(dabc::HierarchyContainer* cont);

         void ReportServiceChanged(const std::string& name, const rdaData* v);
   };
}


#endif

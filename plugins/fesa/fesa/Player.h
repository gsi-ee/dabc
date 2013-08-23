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

#ifndef FESA_Player
#define FESA_Player


#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

class rdaDeviceHandle;
class rdaRDAService;
class DabcReportHandler;

namespace fesa {

   /** \brief Player of FESA data
    *
    * Module builds hierarchy for connected FESA classes,
    * which could be served via DABC web server in any browser
    *
    **/

   class Player : public dabc::ModuleAsync {
      protected:
         virtual void BeforeModuleStart() {}

         virtual void AfterModuleStop() {}

         dabc::Mutex     fHierarchyMutex;
         dabc::Hierarchy fHierarchy;

         unsigned fCounter;

         dabc::Reference fProducer;   ///< binary producer for ROOT objects
         void* fHist;                 ///< ROOT histogram
         void* fCanvas;               ///< canvas for image generation
         
         bool  fSynchron;            ///< is request synchron or asynchron
         std::string fServerName;    ///< FESA server name
         std::string fDeviceName;    ///< FESA device name
         std::string fCycles;        ///< cycles parameter
         std::string fService;       ///< service name
         std::string fField;         ///< field name in the service
         
         rdaRDAService* fRDAService;
         rdaDeviceHandle* fDevice;
         DabcReportHandler* fHandler;
         
         double doGet(const std::string& service, const std::string& field);

      public:

         Player(const std::string& name, dabc::Command cmd = 0);
         virtual ~Player();

         virtual void ModuleCleanup() {}

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void BuildWorkerHierarchy(dabc::HierarchyContainer* cont);
         
         void ReportServiceChanged(const std::string& name, double v);
   };
}


#endif

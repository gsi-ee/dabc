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

#ifndef HADAQ_MonitorModule
#define HADAQ_MonitorModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace hadaq {

   /** \brief Monitor of TRB slow control data
    *
    * Module builds hierarchy for connected FESA classes,
    * which could be served via DABC web server in any browser
    *
    * In addition, module can provide data with stored EPICS records in form of MBS events
    *
    **/

   class MonitorModule : public dabc::ModuleAsync {
      protected:

         double  fInterval;       ///< Time interval for reading in seconds
         std::string fTopFolder;  ///< name of top folder, which should exists also in every variable
         unsigned fSubevId;       ///< subevent id
         unsigned fTriggerType;   ///< trigger type
         std::string fShellCmd;   ///< shell command with formats pattern

         std::vector<uint64_t> fAddrs0;      ///< array of monitored address
         std::vector<uint64_t> fAddrs;      ///< array of monitored address
         uint32_t fEventId;                 ///< event number

         void OnThreadAssigned() override;

         std::string GetItemName(unsigned addr);

         bool ReadAllVariables(dabc::Buffer &buf);

         uint32_t DoRead(uint32_t addr0, uint32_t addr);

      public:
         MonitorModule(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~MonitorModule() = default;

         void ProcessTimerEvent(unsigned timer) override;

         void BeforeModuleStart() override;
   };
}


#endif

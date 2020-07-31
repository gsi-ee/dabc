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

#include "dabc/ModuleAsync.h"

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

         std::vector<uint64_t> fAddrs;      ///< array of monitored address
         uint32_t fEventId;                 ///< event number

         virtual void OnThreadAssigned();

         std::string GetItemName(unsigned addr);

         bool ReadAllVariables(dabc::Buffer &buf);

         uint32_t DoRead(uint32_t addr);

      public:
         MonitorModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual void ProcessTimerEvent(unsigned timer);
   };
}


#endif

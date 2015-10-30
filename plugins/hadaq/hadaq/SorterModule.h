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

#ifndef HADAQ_SorterModule
#define HADAQ_SorterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace hadaq {


/** \brief Sorts HADAQ subevents according to trigger number
 *
 * Need to be applied when TRB send provides events not in order they appear
 * Or when network adapter provides UDP packets not in order
 */

   class SorterModule : public dabc::ModuleAsync {

   protected:
      int    fFlushCnt;

      bool retransmit();

      virtual int ExecuteCommand(dabc::Command cmd);

   public:
      SorterModule(const std::string& name, dabc::Command cmd = 0);

      virtual bool ProcessBuffer(unsigned) { return retransmit(); }

      virtual bool ProcessRecv(unsigned) { return retransmit(); }

      virtual bool ProcessSend(unsigned) { return retransmit(); }

      virtual void ProcessTimerEvent(unsigned);

   };

}


#endif

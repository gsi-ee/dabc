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

#ifndef HADAQ_MbsTransmitterModule
#define HADAQ_MbsTransmitterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace hadaq {


/*
 * This module will take hadaq format input and wrap it into a Mbs subevent for further analysis.
 * Use same formatting as in hadaq eventsource of go4.
 */


   class MbsTransmitterModule : public dabc::ModuleAsync {

      protected:

         void retransmit();

         unsigned int fSubeventId;

      public:
      	MbsTransmitterModule(const char* name, dabc::Command cmd = 0);

         virtual void ProcessInputEvent(dabc::Port*) { retransmit(); }

         virtual void ProcessOutputEvent(dabc::Port*) { retransmit(); }

   };

}


#endif

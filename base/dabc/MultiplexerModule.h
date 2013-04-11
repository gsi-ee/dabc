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

#ifndef DABC_MultiplexerModule
#define DABC_MultiplexerModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif


namespace dabc {

   typedef Queue<unsigned, true> PortIdsQueue;

   class MultiplexerModule : public dabc::ModuleAsync {
      protected:
         PortIdsQueue fQueue;

         std::string fDataRateName;

         void CheckDataSending();

      public:
         MultiplexerModule(const std::string& name, dabc::Command cmd = 0);

         virtual void ProcessInputEvent(unsigned port);
         virtual void ProcessOutputEvent(unsigned port);
   };

}

#endif

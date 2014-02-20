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

#ifndef MBS_api
#define MBS_api


#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

namespace mbs {

   class ReadoutHandle;

   class ReadoutModule : public dabc::ModuleAsync {
      protected:

         friend class ReadoutHandle;

         mbs::ReadIterator fIter;   ///< iterator, accessed only from user side
         dabc::Command fCmd;        ///< current nextbuffer cmd

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void ProcessInputEvent(unsigned port);

         virtual void ProcessTimerEvent(unsigned timer);


      public:

         ReadoutModule(const std::string& name, dabc::Command cmd);

   };


   class ReadoutHandle : protected dabc::ModuleAsyncRef {

      DABC_REFERENCE(ReadoutHandle, dabc::ModuleAsyncRef, ReadoutModule)

      virtual ~ReadoutHandle() { Disconnect(); }

      /** Connect with MBS server */
      static ReadoutHandle Connect(const std::string& url);

      bool null() const { return dabc::ModuleAsyncRef::null(); }

      /** Disconnect from MBS server */
      bool Disconnect();

      /** Retrieve next event from the server */
      mbs::EventHeader* NextEvent(double tm = 1.0);
   };

}


#endif

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

#ifndef STREAM_RecalibrateModule
#define STREAM_RecalibrateModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace stream {

   class DabcProcMgr;


   /** \brief Runs code of stream framework
    *
    * Module used to run code, available in stream framework
    */

   class RecalibrateModule : public dabc::ModuleAsync {

   protected:

      virtual int ExecuteCommand(dabc::Command cmd);

      virtual void OnThreadAssigned();

      bool retransmit();

   public:

      RecalibrateModule(const std::string& name, dabc::Command cmd = 0);
      virtual ~RecalibrateModule();

      virtual bool ProcessRecv(unsigned) { return retransmit(); }

      virtual bool ProcessSend(unsigned) { return retransmit(); }

      virtual void BeforeModuleStart();

      virtual void AfterModuleStop();
   };
}


#endif

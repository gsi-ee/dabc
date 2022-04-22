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

namespace hadaq {
   class HldProcessor;
}

namespace stream {

   class DabcProcMgr;


   /** \brief Runs code of stream framework
    *
    * Module used to run code, available in stream framework
    */

   class RecalibrateModule : public dabc::ModuleAsync {

   protected:

      int fNumSub{0};                  // number of sub-modules
      int fNumEv{0};                   // number of precessed events
      int fMaxNumEv{0};                // maximal number of events to process
      bool fReplace{false};            // replace or not TDC messages
      DabcProcMgr *fProcMgr{nullptr};  // central process manager
      hadaq::HldProcessor *fHLD{nullptr}; // processor of HLD events

      int ExecuteCommand(dabc::Command cmd) override;

      void OnThreadAssigned() override;

      bool retransmit();

   public:

      RecalibrateModule(const std::string &name, dabc::Command cmd = nullptr);
      virtual ~RecalibrateModule();

      bool ProcessRecv(unsigned) override { return retransmit(); }

      bool ProcessSend(unsigned) override { return retransmit(); }

      bool ProcessBuffer(unsigned) override { return retransmit(); }

      void BeforeModuleStart() override;

      void AfterModuleStop() override;
   };
}


#endif

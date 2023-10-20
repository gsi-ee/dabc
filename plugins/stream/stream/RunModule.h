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

#ifndef STREAM_RunModule
#define STREAM_RunModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include "dabc/timing.h"


namespace stream {

   class DabcProcMgr;


   /** \brief Runs code of stream framework
    *
    * Module used to run code, available in stream framework
    */

   class RunModule : public dabc::ModuleAsync {

   protected:
      int          fParallel{0}; /// how many parallel processes to start
      void        *fInitFunc{nullptr}; /// init function
      int          fStopMode{0}; /// for central module waiting that others finish
      DabcProcMgr *fProcMgr{nullptr};
      std::string  fAsf;
      std::string  fFileUrl;  ///<! configured file URL - module used to produce output
      bool         fDidMerge{false};
      bool         fFastMode{false}; ///<! data directly processed, no complex logic of stream
      long unsigned fTotalSize{0};
      long unsigned fTotalEvnts{0};
      long unsigned fTotalOutEvnts{0};
      int           fDefaultFill{0};   ///<! default fill color for 1-D histograms

      int ExecuteCommand(dabc::Command cmd) override;

      void OnThreadAssigned() override;

      bool ProcessNextEvent(void* evnt, unsigned evntsize);

      bool ProcessNextBuffer();

      bool RedistributeBuffers();

      void ProduceMergedHierarchy();

      void SaveHierarchy(dabc::Buffer buf);

      void GenerateEOF(dabc::Buffer buf);

   public:
      RunModule(const std::string &name, dabc::Command cmd = nullptr);
      virtual ~RunModule();

      bool ProcessRecv(unsigned port) override;

      bool ProcessSend(unsigned) override { return RedistributeBuffers(); }

      void ProcessTimerEvent(unsigned) override;

      void BeforeModuleStart() override;

      void AfterModuleStop() override;
   };
}


#endif

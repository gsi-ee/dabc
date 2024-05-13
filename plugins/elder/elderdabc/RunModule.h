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

#ifndef ELDER_RunModule
#define ELDER_RunModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include "dabc/timing.h"

namespace mbs {class ReadIterator;}

 namespace elderpt { namespace control { class Controller; class Event;} }


namespace elderdabc {

   class VisConDabc;


   /** \brief Runs code of elder framework
    *
    * Module used to run code, available in elder framework
    */

   class RunModule : public dabc::ModuleAsync {

   protected:
      elderdabc::VisConDabc* fViscon; // visualization interface
      ::elderpt::control::Controller* fAnalysis; //handle to elder analysis framework
      ::elderpt::control::Event* fEvent; // the current event to be processed
      std::string fElderConfigFile; //!<! configuration file of elder analyis
      std::string  fAsf;
      std::string  fFileUrl;  ///<! configured file URL - module used to produce output
      long unsigned fTotalSize{0};
      long unsigned fTotalEvnts{0};
      long unsigned fTotalOutEvnts{0};
      int           fDefaultFill{0};   ///<! default fill color for 1-D histograms

      int ExecuteCommand(dabc::Command cmd) override;

      void OnThreadAssigned() override;

      bool ProcessNextEvent(mbs::ReadIterator& iter);


      bool ProcessNextBuffer();


      void SaveHierarchy(dabc::Buffer buf);


   public:
      RunModule(const std::string &name, dabc::Command cmd = nullptr);
      virtual ~RunModule();

      bool ProcessRecv(unsigned port) override;

      //bool ProcessSend(unsigned) override { return RedistributeBuffers(); }

      void ProcessTimerEvent(unsigned) override;

      void BeforeModuleStart() override;

      void AfterModuleStop() override;
   };
}


#endif

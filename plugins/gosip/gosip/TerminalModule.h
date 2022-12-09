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

#ifndef GOSIP_TerminalModule
#define GOSIP_TerminalModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif



#include <vector>




/****************************************************************/



namespace gosip {

   /** \brief Abstract GOSIP terminal module
    *
    * Just executes commands
    */

   class TerminalModule : public dabc::ModuleAsync {
      protected:



         int ExecuteCommand(dabc::Command cmd) override;

#ifndef GOSIP_COMMAND_PLAINC
// JAM command execution functions are put here for C++ implementation:




#endif

      public:

         TerminalModule(const std::string &name, dabc::Command cmd = nullptr);

         void BeforeModuleStart() override;

         void ProcessTimerEvent(unsigned timer) override;


         /*** keep result of most recent command call here
          * TODO: later move to Command class*/
         static std::vector<long> fCommandResults;

         /*** keep address of most recent command call here*/
         static std::vector<long> fCommandAddress;

         /*** keep Sfp of most recent command call here, for broadcast*/
          static std::vector<long> fCommandSfp;

          /*** keep Slave of most recent command call here, for broadcast*/
          static std::vector<long> fCommandSlave;
   };
}


#endif

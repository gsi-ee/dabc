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

#ifndef DOFI_Player
#define DOFI_Player

#include <cstddef>
#include <cstring>

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

namespace dofi {

   /** \brief Player of DOFI data
    *
    * Module provides DOFI command and custom user interface
    *
    **/

   class Player : public dabc::ModuleAsync {
      protected:
         virtual void BeforeModuleStart() override {}

         virtual void AfterModuleStop() override {}

      public:

         Player(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~Player();

         virtual void ModuleCleanup() override {}

         virtual void ProcessTimerEvent(unsigned timer) override;

         virtual int ExecuteCommand(dabc::Command cmd) override;;
   };
}


#endif

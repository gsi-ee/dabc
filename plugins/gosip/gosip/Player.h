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

#ifndef GOSIP_Player
#define GOSIP_Player

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

namespace gosip {

   /** \brief Player of GOSIP data
    *
    * Module provides GOSIP command and custom user interface
    *
    **/

   class Player : public dabc::ModuleAsync {
      protected:
         virtual void BeforeModuleStart() {}

         virtual void AfterModuleStop() {}

      public:

         Player(const std::string& name, dabc::Command cmd = nullptr);
         virtual ~Player();

         virtual void ModuleCleanup() {}

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);
   };
}


#endif

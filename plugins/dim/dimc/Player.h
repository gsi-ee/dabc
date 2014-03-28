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

#ifndef DIMC_Player
#define DIMC_Player

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif


class DimBrowser;

namespace dimc {

   /** \brief Player of DIM data
    *
    * Module builds hierarchy for discovered DIM variables
    *
    **/

   class Player : public dabc::ModuleAsync {

      protected:

         std::string    fDimDns;  ///<  name of DNS server
         ::DimBrowser*  fDimBr;   ///<  dim browser

         virtual void OnThreadAssigned();

      public:
         Player(const std::string& name, dabc::Command cmd = 0);
         virtual ~Player();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);
   };
}


#endif

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

#ifndef DABC_ROOT_RootSniffer
#define DABC_ROOT_RootSniffer

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

namespace dabc_root {

   /** \brief %RootSniffer provides access to ROOT objects for DABC
    *
    */

   class RootSniffer : public dabc::Worker  {

      protected:
         bool fEnabled;

         /** \brief Complete snapshot of ROOT hierarchy
          *  \details Will be updated from ROOT main loop  */
         dabc::Hierarchy fHierarchy;

         dabc::Mutex fHierarchyMutex; ///< used to protect content of hierarchy

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

      public:
         RootSniffer(const std::string& name);

         virtual ~RootSniffer();

         virtual const char* ClassName() const { return "RootSniffer"; }

         bool IsEnabled() const { return fEnabled; }
   };

}

#endif

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

#ifndef ROOT_Player
#define ROOT_Player

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef DABC_CommandsQueue
#include "dabc/CommandsQueue.h"
#endif

class TRootSniffer;

namespace root {


   /** \brief %Player provides access to ROOT objects for DABC
    *
    */

   class PlayerRef;

   class Player : public dabc::Worker  {

      friend class PlayerRef;

      protected:

         bool fEnabled;

         std::string fPrefix;     ///< name prefix for hierarchy like ROOT or Go4

         /** \brief Last hierarchy, build in ROOT main thread */
         dabc::Hierarchy fRoot;

         /** \brief hierarchy, used for version control */
         dabc::Hierarchy fHierarchy;

         dabc::CommandsQueue fRootCmds; ///< list of commands, which must be executed in the ROOT context

         dabc::TimeStamp fLastUpdate;

         virtual void OnThreadAssigned();

         virtual void InitializeHierarchy() {}

         virtual double ProcessTimeout(double last_diff);

         /** Method scans normal objects, registered in ROOT and DABC */
         void RescanHierarchy(TRootSniffer* sniff, dabc::Hierarchy& main, const char* path = 0);

         virtual int ExecuteCommand(dabc::Command cmd);

         void ProcessActionsInRootContext(TRootSniffer* sniff);

         virtual int ProcessGetBinary(TRootSniffer* sniff, dabc::Command cmd);

      public:
         Player(const std::string& name, dabc::Command cmd = 0);

         virtual ~Player();

         virtual const char* ClassName() const { return "Player"; }

         bool IsEnabled() const { return fEnabled; }
   };


   class PlayerRef : public dabc::WorkerRef  {
      DABC_REFERENCE(PlayerRef, dabc::WorkerRef, Player);

      void ProcessActionsInRootContext(TRootSniffer* sniff)
      {
         if (GetObject()) GetObject()->ProcessActionsInRootContext(sniff);
      }

   };

}

#endif

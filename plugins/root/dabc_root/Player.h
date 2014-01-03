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

#ifndef DABC_ROOT_Player
#define DABC_ROOT_Player

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef DABC_CommandsQueue
#include "dabc/CommandsQueue.h"
#endif

class TDabcTimer;
class TRootSniffer;

namespace dabc_root {


   /** \brief %Player provides access to ROOT objects for DABC
    *
    */

   class Player : public dabc::Worker  {

      friend class ::TDabcTimer;


      protected:

         bool fEnabled;
         bool fBatch;             ///< if true, batch mode will be activated
         bool fSyncTimer;         ///< is timer will run in synchronous mode (only within gSystem->ProcessEvents())
         int  fCompression;       ///< compression level, default 5

         std::string fPrefix;     ///< name prefix for hierarchy like ROOT or Go4

         TRootSniffer* fNewSniffer;  ///< native sniffer, used to scan ROOT data

         ::TDabcTimer*  fTimer;  ///< timer used to perform actions in ROOT context

         /** \brief Last hierarchy, build in ROOT main thread */
         dabc::Hierarchy fRoot;

         /** \brief hierarchy, used for version control */
         dabc::Hierarchy fHierarchy;

         dabc::CommandsQueue fRootCmds; ///< list of commands, which must be executed in the ROOT context

         dabc::TimeStamp fLastUpdate;

         dabc::Thread_t fStartThrdId;   ///< remember thread id where sniffer was started, can be used to check how timer processing is working

         virtual void OnThreadAssigned();

         virtual void InitializeHierarchy() {}

         virtual double ProcessTimeout(double last_diff);

         /** Method scans normal objects, registered in ROOT and DABC */
         void RescanHierarchy(dabc::Hierarchy& main, const char* path = 0);

         virtual int ExecuteCommand(dabc::Command cmd);

         void ProcessActionsInRootContext();

         virtual int ProcessGetBinary(dabc::Command cmd);

         void SetObjectSniffer(TRootSniffer* sniff);

      public:
         Player(const std::string& name, dabc::Command cmd = 0);

         virtual ~Player();

         virtual const char* ClassName() const { return "Player"; }

         bool IsEnabled() const { return fEnabled; }

         TRootSniffer* GetSniffer() { return fNewSniffer; }

         /** Create TTimer object, which allows to perform action in ROOT context */
         void InstallSniffTimer();

   };

}

#endif

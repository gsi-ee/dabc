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

#ifndef ROOT_Monitor
#define ROOT_Monitor

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef DABC_CommandsQueue
#include "dabc/CommandsQueue.h"
#endif

class THttpServer;
class TRootSniffer;

namespace root {


   /** \brief Class provides live access to ROOT objects for DABC
    *
    */

   class MonitorRef;

   class Monitor : public dabc::Worker  {

      friend class MonitorRef;

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

         void ProcessActionsInRootContext(THttpServer* serv, TRootSniffer* sniff);

         /** Execute extra command in ROOT context, used in Go4 plugin */
         virtual bool ProcessHCommand(const std::string& cmdname, dabc::Command cmd) { return false; }

         virtual int ProcessGetBinary(THttpServer* serv, TRootSniffer* sniff, dabc::Command cmd);

      public:
         Monitor(const std::string& name, dabc::Command cmd = nullptr);

         virtual ~Monitor();

         virtual const char* ClassName() const { return "Player"; }

         bool IsEnabled() const { return fEnabled; }
   };


   class MonitorRef : public dabc::WorkerRef  {
      DABC_REFERENCE(MonitorRef, dabc::WorkerRef, Monitor);

      void ProcessActionsInRootContext(THttpServer* serv, TRootSniffer* sniff)
      {
         if (GetObject()) GetObject()->ProcessActionsInRootContext(serv, sniff);
      }

   };

}

#endif

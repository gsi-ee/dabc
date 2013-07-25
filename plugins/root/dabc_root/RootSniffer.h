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

#ifndef DABC_CommandsQueue
#include "dabc/CommandsQueue.h"
#endif

class TDirectory;
class TBufferFile;
class TMemFile;
class TClass;
class TCollection;
class TObject;
class TDabcTimer;

namespace dabc_root {


   class BinaryProducer;

   /** \brief %RootSniffer provides access to ROOT objects for DABC
    *
    */

   class RootSniffer : public dabc::Worker  {

      friend class ::TDabcTimer;

      protected:
         bool fEnabled;
         bool fBatch;             ///< if true, batch mode will be activated
         int  fCompression;       ///< compression level, default 5

         BinaryProducer*  fProducer;  ///< object which will convert ROOT objects into binary data

         ::TDabcTimer*  fTimer;  ///< timer used to perform actions in ROOT context

         /** \brief Last hierarchy, build from ROOT itself */
         dabc::Hierarchy fRoot;

         /** \brief hierarchy, used for version control */
         dabc::Hierarchy fHierarchy;

         /** \brief Mutex, used to protect fHierarchy object, why it can be accessed from different threads */
         dabc::Mutex fHierarchyMutex; ///< used to protect content of hierarchy

         dabc::CommandsQueue fRootCmds; ///< list of commands, which must be executed in the ROOT context

         dabc::TimeStamp fLastUpdate;

         virtual void OnThreadAssigned();

         virtual void InitializeHierarchy() {}

         virtual double ProcessTimeout(double last_diff);

         static void* AddObjectToHierarchy(dabc::Hierarchy& parent, const char* searchpath, TObject* obj, int lvl);

         static void* ScanListHierarchy(dabc::Hierarchy& h, const char* searchpath, TCollection* lst, int lvl, const std::string& foldername = "");

         /* Method is used to scan ROOT objects.
          * If path is empty, than hierarchy structure will be created.
          * If path specified, object with provided path name will be searched */
         virtual void* ScanRootHierarchy(dabc::Hierarchy& h, const char* searchpath = 0);

         virtual int ExecuteCommand(dabc::Command cmd);

         static bool IsDrawableClass(TClass* cl);
         static bool IsBrowsableClass(TClass* cl);

         void ProcessActionsInRootContext();

         int ProcessGetBinary(dabc::Command cmd);

         /** Should deliver hierarchy of the ROOT objects */
         virtual void BuildWorkerHierarchy(dabc::HierarchyContainer* cont);

      public:
         RootSniffer(const std::string& name, dabc::Command cmd = 0);

         virtual ~RootSniffer();

         virtual const char* ClassName() const { return "RootSniffer"; }

         bool IsEnabled() const { return fEnabled; }

         /** Create TTimer object, which allows to perform action in ROOT context */
         void InstallSniffTimer();

   };

}

#endif

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
class TSeqCollection;
class TObject;
class TDabcTimer;

namespace dabc_root {

   /** \brief %RootBinDataContainer provides access to ROOT TBuffer object for DABC
    *
    */

   class RootBinDataContainer : public dabc::BinDataContainer {
      protected:
         TBufferFile* fBuf;
      public:
         RootBinDataContainer(TBufferFile* buf);
         virtual ~RootBinDataContainer();

         virtual void* data() const;
         virtual unsigned length() const;
   };


   /** \brief %RootSniffer provides access to ROOT objects for DABC
    *
    */

   class RootSniffer : public dabc::Worker  {

      friend class ::TDabcTimer;

      protected:
         bool fEnabled;
         bool fBatch;           ///< if true, batch mode will be activated

         ::TDabcTimer*  fTimer;  ///< timer used to perform actions in ROOT context

         /** \brief Last hierarchy, build from ROOT itself */
         dabc::Hierarchy fRoot;

         /** \brief hierarchy, used for version control */
         dabc::Hierarchy fHierarchy;

         /** \brief Mutex, used to protect fHierarchy object, why it can be accessed from different threads */
         dabc::Mutex fHierarchyMutex; ///< used to protect content of hierarchy

         dabc::CommandsQueue fRootCmds; ///< list of commands, which must be executed in the ROOT context

         TMemFile*   fMemFile;         ///< file used to generate streamer infos

         int         fSinfoSize;       ///< size of last generated streamer info

         dabc::TimeStamp fLastUpdate;

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         static void AddObjectToHierarchy(dabc::Hierarchy& parent, TObject* obj, int lvl);

         static void FillListHieararchy(dabc::Hierarchy& h, TSeqCollection* lst, int lvl, const std::string& foldername = "");

         static void FillROOTHieararchy(dabc::Hierarchy& h);

         virtual int ExecuteCommand(dabc::Command cmd);

         static bool IsSupportedClass(TClass* cl);

         TBufferFile* ProduceStreamerInfos();
         TBufferFile* ProduceStreamerInfosMem();

         void ProcessActionsInRootContext();

         int SimpleGetBinary(dabc::Command cmd);

         int ProcessGetBinary(dabc::Command cmd);


      public:
         RootSniffer(const std::string& name, dabc::Command cmd = 0);

         virtual ~RootSniffer();

         virtual const char* ClassName() const { return "RootSniffer"; }

         bool IsEnabled() const { return fEnabled; }

         /** Should deliver hierarchy of the ROOT objects */
         virtual void BuildHierarchy(dabc::HierarchyContainer* cont);

         /** Create TTimer object, which allows to perform action in ROOT context */
         void InstallSniffTimer();

   };

}

#endif

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

         enum {
            mask_Scan    = 1,  ///< normal scan of hierarchy
            mask_Expand  = 2,  ///< expand of specified item
            mask_Search  = 4,  ///< search for specified item
            mask_Members = 8,  ///< enable expand of objects with class info
            mask_ChldMemb = 16 ///< expand child members
         };

         struct ScanRec {
            unsigned mask;
            const char* searchpath;
            dabc::Hierarchy top, sub, prnt;
            int lvl;
            std::string itemname, objname;
            ScanRec* parent_rec;
            TObject* res;

            ScanRec() : mask(0), searchpath(0), top(), sub(),
                        prnt(), lvl(0), itemname(), objname(),
                        parent_rec(0), res(0) {}

            bool MakeChild(ScanRec& super, const std::string& foldername = "");

            bool TestObject(TObject* obj);

            /** Set item field only when creating is specified */
            void SetField(const std::string& name, const std::string& value);

            bool HasField(const std::string& name) { return top.HasField(name); }

            /** Returns true when extra entries like object member can be extracted */
            bool CanExpandItem()
            {
               if (mask & mask_Members) return true;

               if (mask & mask_Expand) return true;

               // when we search for object, but did not found item or item marked as expandable
               if ((mask & mask_Search) && (objname.empty() || top.HasField(dabc::prop_more))) return true;

               return top.HasField("#members");
            }

            /** Set result pointer and return true if result is found */
            bool SetResult(TObject* obj);

            /** Method indicates that scanning can be interrupted while result is set */
            bool Done();
         };

         bool fEnabled;
         bool fBatch;             ///< if true, batch mode will be activated
         bool fSyncTimer;         ///< is timer will run in synchronous mode (only within gSystem->ProcessEvents())
         int  fCompression;       ///< compression level, default 5

         std::string fPrefix;     ///< name prefix for hierarchy like ROOT or Go4

         BinaryProducer*  fProducer;  ///< object which will convert ROOT objects into binary data

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

         void ScanObjectMemebers(ScanRec& rec, TClass* cl, char* ptr, unsigned long int cloffset);

         void ScanObject(ScanRec& rec, TObject* obj);

         void ScanList(ScanRec& rec, TCollection* lst, const std::string& foldername = "");

         /* Method is used to scan ROOT objects.
          * If path is empty, than hierarchy structure will be created.
          * If path specified, object with provided path name will be searched */
         virtual void ScanRoot(ScanRec& rec);

         /** Method scans normal objects, registered in ROOT and DABC */
         void RescanHierarchy(dabc::Hierarchy& main);

         /** Selectively expand selected objects */
         void ExpandHierarchy(dabc::Hierarchy& main, const std::string& itemname);

         /** Find object in hierarchy */
         void* FindInHierarchy(dabc::Hierarchy& main, const std::string& itemname);

         virtual int ExecuteCommand(dabc::Command cmd);

         static bool IsDrawableClass(TClass* cl);
         static bool IsBrowsableClass(TClass* cl);

         void ProcessActionsInRootContext();

         virtual int ProcessGetBinary(dabc::Command cmd);

      public:
         RootSniffer(const std::string& name, dabc::Command cmd = 0);

         virtual ~RootSniffer();

         virtual const char* ClassName() const { return "RootSniffer"; }

         bool IsEnabled() const { return fEnabled; }

         /** Create TTimer object, which allows to perform action in ROOT context */
         void InstallSniffTimer();

         /** Register object in objects hierarchy
          * Should be called from main ROOT thread */
         bool RegisterObject(const char* folder, TObject* obj);

         /** Unregister object from objects hierarchy
          * Should be called from main ROOT thread */
         bool UnregisterObject(TObject* obj);

   };

}

#endif

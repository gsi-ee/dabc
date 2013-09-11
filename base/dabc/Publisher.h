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

#ifndef DABC_Publisher
#define DABC_Publisher

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include <list>

namespace dabc {

   class PublisherRef;

   class CmdPublisher : public Command {
      DABC_COMMAND(CmdPublisher, "CmdPublisher");
   };

   /** Command used to get published values from hierarchy */
   class CmdPublisherGet : public Command {
      DABC_COMMAND(CmdPublisherGet, "CmdPublisherGet");
   };

   /** Command used to produce custom binary data for published in hierarchy entries */
   class CmdGetBinary : public Command {
      DABC_COMMAND(CmdGetBinary, "CmdGetBinary");
   };


   class CmdSubscriber : public Command {
      DABC_COMMAND(CmdSubscriber, "CmdSubscriber");

      void SetEntry(const Hierarchy& e) { SetRef("entry", e); }
      Hierarchy GetEntry() { return GetRef("entry"); }

   };


   struct PublisherEntry {
      unsigned id;          // unique id in the worker
      std::string path;     // absolute path in hierarchy
      std::string worker;   // worker which is manages hierarchy
      std::string fulladdr;  // address, used to identify producer of the hierarchy branch
      void* hier;           // local hierarchy pointer, one not allowed to use it from publisher
      void* mutex;          // local mutex, which must be used by worker for the hierarchy
      uint64_t version;     // last requested version
      uint64_t lastglvers;  // last version, used to build global
      bool local;           // is entry from local node
      int  errcnt;          // counter of consequent errors
      bool waiting_publisher; // indicate if next request is submitted
      Hierarchy rem;        // remote hierarchy

      PublisherEntry() :
         id(0), path(), worker(), fulladdr(), hier(0), mutex(0),
         version(0), lastglvers(0), local(true), errcnt(0), waiting_publisher(false), rem() {}
   };

   struct SubscriberEntry {
      unsigned id;          // unique id in the worker
      std::string path;     // absolute path in hierarchy
      std::string worker;   // worker which is manages hierarchy
      bool local;           // is entry from local node
      int hlimit;           // history limit -1 - DNS, 0 - no history
      Hierarchy fEntry;     // entry which is provided to the worker as result
      uint64_t version;     // last version provided to subscriber
      bool waiting_worker;  // when enabled, we are waiting while worker process entry

      SubscriberEntry() :
         id(0), path(), worker(), local(true),
         hlimit(0), fEntry(), version(0), waiting_worker(false) {}
   };

   /** \brief %Module manages published hierarchies and provide optimize access to them
    *
    * \ingroup dabc_all_classes
    */

   class Publisher : public dabc::ModuleAsync {

      friend class PublisherRef;

      protected:

         Hierarchy fGlobal;  ///! this is hierarchy of all known items, including remote, used only when any global hierarchies are existing

         Hierarchy fLocal;   ///! this is hierarchy only for entries from local modules

         uint64_t fLastLocalVers; ///! last local version, used to build global list

         typedef std::list<PublisherEntry> PublishersList;

         typedef std::list<SubscriberEntry> SubscribersList;

         PublishersList fPublishers;

         SubscribersList fSubscribers;

         unsigned fCnt;

         Hierarchy fMgrHiearchy; ///! this is manager hierarchy, published by ourselfs

         virtual int ExecuteCommand(Command cmd);

         virtual bool ReplyCommand(Command cmd);

         virtual void ProcessTimerEvent(unsigned timer);

         void CheckDnsSubscribers();

         /** \brief Method marks that global version is out of date and should be rebuild */
         void InvalidateGlobal();

         virtual void BeforeModuleStart();

         bool ApplyEntryDiff(unsigned recid, dabc::Buffer& buf, uint64_t version, bool witherror = false);

      public:


         Publisher(const std::string& name, dabc::Command cmd = 0);

         static const char* DfltName() { return "Publisher"; }
   };



   class PublisherRef : public ModuleAsyncRef {
      DABC_REFERENCE(PublisherRef, ModuleAsyncRef, Publisher)

      bool Register(const std::string& path, const std::string& workername, void* hierarchy, void* mutex = 0)
      {  return OwnCommand(1, path, workername, hierarchy, mutex); }

      bool Unregister(const std::string& path, const std::string& workername, void* hierarchy)
      {  return OwnCommand(2, path, workername, hierarchy); }

      bool Subscribe(const std::string& path, const std::string& workername)
      {  return OwnCommand(3, path, workername); }

      bool Unsubscribe(const std::string& path, const std::string& workername)
      {  return OwnCommand(4, path, workername); }

      bool RemoveWorker(const std::string& workername, bool sync = true)
      {  return OwnCommand(sync ? 5 : -5, "", workername); }

      bool AddRemote(const std::string& remnode, const std::string& workername)
      {  return OwnCommand(6, remnode, workername); }

      bool SaveGlobalNamesListAsXml(std::string& str);

      Hierarchy Get(const std::string& fullname, double tmout = 5.);

      Hierarchy Get(const std::string& fullname, uint64_t version, unsigned hlimit, double tmout = 5.);

      Buffer GetBinary(const std::string& fullname, uint64_t version, double tmout = 5.);


      protected:

      bool OwnCommand(int id, const std::string& path, const std::string& workername, void* hier = 0, void* mutex = 0)
      {
         if (null()) return false;

         bool sync = id > 0;
         if (!sync) id = -id;

         Command cmd("OwnCommand");
         cmd.SetInt("cmdid", id);
         cmd.SetStr("Path", path);
         cmd.SetStr("Worker", workername);
         cmd.SetPtr("Hierarchy", hier);
         cmd.SetPtr("Mutex", mutex);

         if (!sync) return Submit(cmd);

         return Execute(cmd) == cmd_true;

      }

   };

}

#endif

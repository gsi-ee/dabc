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

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#include <list>

namespace dabc {

   class PublisherRef;
   class HierarchyStore;

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
      uint64_t version;     // last requested version
      uint64_t lastglvers;  // last version, used to build global
      bool local;           // is entry from local node
      int  errcnt;          // counter of consequent errors
      bool waiting_publisher; // indicate if next request is submitted
      Hierarchy rem;        // remote hierarchy
      HierarchyStore* store; // store object for the registered hierarchy

      PublisherEntry() :
         id(0), path(), worker(), fulladdr(), hier(0),
         version(0), lastglvers(0), local(true), errcnt(0), waiting_publisher(false), rem(),
         store(0) {}

      // implement copy constructor to avoid any extra copies which are not necessary
      PublisherEntry(const PublisherEntry& src) :
         id(src.id), path(), worker(), fulladdr(), hier(0),
         version(0), lastglvers(0), local(true), errcnt(0), waiting_publisher(false), rem(),
         store(0) {}

      ~PublisherEntry();

      // implement assign operator to avoid any extra copies which are not necessary
      PublisherEntry& operator=(const PublisherEntry& src)
      {
         id = src.id;
         return *this;
      }
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

   class Publisher : public dabc::Worker {

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

         std::string fStoreDir;   ///! directory to store data
         std::string fStoreSel;   ///! selected hierarchy path for storage like 'MBS' or 'FESA/server'
         int         fFileLimit;  ///! maximum size of store file, in MB
         int         fTimeLimit;  ///! maximum time of store file, in seconds
         double      fStorePeriod; ///! how often storage is triggered

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         virtual int ExecuteCommand(Command cmd);

         virtual bool ReplyCommand(Command cmd);


         void CheckDnsSubscribers();

         /** \brief Method marks that global version is out of date and should be rebuild */
         void InvalidateGlobal();

         bool ApplyEntryDiff(unsigned recid, dabc::Buffer& buf, uint64_t version, bool witherror = false);

         bool DoStorage() const { return !fStoreDir.empty(); }

         /** \brief Return hierarchy item selected for work */
         Hierarchy GetWorkItem(const std::string& path);

         /** \brief Command redirected to local modules or remote publisher,
          * where it should be processed
          * Primary usage - for GetBinary commands, but also for any extra requests */
         bool RedirectCommand(dabc::Command cmd, const std::string& itemname);

         /** Try to find producer which potentially could deliver item
          * It could happen that item is not exists in hierarchy, therefore
          * shorter name will be tried */
         bool IdentifyProducer(const std::string& itemname, bool& islocal, std::string& producer_name, std::string& request_name);

      public:

         Publisher(const std::string& name, dabc::Command cmd = 0);

         static const char* DfltName() { return "/publ"; }

         virtual const char* ClassName() const { return "Publisher"; }
   };

   // ==============================================================

   class PublisherRef : public WorkerRef {
      DABC_REFERENCE(PublisherRef, WorkerRef, Publisher)

      bool Register(const std::string& path, const std::string& workername, void* hierarchy)
      {  return OwnCommand(1, path, workername, hierarchy); }

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

      bool SaveGlobalNamesListAsXml(const std::string& path, std::string& str);

      /** Returns 0 - no childs, 1 - has childs, -1 - uncknown */
      int HasChilds(const std::string& path);

      Hierarchy Get(const std::string& fullname, double tmout = 5.);

      Hierarchy Get(const std::string& fullname, uint64_t version, unsigned hlimit, double tmout = 5.);

      Buffer GetBinary(const std::string& fullname, uint64_t version, double tmout = 5.);

      /** \brief Execute item is command, providing parameters in query */
      Command ExeCmd(const std::string& fullname, const std::string& query);


      protected:

      bool OwnCommand(int id, const std::string& path, const std::string& workername, void* hier = 0)
      {
         if (null()) return false;

         bool sync = id > 0;
         if (!sync) id = -id;

         Command cmd("OwnCommand");
         cmd.SetInt("cmdid", id);
         cmd.SetStr("Path", path);
         cmd.SetStr("Worker", workername);
         cmd.SetPtr("Hierarchy", hier);

         if (!sync) return Submit(cmd);

         return Execute(cmd) == cmd_true;

      }

   };

}

#endif

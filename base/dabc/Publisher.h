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

namespace dabc {

   class PublisherRef;
   class HierarchyStore;

   class CmdPublisher : public Command {
      DABC_COMMAND(CmdPublisher, "CmdPublisher");
   };

   /** Command used to produce custom binary data for published in hierarchy entries */
   class CmdGetBinary : public Command {
      DABC_COMMAND(CmdGetBinary, "CmdGetBinary");

      CmdGetBinary(const std::string &path, const std::string &kind, const std::string &query) :
         Command(CmdName())
      {
         SetStr("Item", path);
         SetStr("Kind", kind);
         SetStr("Query", query);
      }
   };

   /** Command submitted to worker when item in hierarchy defined as DABC.Command
    * and used to produce custom binary data for published in hierarchy entries */
   class CmdHierarchyExec : public Command {
      DABC_COMMAND(CmdHierarchyExec, "CmdHierarchyExec");

      CmdHierarchyExec(const std::string &path) :
         Command(CmdName())
      {
         SetStr("Item", path);
      }
   };

   /** Command to request names list */
   class CmdGetNamesList : public Command {
      DABC_COMMAND(CmdGetNamesList, "CmdGetNamesList");

      CmdGetNamesList(const std::string &kind, const std::string &path, const std::string &query) :
         Command(CmdName())
      {
         SetStr("textkind", kind);
         SetStr("path", path);
         SetStr("query", query);
      }

      void AddHeader(const std::string &name, const std::string &value, bool withquotes = true)
      {
         int num = GetInt("NumHdrs");
         SetStr(dabc::format("OptHdrName%d", num), name);
         SetStr(dabc::format("OptHdrValue%d", num), withquotes ? std::string("\"") + value + "\"" : value);
         SetInt("NumHdrs", num+1);
      }

      static void SetResNamesList(dabc::Command &cmd, Hierarchy& res);

      static Hierarchy GetResNamesList(dabc::Command &cmd);
   };


   class CmdSubscriber : public Command {
      DABC_COMMAND(CmdSubscriber, "CmdSubscriber");

      void SetEntry(const Hierarchy& e) { SetRef("entry", e); }
      Hierarchy GetEntry() { return GetRef("entry"); }
   };

   struct PublisherEntry {
      unsigned id{0};                 // unique id in the worker
      std::string path;               // absolute path in hierarchy
      std::string worker;             // worker which is manages hierarchy
      std::string fulladdr;           // address, used to identify producer of the hierarchy branch
      void* hier{nullptr};            // local hierarchy pointer, one not allowed to use it from publisher
      uint64_t version{0};            // last requested version
      uint64_t lastglvers{0};         // last version, used to build global
      bool local{false};              // is entry from local node
      bool mgrsubitem{false};         // if true, belongs to manager hierarchy
      int  errcnt{0};                 // counter of consequent errors
      bool waiting_publisher{false};  // indicate if next request is submitted
      Hierarchy rem;                  // remote hierarchy
      HierarchyStore* store{nullptr}; // store object for the registered hierarchy

      PublisherEntry() :
         id(0), path(), worker(), fulladdr(), hier(nullptr),
         version(0), lastglvers(0), local(true), mgrsubitem(false),
         errcnt(0), waiting_publisher(false), rem(), store(nullptr) {}

      // implement copy constructor to avoid any extra copies which are not necessary
      PublisherEntry(const PublisherEntry& src) :
         id(src.id), path(), worker(), fulladdr(), hier(nullptr),
         version(0), lastglvers(0), local(true), mgrsubitem(false),
         errcnt(0), waiting_publisher(false), rem(), store(nullptr) {}

      ~PublisherEntry();

      // implement assign operator to avoid any extra copies which are not necessary
      PublisherEntry& operator=(const PublisherEntry& src)
      {
         id = src.id;
         return *this;
      }
   };

   struct SubscriberEntry {
      unsigned id{0};              // unique id in the worker
      std::string path;            // absolute path in hierarchy
      std::string worker;          // worker which is manages hierarchy
      bool local{false};           // is entry from local node
      int hlimit{0};               // history limit -1 - DNS, 0 - no history
      Hierarchy fEntry;            // entry which is provided to the worker as result
      uint64_t version{0};         // last version provided to subscriber
      bool waiting_worker{false};  // when enabled, we are waiting while worker process entry

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

         uint64_t fLastLocalVers{0}; ///! last local version, used to build global list

         typedef std::list<PublisherEntry> PublishersList;

         typedef std::list<SubscriberEntry> SubscribersList;

         PublishersList fPublishers;

         SubscribersList fSubscribers;

         unsigned fCnt{0};            ///! counter for new records

         std::string fMgrPath;     ///! path for manager
         Hierarchy   fMgrHiearchy; ///! this is manager hierarchy, published by ourselfs

         std::string fStoreDir;   ///! directory to store data
         std::string fStoreSel;   ///! selected hierarchy path for storage like 'MBS' or 'FESA/server'
         int         fFileLimit{0};  ///! maximum size of store file, in MB
         int         fTimeLimit{0};  ///! maximum time of store file, in seconds
         double      fStorePeriod{0}; ///! how often storage is triggered

         void OnThreadAssigned() override;

         double ProcessTimeout(double last_diff) override;

         int ExecuteCommand(Command) override;

         bool ReplyCommand(Command cmd) override;

         dabc::Command CreateExeCmd(const std::string &path, const std::string &query, dabc::Command tgt = nullptr);

         void CheckDnsSubscribers();

         /** \brief Method marks that global version is out of date and should be rebuild */
         void InvalidateGlobal();

         bool ApplyEntryDiff(unsigned recid, dabc::Buffer& buf, uint64_t version, bool witherror = false);

         bool DoStorage() const { return !fStoreDir.empty(); }

         /** \brief Return hierarchy item selected for work */
         Hierarchy GetWorkItem(const std::string &path, bool *islocal = nullptr);

         /** \brief Command redirected to local modules or remote publisher,
          * where it should be processed
          * Primary usage - for GetBinary commands, but also for any extra requests */
         bool RedirectCommand(dabc::Command cmd, const std::string &itemname);

         /** Try to find producer which potentially could deliver item
          * It could happen that item is not exists in hierarchy, therefore
          * shorter name will be tried */
         bool IdentifyItem(bool asproducer, const std::string &itemname, bool &islocal, std::string &producer_name, std::string &request_name);

      public:

         Publisher(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~Publisher();

         static const char *DfltName() { return "/publ"; }

         const char *ClassName() const override { return "Publisher"; }
   };

   // ==============================================================

   class PublisherRef : public WorkerRef {
      DABC_REFERENCE(PublisherRef, WorkerRef, Publisher)

      bool Register(const std::string &path, const std::string &workername, void* hierarchy)
      {  return OwnCommand(1, path, workername, hierarchy); }

      bool Unregister(const std::string &path, const std::string &workername, void* hierarchy)
      {  return OwnCommand(2, path, workername, hierarchy); }

      bool Subscribe(const std::string &path, const std::string &workername)
      {  return OwnCommand(3, path, workername); }

      bool Unsubscribe(const std::string &path, const std::string &workername)
      {  return OwnCommand(4, path, workername); }

      bool RemoveWorker(const std::string &workername, bool sync = true)
      {  return OwnCommand(sync ? 5 : -5, "", workername); }

      bool AddRemote(const std::string &remnode, const std::string &workername)
      {  return OwnCommand(6, remnode, workername); }

      /** Returns "" - undefined,
       *          "__tree__"    -- tree hierarchy
       *          "__single__"  -- single element
       *          "__file__"    -- just a file name */
      std::string UserInterfaceKind(const char *uri, std::string &path, std::string &fname);

      /** Returns 1 - need auth,  0 - no need auth, -1 - undefined */
      int NeedAuth(const std::string &path);

      /** Return different kinds of binary data, depends from kind */
      Buffer GetBinary(const std::string &fullname, const std::string &kind, const std::string &query, double tmout = 5.);

      Hierarchy GetItem(const std::string &fullname, const std::string &query = "", double tmout = 5.);

      /** \brief Execute item is command, providing parameters in query */
      Command ExeCmd(const std::string &fullname, const std::string &query);

      protected:
         bool OwnCommand(int id, const std::string &path, const std::string &workername, void *hier = nullptr);
   };

}

#endif

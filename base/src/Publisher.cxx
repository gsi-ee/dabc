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

#include "dabc/Publisher.h"

#include "dabc/Manager.h"
#include "dabc/Url.h"
#include "dabc/HierarchyStore.h"


void dabc::CmdGetNamesList::SetResNamesList(dabc::Command& cmd, Hierarchy& res)
{
   if (cmd.GetBool("asxml")) {
      std::string str = res.SaveToXml(dabc::xmlmask_TopDabc, cmd.GetStr("path"));
      cmd.SetStr("xml", str);
   } else {
      dabc::Buffer buf = res.SaveToBuffer(dabc::stream_NamesList);
      cmd.SetRawData(buf);
   }
}


// ==========================================================

dabc::PublisherEntry::~PublisherEntry()
{
   if (store!=0) {
      store->CloseFile();
      delete store;
      store = 0;
   }
}

// ======================================================================

dabc::Publisher::Publisher(const std::string& name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fGlobal(),
   fLocal(),
   fLastLocalVers(0),
   fPublishers(),
   fSubscribers(),
   fCnt(0),
   fMgrPath(),
   fMgrHiearchy()
{
   fLocal.Create("LOCAL");

   if (Cfg("manager",cmd).AsBool(false))
      fMgrHiearchy.Create("Manager");

   fStoreDir = Cfg("storedir", cmd).AsStr();
   fStoreSel = Cfg("storesel", cmd).AsStr();
   fFileLimit = Cfg("filelimit", cmd).AsInt(100);
   fTimeLimit = Cfg("timelimit", cmd).AsInt(600);
   fStorePeriod = Cfg("period",cmd).AsDouble(5.);

   if (!Cfg("store", cmd).AsBool()) fStoreDir.clear();

   DOUT3("PUBLISHER name:%s item:%s class:%s mgr:%s", GetName(), ItemName().c_str(), ClassName(), DBOOL(!fMgrHiearchy.null()));
}

void dabc::Publisher::OnThreadAssigned()
{
//   fMgrPath = "DABC/localhost";

   fMgrPath = "DABC/";
   std::string addr = dabc::mgr.GetLocalAddress();
   size_t pos = addr.find(":");
   if (pos<addr.length()) addr[pos]='_';
   fMgrPath += addr;

   if (!fMgrHiearchy.null()) {
      DOUT3("dabc::Publisher::BeforeModuleStart mgr path %s", fMgrPath.c_str());
      fPublishers.push_back(PublisherEntry());
      fPublishers.back().id = fCnt++;
      fPublishers.back().path = fMgrPath;
      fPublishers.back().worker = ItemName();
      fPublishers.back().fulladdr = WorkerAddress(true);
      fPublishers.back().hier = fMgrHiearchy();
      fPublishers.back().local = true;

      fLocal.GetFolder(fMgrPath, true);
   }

   ActivateTimeout(0.1);
}

void dabc::Publisher::InvalidateGlobal()
{
   fLastLocalVers = 0;

   for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
      if (!iter->local) iter->lastglvers = 0;
   }
}


double dabc::Publisher::ProcessTimeout(double last_diff)
{
//   DOUT0("dabc::Publisher::ProcessTimerEvent");

   bool is_any_global(false);
   bool rebuild_global = fLocal.GetVersion() > fLastLocalVers;
/*
   static int mycnt = 0;
   if ((mycnt++ % 20 == 0) && !fStoreDir.empty()) {
      HierarchyReading rr;
      rr.SetBasePath(fStoreDir);
      DOUT0("-------------- DO SCAN -------------");
      rr.ScanTree();

      dabc::Hierarchy hh;
      rr.GetStrucutre(hh);

      DOUT0("GOT\n%s", hh.SaveToXml().c_str());

      dabc::DateTime from;
      from.SetOnlyDate("2013-09-18");
      from.SetOnlyDate("13:05:00");

      dabc::DateTime till = from;
      till.SetOnlyDate("14:05:00");

      dabc::Hierarchy sel = rr.GetSerie("/FESA/Test/TestRate", from, till);

      if (!sel.null())
         DOUT0("SELECT\n%s",sel.SaveToXml(dabc::xmlmask_History).c_str());
      else
         DOUT0("???????? SELECT FAILED ?????????");


      DOUT0("-------------- DID SCAN -------------");
   }
*/
   DateTime storetm; // stamp  used to mark time when next store operation was triggered

   for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {

      if (!iter->local) {
         is_any_global = true;
         if (iter->version > iter->lastglvers) rebuild_global = true;
      }

      if (iter->waiting_publisher) continue;

      iter->waiting_publisher = true;

      if (iter->hier == fMgrHiearchy())
      {
         // first, generate current objects hierarchy
         dabc::Hierarchy curr;
         curr.BuildNew(dabc::mgr);
         curr.SetField(prop_producer, WorkerAddress());

         // than use update to mark all changes
         fMgrHiearchy.Update(curr);

         // DOUT0("MANAGER %u\n %s", fMgrHiearchy.GetVersion(), fMgrHiearchy.SaveToXml().c_str());

         // generate diff to the last requested version
         Buffer diff = fMgrHiearchy.SaveToBuffer(dabc::stream_NamesList, iter->version);

         // and finally, apply diff to the main hierarchy
         ApplyEntryDiff(iter->id, diff, fMgrHiearchy.GetVersion());

      } else
      if (iter->local) {
         CmdPublisher cmd;
         bool dostore = false;
         cmd.SetReceiver(iter->worker);
         cmd.SetUInt("version", iter->version);
         cmd.SetPtr("hierarchy", iter->hier);
         cmd.SetUInt("recid", iter->id);
         if (iter->store && iter->store->CheckForNextStore(storetm, fStorePeriod, fTimeLimit)) {
            cmd.SetPtr("store", iter->store);
            dostore = true;
         }
         cmd.SetTimeout(dostore ? 50. : 5.);
         dabc::mgr.Submit(Assign(cmd));
      } else {
         Command cmd("GetLocalHierarchy");
         cmd.SetReceiver(iter->fulladdr);
         cmd.SetUInt("version", iter->version);
         cmd.SetUInt("recid", iter->id);
         cmd.SetTimeout(10.);
         dabc::mgr.Submit(Assign(cmd));
      }

//      DOUT0("Submit command to worker %s id %u", iter->worker.c_str(), iter->id);
   }

   if (rebuild_global && is_any_global) {
      // recreate global structures again

      fGlobal.Release();
      fGlobal.Create("Global");

      //DOUT0("LOCAL version before:%u now:%u", (unsigned) fLastLocalVers, (unsigned) fLocal.GetVersion());

      fGlobal.Duplicate(fLocal);
      fLastLocalVers = fLocal.GetVersion();

      for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {

         if (iter->local || (iter->version==0)) continue;

         //DOUT0("REMOTE %s version before:%u now:%u", iter->fulladdr.c_str(), (unsigned) iter->lastglvers, (unsigned) iter->version);

         fGlobal.Duplicate(iter->rem);

         iter->lastglvers = iter->version;
      }

      //DOUT0("GLOBAL\n%s", fGlobal.SaveToXml().c_str());
   } else
   if (!is_any_global) {
      fGlobal.Release();
      fLastLocalVers = 0;
   }

   for (SubscribersList::iterator iter = fSubscribers.begin(); iter != fSubscribers.end(); iter++) {
      if (iter->waiting_worker) continue;

      if (iter->hlimit < 0) continue;

      // here direct request can be submitted, do it later, may be even not here
   }

   return 0.5;
}

void dabc::Publisher::CheckDnsSubscribers()
{
   for (SubscribersList::iterator iter = fSubscribers.begin(); iter != fSubscribers.end(); iter++) {
      if (iter->waiting_worker) continue;

      if (iter->hlimit >= 0) continue;

      dabc::Hierarchy h = iter->local ? fLocal.GetFolder(iter->path) : fGlobal.GetFolder(iter->path);

      if (h.null()) { EOUT("Subscribed path %s not found", iter->path.c_str()); continue; }
   }
}

bool dabc::Publisher::ApplyEntryDiff(unsigned recid, dabc::Buffer& diff, uint64_t version, bool witherror)
{
   PublishersList::iterator iter = fPublishers.begin();
   while (iter != fPublishers.end()) {
      if (iter->id == recid) break;
      iter++;
   }

   if (iter == fPublishers.end()) {
      EOUT("Get reply for non-existing id %u", recid);
      return false;
   }

   iter->waiting_publisher = false;

   if (witherror) {
      iter->errcnt++;
      EOUT("Command failed for rec %u addr %s errcnt %d", recid, iter->fulladdr.c_str(), iter->errcnt);
      return false;
   }

   iter->errcnt = 0;
   iter->version = version;

   if (iter->local) {

      dabc::Hierarchy top = fLocal.GetFolder(iter->path);
      if (!top.null()) {
         // we ensure that update of that item in manager hierarchy will not change its properties
         top.UpdateFromBuffer(diff);
         top.SetField(prop_producer, iter->fulladdr);
         if (iter->mgrsubitem) top.DisableReadingAsChild();

      } else {
         EOUT("Did not found local folder %s ", iter->path.c_str());
      }
   } else {
      iter->rem.UpdateFromBuffer(diff);
   }

   DOUT5("LOCAL ver %u diff %u itemver %u \n%s",  fLocal.GetVersion(), diff.GetTotalSize(), iter->version, fLocal.SaveToXml(2).c_str());

   // check if hierarchy was changed
   CheckDnsSubscribers();

   return true;
}


bool dabc::Publisher::ReplyCommand(Command cmd)
{
   if (cmd.IsName(CmdPublisher::CmdName())) {
      dabc::Buffer diff = cmd.GetRawData();

      ApplyEntryDiff(cmd.GetUInt("recid"), diff, cmd.GetUInt("version"), cmd.GetResult() != cmd_true);

      return true;
   } else
   if (cmd.IsName("GetLocalHierarchy")) {
      dabc::Buffer diff = cmd.GetRawData();

      ApplyEntryDiff(cmd.GetUInt("recid"), diff, cmd.GetUInt("version"), cmd.GetResult() != cmd_true);

      return true;
   }

   return dabc::Worker::ReplyCommand(cmd);
}


dabc::Hierarchy dabc::Publisher::GetWorkItem(const std::string& path, bool* islocal)
{

   dabc::Hierarchy top = fGlobal.null() ? fLocal : fGlobal;

   if (islocal) *islocal = fGlobal.null();

   if (path.empty() || (path=="/")) return top;

   return top.FindChild(path.c_str());
}

bool dabc::Publisher::IdentifyProducer(const std::string& itemname, bool& islocal, std::string& producer_name, std::string& request_name)
{
   if (itemname.length()==0) return false;

   dabc::Hierarchy h = fLocal.GetFolder(itemname);
   if (!h.null()) {
      // we need to redirect command to appropriate worker (or to ourself)
      // for local producers we need to find last (maximal depth) producer
      producer_name = h.FindBinaryProducer(request_name, false);
      DOUT3("Producer:%s request:%s item:%s", producer_name.c_str(), request_name.c_str(), itemname.c_str());

      islocal = true;
   } else
   for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
      if (iter->local) continue;

      h = iter->rem.GetFolder(itemname);
      if (h.null()) continue;

      // we need to redirect command to remote node

      producer_name = h.FindBinaryProducer(request_name);
      islocal = false;
      break;
   }

   if (!h.null()) return !producer_name.empty();

   std::string item1 = itemname;
   while (item1[item1.length()-1] == '/') item1.resize(item1.length()-1);
   size_t pos = item1.find_last_of("/");
   if ((pos == 0) || (pos == std::string::npos)) return false;

   std::string sub = item1.substr(pos);
   item1.resize(pos);

   if (IdentifyProducer(item1, islocal, producer_name, request_name)) {
      request_name.append(sub);
      return true;
   }

   return false;
}



bool dabc::Publisher::RedirectCommand(dabc::Command cmd, const std::string& itemname)
{
   std::string producer_name, request_name;
   bool islocal(true);

   DOUT3("PUBLISHER CMD %s ITEM %s", cmd.GetName(), itemname.c_str());

   if (!IdentifyProducer(itemname, islocal, producer_name, request_name)) {
      EOUT("Not found producer for item %s", itemname.c_str());
      return false;
   }

   DOUT3("PRODUCER %s REQUEST %s", producer_name.c_str(), request_name.c_str());

   bool producer_local(true);
   std::string producer_server, producer_item;

   if (!dabc::mgr.DecomposeAddress(producer_name, producer_local, producer_server, producer_item)) {
      EOUT("Wrong address specified as producer %s", producer_name.c_str());
      return false;
   }

   if (islocal || producer_local) {
      // this is local case, we need to redirect command to the appropriate worker
      // but first we should locate hierarchy which is assigned with the worker

      for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
         if (!iter->local) continue;

         if ((iter->worker != producer_item) && (iter->worker != std::string("/") + producer_item)) continue;

         // we redirect command to local worker
         // manager should find proper worker for execution

         DOUT2("Submit GET command to %s subitem %s", producer_item.c_str(), request_name.c_str());
         cmd.SetReceiver(iter->worker);
         cmd.SetPtr("hierarchy", iter->hier);
         cmd.SetStr("subitem", request_name);
         dabc::mgr.Submit(cmd);
         return true;
      }

      EOUT("Not found producer %s, which is correspond to item %s", producer_item.c_str(), itemname.c_str());
      return false;
   }

   if (cmd.GetBool("analyzed")) {
      EOUT("Command to get item %s already was analyzed - something went wrong", itemname.c_str());
      return false;
   }

   cmd.SetReceiver(dabc::mgr.ComposeAddress(producer_server, dabc::Publisher::DfltName()));
   cmd.SetBool("analyzed", true);
   dabc::mgr.Submit(cmd);
   return true;
}


int dabc::Publisher::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("OwnCommand")) {

      std::string path = cmd.GetStr("Path");
      std::string worker = cmd.GetStr("Worker");
      bool ismgrpath = false;
      if (path.find("$MGR$")==0) {
         ismgrpath = true;
         path.erase(0, 5);
         path = fMgrPath + path;
      }

      switch (cmd.GetInt("cmdid")) {
         case 1: { // REGISTER

            for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
               if (iter->path == path) {
                  EOUT("Path %s already registered!!!", path.c_str());
                  return cmd_false;
               }
            }

            dabc::Hierarchy h = fLocal.GetFolder(path);
            if (!h.null()) {
               if (ismgrpath) {
                  DOUT0("Path %s is registered in manager hierarchy, treat it individually", path.c_str());
                  h.DisableReadingAsChild();
               } else {
                  EOUT("Path %s already present in the hierarchy", path.c_str());
                  return cmd_false;
               }
            }

            DOUT0("PUBLISH folder %s", path.c_str());

            fPublishers.push_back(PublisherEntry());
            fPublishers.back().id = fCnt++;
            fPublishers.back().path = path;
            fPublishers.back().worker = worker;
            fPublishers.back().fulladdr = dabc::mgr.ComposeAddress("", worker);
            fPublishers.back().hier = cmd.GetPtr("Hierarchy");
            fPublishers.back().local = true;
            fPublishers.back().mgrsubitem = ismgrpath;

            if (!fStoreDir.empty()) {
               if (fStoreSel.empty() || (path.find(fStoreSel) == 0)) {
                  DOUT1("Create store for %s", path.c_str());
                  fPublishers.back().store = new HierarchyStore();
                  fPublishers.back().store->SetBasePath(fStoreDir + path);
               }
            }

            fLocal.GetFolder(path, true);
            // set immediately producer
            // h.SetField(prop_producer, fPublishers.back().fulladdr);

            // ShootTimer("Timer");

            return cmd_true;
         }

         case 2:  { // UNREGISTER
            bool find = false;
            for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
               if (iter->local && (iter->path == path) && (iter->worker == worker)) {

                  if (!fLocal.RemoveEmptyFolders(path))
                     EOUT("Not found local entry with path %s", path.c_str());

                  fPublishers.erase(iter);
                  find = true;
                  break;
               }
            }

            return cmd_bool(find);
         }

         case 3: {   // SUBSCRIBE

            bool islocal = true;

            dabc::Hierarchy h = fLocal.GetFolder(path);
            if (h.null()) { h = fGlobal.GetFolder(path); islocal = false; }
            if (h.null()) {
               EOUT("Path %s not exists", path.c_str());
               return cmd_false;
            }

            fSubscribers.push_back(SubscriberEntry());

            SubscriberEntry& entry = fSubscribers.back();

            entry.id = fCnt++;
            entry.path = path;
            entry.worker = worker;
            entry.local = islocal;
            entry.hlimit = 0;

            return cmd_true;
         }

         case 4: {   // UNSUBSCRIBE

            SubscribersList::iterator iter = fSubscribers.begin();
            while (iter != fSubscribers.end()) {
               if ((iter->worker == worker) && (iter->path == path))
                  fSubscribers.erase(iter++);
               else
                  iter++;
            }

            return cmd_true;
         }

         case 5: {  // REMOVE WORKER

            DOUT4("REMOVE WORKER %s", worker.c_str());

            PublishersList::iterator iter = fPublishers.begin();
            while (iter!=fPublishers.end()) {
               if (iter->worker == worker)
                  fPublishers.erase(iter++);
               else
                  iter++;
            }

            SubscribersList::iterator iter2 = fSubscribers.begin();
            while (iter2 != fSubscribers.end()) {
               if (iter2->worker == worker)
                  fSubscribers.erase(iter2++);
               else
                  iter2++;
            }

            return cmd_true;
         }

         case 6: {  // ADD REMOTE NODE

            std::string remoteaddr = dabc::mgr.ComposeAddress(path, dabc::Publisher::DfltName());

            for (PublishersList::iterator iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
               if ((iter->fulladdr == remoteaddr) && !iter->local) {
                  EOUT("Path %s already registered!!!", path.c_str());
                  return cmd_false;
               }
            }

            fPublishers.push_back(PublisherEntry());
            fPublishers.back().id = fCnt++;
            fPublishers.back().path = "";
            fPublishers.back().worker = worker;
            fPublishers.back().fulladdr = remoteaddr;
            fPublishers.back().hier = 0;
            fPublishers.back().local = false;
            fPublishers.back().rem.Create("remote");

            DOUT0("PUBLISH NODE %s", path.c_str());

            return cmd_true;
         }

         default:
           EOUT("BAD OwnCommand ID");
           return cmd_false;
      }
   } else
   if (cmd.IsName("GetLocalHierarchy")) {

      Buffer diff = fLocal.SaveToBuffer(dabc::stream_NamesList, cmd.GetUInt("version"));

      cmd.SetRawData(diff);
      cmd.SetUInt("version", fLocal.GetVersion());

      return cmd_true;
   } else
   if (cmd.IsName(CmdGetNamesList::CmdName())) {
      std::string path = cmd.GetStr("path");

      dabc::Hierarchy h = GetWorkItem(path);

      // if item was not found directly, try to ask producer if it can be extended
      if (h.null() || h.HasField(dabc::prop_more)) {
         if (!RedirectCommand(cmd, path)) return cmd_false;
         DOUT3("ITEM %s CAN PROVIDE MORE!!!", path.c_str());
         return cmd_postponed;
      }

      dabc::CmdGetNamesList::SetResNamesList(cmd, h);

      return cmd_true;
   } else
   if (cmd.IsName("CreateExeCmd")) {
      std::string path = cmd.GetStr("path");

      bool islocal(true);
      dabc::Hierarchy def = GetWorkItem(path, &islocal);
      if (def.null()) return cmd_false;

      if (def.Field(dabc::prop_kind).AsStr() !="DABC.Command") return cmd_false;

      std::string request_name;
      std::string producer_name = def.FindBinaryProducer(request_name, !islocal);
      if (producer_name.empty()) return cmd_false;

      dabc::Command res = dabc::Command(def.GetName());

      dabc::Url url(std::string("execute?") + cmd.GetStr("query"));

      if (url.IsValid()) {
         int cnt = 0;
         std::string part;
         do {
            part = url.GetOptionsPart(cnt++);
            if (part.empty()) break;

            size_t p = part.find("=");
            if ((p==std::string::npos) || (p==0) || (p==part.length()-1)) break;

            std::string parname = part.substr(0,p);
            std::string parvalue = part.substr(p+1);

            size_t pos;
            while ((pos = parvalue.find("%20")) != std::string::npos)
               parvalue.replace(pos, 3, " ");

            res.SetStr(parname, parvalue);

            // DOUT0("************ CMD ARG %s = %s", part.substr(0,p-1).c_str(), part.substr(p+1).c_str());

         } while (!part.empty());
      }

      res.SetReceiver(producer_name);

      cmd.SetRef("ExeCmd", res);

      return cmd_true;
   } else
   if (cmd.IsName("CmdHasChilds")) {
      std::string path = cmd.GetStr("path");
      dabc::Hierarchy h = GetWorkItem(path);
      cmd.SetInt("num_childs", h.null() ? -1 : h.NumChilds());

      return cmd_true;

   } else
   if (cmd.IsName(CmdGetBinary::CmdName())) {

      // if we get command here, we need to find destination for it

      std::string itemname = cmd.GetStr("Item");

      if (!RedirectCommand(cmd, itemname)) return cmd_false;

      return cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

// ===================================================================

bool dabc::PublisherRef::SaveGlobalNamesListAsXml(const std::string& path, std::string& str)
{
   if (null()) return false;

   CmdGetNamesList cmd;
   cmd.SetBool("asxml", true);
   cmd.SetStr("path", path);

   if (Execute(cmd) != cmd_true) return false;

   str = cmd.GetStr("xml");

   return true;
}

int dabc::PublisherRef::HasChilds(const std::string& path)
{
   if (null()) return -1;

   dabc::Command cmd("CmdHasChilds");
   cmd.SetStr("path", path);

   if (Execute(cmd) != cmd_true) return -1;

   return cmd.GetInt("num_childs");
}


dabc::Command dabc::PublisherRef::ExeCmd(const std::string& fullname, const std::string& query)
{
   dabc::Command res;
   if (null()) return res;

   dabc::Command cmd("CreateExeCmd");
   cmd.SetStr("path", fullname);
   cmd.SetStr("query", query);

   if (Execute(cmd) != cmd_true) return res;

   res = cmd.GetRef("ExeCmd");
   if (res.null()) return res;

   dabc::mgr.Execute(res);
   return res;
}


dabc::Buffer dabc::PublisherRef::GetBinary(const std::string& fullname, const std::string& kind, const std::string& query, double tmout)
{
   if (null()) return 0;

   CmdGetBinary cmd(fullname, kind, query);
   cmd.SetTimeout(tmout);

   if (Execute(cmd) == cmd_true)
      return cmd.GetRawData();

   return 0;
}


dabc::Hierarchy dabc::PublisherRef::GetItem(const std::string& fullname, const std::string& query, double tmout)
{
   dabc::Hierarchy res;

   if (null()) return res;

   CmdGetBinary cmd(fullname, "hierarchy", query);
   cmd.SetTimeout(tmout);

   if (Execute(cmd) != cmd_true) return res;

   res.Create("get");
   res.SetVersion(cmd.GetUInt("version"));
   res.ReadFromBuffer(cmd.GetRawData());

   return res;
}

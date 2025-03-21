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

void dabc::CmdGetNamesList::SetResNamesList(dabc::Command &cmd, Hierarchy &res)
{
   if (res.null()) return;

   std::string kind = cmd.GetStr("textkind");

   if (kind.empty()) {
      dabc::Buffer buf = res.SaveToBuffer(dabc::stream_NamesList);
      cmd.SetRawData(buf);
   } else {
      unsigned mask = 0;

      dabc::Url url;
      url.SetOptions(cmd.GetStr("query"));

      if (url.HasOption("compact"))
         mask |= (url.GetOptionInt("compact", dabc::storemask_Compact) & dabc::storemask_Compact);

      if (kind == "xml") mask |= dabc::storemask_AsXML;

      dabc::NumericLocale loc;

      dabc::HStore store(mask);
      store.CreateNode(res.GetName());

      int num = cmd.GetInt("NumHdrs");
      for (int n=0;n<num;++n) {
         std::string name = cmd.GetStr(dabc::format("OptHdrName%d", n)),
                     value = cmd.GetStr(dabc::format("OptHdrValue%d", n));
         store.SetField(name.c_str(), value.c_str());
      }

      if (res.SaveTo(store, false)) {
         store.CloseNode(res.GetName());
         cmd.SetStr("astext", store.GetResult());
      }
   }
}

// ======================================================================

dabc::Hierarchy dabc::CmdGetNamesList::GetResNamesList(dabc::Command &cmd)
{
   dabc::Hierarchy res;

   dabc::Buffer buf = cmd.GetRawData();

   if (buf.null()) {
      EOUT("No raw data when requesting hierarchy");
      return res;
   }

   if (!res.ReadFromBuffer(buf)) {
      EOUT("Error decoding hierarchy data from buffer");
      res.Release();
   }

   return res;
}


// ==========================================================

dabc::PublisherEntry::~PublisherEntry()
{
   if (store) {
      store->CloseFile();
      delete store;
      store = nullptr;
   }
}

// ======================================================================

dabc::Publisher::Publisher(const std::string &name, dabc::Command cmd) :
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

dabc::Publisher::~Publisher()
{
}


void dabc::Publisher::OnThreadAssigned()
{
   dabc::Worker::OnThreadAssigned();

   fMgrPath = dabc::format("DABC/%s", dabc::mgr.GetName());

//   std::string addr = dabc::mgr.GetLocalAddress();
//   size_t pos = addr.find(":");
//   if (pos<addr.length()) addr[pos]='_';
//   fMgrPath = std::string("DABC/")+ addr;

   if (!fMgrHiearchy.null()) {
      DOUT3("dabc::Publisher::BeforeModuleStart mgr path %s", fMgrPath.c_str());
      fPublishers.emplace_back(PublisherEntry());
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

   for (auto &item: fPublishers) {
      if (!item.local)
         item.lastglvers = 0;
   }
}


double dabc::Publisher::ProcessTimeout(double)
{
//   DOUT0("dabc::Publisher::ProcessTimerEvent");

   // when application terminated - do not try to update any records
   if (dabc::mgr.IsTerminated()) return -1;

   bool is_any_global = false;
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
         DOUT0("SELECT\n%s",sel.SaveToXml(dabc::storemask_History).c_str());
      else
         DOUT0("???????? SELECT FAILED ?????????");


      DOUT0("-------------- DID SCAN -------------");
   }
*/
   DateTime storetm; // stamp  used to mark time when next store operation was triggered

   for (auto iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {

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

      for (auto iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {

         if (iter->local || (iter->version == 0)) continue;

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

   for (auto &entry : fSubscribers) {
      if (entry.waiting_worker) continue;

      if (entry.hlimit < 0) continue;

      // here direct request can be submitted, do it later, may be even not here
   }

   return 0.5;
}

void dabc::Publisher::CheckDnsSubscribers()
{
   for (auto &entry : fSubscribers) {
      if (entry.waiting_worker) continue;

      if (entry.hlimit >= 0) continue;

      dabc::Hierarchy h = entry.local ? fLocal.GetFolder(entry.path) : fGlobal.GetFolder(entry.path);

      if (h.null()) { EOUT("Subscribed path %s not found", entry.path.c_str()); continue; }
   }
}

bool dabc::Publisher::ApplyEntryDiff(unsigned recid, dabc::Buffer& diff, uint64_t version, bool witherror)
{
   auto iter = fPublishers.begin();
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

   DOUT5("LOCAL ver %u diff %u itemver %u \n%s",  fLocal.GetVersion(), diff.GetTotalSize(), iter->version, fLocal.SaveToXml().c_str());

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


dabc::Hierarchy dabc::Publisher::GetWorkItem(const std::string &path, bool* islocal)
{

   dabc::Hierarchy top = fGlobal.null() ? fLocal : fGlobal;

   if (islocal) *islocal = fGlobal.null();

   if (path.empty() || (path=="/")) return top;

   return top.FindChild(path.c_str());
}

bool dabc::Publisher::IdentifyItem(bool asproducer, const std::string &itemname, bool& islocal, std::string& producer_name, std::string& request_name)
{
   if (asproducer && itemname.empty()) return false;

   dabc::Hierarchy h = fLocal.GetFolder(itemname);
   if (!h.null()) {
      // we need to redirect command to appropriate worker (or to ourself)
      // for local producers we need to find last (maximal depth) producer
      if (asproducer)
         producer_name = h.FindBinaryProducer(request_name, false);
      else
         producer_name = itemname;
      DOUT3("Producer:%s request:%s item:%s", producer_name.c_str(), request_name.c_str(), itemname.c_str());

      islocal = true;
   } else
   for (auto &entry : fPublishers) {
      if (entry.local) continue;

      h = entry.rem.GetFolder(itemname);
      if (h.null()) continue;

      // we need to redirect command to remote node

      if (asproducer)
         producer_name = h.FindBinaryProducer(request_name);
      else
         producer_name = itemname;
      islocal = false;
      break;
   }

   if (!h.null() || (itemname.length()<2)) return !producer_name.empty() || !!asproducer;

   // DOUT0("Cut part from item %s", itemname.c_str());

   std::string item1 = itemname;
   //while (item1[item1.length()-1] == '/') item1.resize(item1.length()-1);
   size_t pos = item1.find_last_of("/", item1.length()-2);
   if (pos == std::string::npos) return false;
   // when searching producer, it cannot be root folder
   if ((pos == 0) && asproducer) return false;

   std::string sub = item1.substr(pos+1); // keep slash
   item1.resize(pos+1);

   // DOUT0("After cut item1 %s sub %s", item1.c_str(), sub.c_str());

   if (IdentifyItem(asproducer, item1, islocal, producer_name, request_name)) {
      if ((sub.length()>0) && (request_name.length()>0) && (request_name[request_name.length()-1]!='/')) request_name.append("/");
      request_name.append(sub);
      return true;
   }

   return false;
}


bool dabc::Publisher::RedirectCommand(dabc::Command cmd, const std::string &itemname)
{
   std::string producer_name, request_name;
   bool islocal = true;

   DOUT3("PUBLISHER CMD %s ITEM %s", cmd.GetName(), itemname.c_str());

   if (!IdentifyItem(true, itemname, islocal, producer_name, request_name)) {
      DOUT2("Not found producer for item %s", itemname.c_str());
      return false;
   }

   DOUT3("ITEM %s PRODUCER %s REQUEST %s", itemname.c_str(), producer_name.c_str(), request_name.c_str());

   bool producer_local = true;
   std::string producer_server, producer_item;

   if (!dabc::mgr.DecomposeAddress(producer_name, producer_local, producer_server, producer_item)) {
      EOUT("Wrong address specified as producer %s", producer_name.c_str());
      return false;
   }

   if (islocal || producer_local) {
      // this is local case, we need to redirect command to the appropriate worker
      // but first we should locate hierarchy which is assigned with the worker

      for (auto &entry : fPublishers) {
         if (!entry.local) continue;

         if ((entry.worker != producer_item) && (entry.worker != std::string("/") + producer_item)) continue;

         // we redirect command to local worker
         // manager should find proper worker for execution

         DOUT3("Submit GET command to %s subitem %s", producer_item.c_str(), request_name.c_str());
         cmd.SetReceiver(entry.worker);
         cmd.SetPtr("hierarchy", entry.hier);
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

   DOUT3("Submit command to Receiver %s", dabc::mgr.ComposeAddress(producer_server, dabc::Publisher::DfltName()).c_str());

   cmd.SetReceiver(dabc::mgr.ComposeAddress(producer_server, dabc::Publisher::DfltName()));
   cmd.SetBool("analyzed", true);
   dabc::mgr.Submit(cmd);
   return true;
}


dabc::Command dabc::Publisher::CreateExeCmd(const std::string &path, const std::string &query, dabc::Command res)
{
   bool islocal = true;
   dabc::Hierarchy def = GetWorkItem(path, &islocal);
   if (def.null()) return nullptr;

   if (def.Field(dabc::prop_kind).AsStr()!="DABC.Command") return nullptr;

   std::string request_name;
   std::string producer_name = def.FindBinaryProducer(request_name, !islocal);
   if (producer_name.empty()) return nullptr;

   if (def.GetField("_parcmddef").AsBool(false)) {
      DOUT3("Create normal command %s for path %s", def.GetName(), path.c_str());
      if (res.null()) {
         res = dabc::Command(def.GetName());
      } else {
         res.ChangeName(def.GetName());
      }
   } else {
      DOUT3("Create hierarchy command %s for path %s", request_name.c_str(), path.c_str());
      if (res.null()) {
         res = dabc::CmdHierarchyExec(request_name);
      } else {
         res.ChangeName(dabc::CmdHierarchyExec::CmdName());
         res.SetStr("Item", request_name);
      }
   }

   dabc::Url url(std::string("execute?") + query);

   if (url.IsValid()) {
      int cnt = 0;
      std::string part;
      do {
         part = url.GetOptionsPart(cnt++);
         if (part.empty()) break;

         size_t p = part.find("=");
         if ((p == std::string::npos) || (p == 0) || (p == part.length()-1)) break;

         std::string parname = part.substr(0,p);
         std::string parvalue = part.substr(p+1);

         size_t pos;
         while ((pos = parvalue.find("%20")) != std::string::npos)
            parvalue.replace(pos, 3, " ");

         bool quotes = false;

         if ((parvalue.length()>1) && ((parvalue[0]=='\'') || (parvalue[0]=='\"'))
               && (parvalue[0] == parvalue[parvalue.length()-1])) {
            parvalue.erase(0,1);
            parvalue.resize(parvalue.length()-1);
            quotes = true;
         }

         // DOUT0("parname %s parvalue %s", parname.c_str(), parvalue.c_str());

         if (quotes) {

            std::vector<std::string> vect;
            std::vector<double> dblvect;
            std::vector<int64_t> intvect;

            if (!dabc::RecordField::StrToStrVect(parvalue.c_str(), vect)) {
               res.SetStr(parname, parvalue);
               continue;
            }

            if (vect.empty()) {
               res.SetField(parname, parvalue);
               continue;
            }

            for (unsigned n=0;n<vect.size();n++) {
               double ddd = 0.;
               long long iii = 0;
               if (str_to_double(vect[n].c_str(), &ddd))
                  dblvect.emplace_back(ddd);
               if (str_to_llint(vect[n].c_str(), &iii))
                  intvect.emplace_back(iii);
            }

            if (intvect.size() == vect.size())
               res.SetField(parname, intvect);
            else if (dblvect.size() == vect.size())
               res.SetField(parname, dblvect);
            else
               res.SetField(parname, vect);

         } else if (parname == "tmout") {
            double tmout = 10;
            if (!str_to_double(parvalue.c_str(), &tmout)) tmout = 10;
            res.SetTimeout(tmout);
         } else {
            double ddd = 0.;
            long long iii = 0;
            if (str_to_llint(parvalue.c_str(), &iii))
               res.SetInt64(parname, iii);
            else if (str_to_double(parvalue.c_str(), &ddd))
               res.SetDouble(parname, ddd);
            else
               res.SetStr(parname, parvalue);
         }

      } while (!part.empty());
   }

   res.SetReceiver(producer_name);

   return res;
}


int dabc::Publisher::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("OwnCommand")) {

      std::string path = cmd.GetStr("Path");
      std::string worker = cmd.GetStr("Worker");
      bool ismgrpath = false;
      if (path.find("$MGR$") == 0) {
         ismgrpath = true;
         path.erase(0, 5);
         path = fMgrPath + path;
      } else
      if (path.find("$CONTEXT$") == 0) {
         path.erase(0, 9);
         path = dabc::format("/%s", dabc::mgr.GetName()) + path;
      }

      switch (cmd.GetInt("cmdid")) {
         case 1: { // REGISTER

            for (auto &entry : fPublishers) {
               if (entry.path == path) {
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

            DOUT3("PUBLISH folder %s", path.c_str());

            fPublishers.emplace_back(PublisherEntry());
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

            // ShootTimer("Timer");

            return cmd_true;
         }

         case 2:  { // UNREGISTER
            bool find = false;
            for (auto iter = fPublishers.begin(); iter != fPublishers.end(); iter++) {
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

            fSubscribers.emplace_back(SubscriberEntry());

            SubscriberEntry& entry = fSubscribers.back();

            entry.id = fCnt++;
            entry.path = path;
            entry.worker = worker;
            entry.local = islocal;
            entry.hlimit = 0;

            return cmd_true;
         }

         case 4: {   // UNSUBSCRIBE

            auto iter = fSubscribers.begin();
            while (iter != fSubscribers.end()) {
               if ((iter->worker == worker) && (iter->path == path))
                  fSubscribers.erase(iter++);
               else
                  iter++;
            }

            return cmd_true;
         }

         case 5: {  // REMOVE WORKER

            DOUT2("Publisher removes worker %s", worker.c_str());

            auto iter = fPublishers.begin();
            while (iter!=fPublishers.end()) {
               if (iter->worker == worker) {
                  DOUT2("Publisher removes path %s of worker %s", iter->path.c_str(), worker.c_str());
                  if (iter->local) fLocal.GetFolder(iter->path).Destroy();
                  fPublishers.erase(iter++);
               } else {
                  iter++;
               }
            }

            auto iter2 = fSubscribers.begin();
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

            for (auto &entry : fPublishers) {
               if ((entry.fulladdr == remoteaddr) && !entry.local) {
                  EOUT("Path %s already registered!!!", path.c_str());
                  return cmd_false;
               }
            }

            fPublishers.emplace_back(PublisherEntry());
            fPublishers.back().id = fCnt++;
            fPublishers.back().path = "";
            fPublishers.back().worker = worker;
            fPublishers.back().fulladdr = remoteaddr;
            fPublishers.back().hier = nullptr;
            fPublishers.back().local = false;
            fPublishers.back().rem.Create("remote");

            DOUT3("PUBLISH NODE %s", path.c_str());

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

      DOUT3("Get names list %s query %s", path.c_str(), cmd.GetStr("query").c_str());

      // if item was not found directly, try to ask producer if it can be extended
      if (h.null() || h.HasField(dabc::prop_more) || (cmd.GetStr("query").find("more")!=std::string::npos)) {
         if (!RedirectCommand(cmd, path)) return cmd_false;
         DOUT3("ITEM %s CAN PROVIDE MORE!!!", path.c_str());
         return cmd_postponed;
      }

      dabc::CmdGetNamesList::SetResNamesList(cmd, h);

      return cmd_true;
   } else
   if (cmd.IsName("CreateExeCmd")) {

      dabc::Command res = CreateExeCmd(cmd.GetStr("path"), cmd.GetStr("query"));
      if (res.null()) return cmd_false;

      cmd.SetRef("ExeCmd", res);

      return cmd_true;
   } else
   if (cmd.IsName("CmdUIKind")) {

      bool islocal = true;
      std::string item_name, request_name, uri = cmd.GetStr("uri");

      if (!uri.empty())
         if (!IdentifyItem(false, uri, islocal, item_name, request_name)) return cmd_false;

      dabc::Hierarchy h = GetWorkItem(item_name);

      if (islocal && h.Field(dabc::prop_kind).AsStr()=="DABC.HTML") {
         cmd.SetStr("ui_kind", "__user__");
         cmd.SetStr("path", h.GetField("_UserFilePath").AsStr());
         if (request_name.empty()) request_name = h.GetField("_UserFileMain").AsStr();
         cmd.SetStr("fname", request_name);
      } else {
         cmd.SetStr("path", item_name);
         cmd.SetStr("fname", request_name);

         //if (request_name.empty())
         //   cmd.SetStr("ui_kind", h.NumChilds() > 0 ? "__tree__" : "__single__");

         // publisher can only identify existing entries
         // all extra entries (like objects members in Go4 events browser) appears as subfolders in request
         size_t pos = request_name.rfind("/");
         if ((pos != std::string::npos) && (pos != request_name.length()-1)) {
            cmd.SetStr("path", item_name + request_name.substr(0, pos));
            cmd.SetStr("fname", request_name.substr(pos+1));
         }
      }

      return cmd_true;

   } else
   if (cmd.IsName("CmdNeedAuth")) {
      std::string path = cmd.GetStr("path");
      dabc::Hierarchy h = GetWorkItem(path);

      int res = -1;

      while (!h.null()) {
         if (h.HasField(dabc::prop_auth)) {
            res = h.GetField(dabc::prop_auth).AsBool() ? 1 : 0;
            break;
         }

         h = h.GetParentRef();
      }

      cmd.SetInt("need_auth", res);

      return cmd_true;
   }

   if (cmd.IsName(CmdGetBinary::CmdName())) {

      // if we get command here, we need to find destination for it

      std::string itemname = cmd.GetStr("Item");

      // this is executing of the command
      if (cmd.GetStr("Kind") == "execute") {
         std::string query = cmd.GetStr("Query");

         cmd.RemoveField("Item");
         cmd.RemoveField("Kind");
         cmd.RemoveField("Query");

         dabc::Command res = CreateExeCmd(itemname, query, cmd);

         if (res == cmd) {
            dabc::mgr.Submit(cmd);
            return cmd_postponed;
         } else {
            return cmd_false;
         }
      }

      // DOUT3("Publisher::CmdGetBinary for item %s", itemname.c_str());

      if (!RedirectCommand(cmd, itemname)) return cmd_false;

      return cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

// ===================================================================


bool dabc::PublisherRef::OwnCommand(int id, const std::string &path, const std::string &workername, void* hier)
{
   if (null()) return false;

/*   if (thread().null()) {
      DOUT2("Trying to submit publisher command when thread not assigned - ignore");
      return true;
   }
*/
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


std::string dabc::PublisherRef::UserInterfaceKind(const char *uri, std::string& path, std::string& fname)
{
   if (null()) return "__error__";

   dabc::Command cmd("CmdUIKind");
   cmd.SetStr("uri", uri);

   if (Execute(cmd) != cmd_true) return "__error__";

   path = cmd.GetStr("path");
   fname = cmd.GetStr("fname");
   return cmd.GetStr("ui_kind");
}

int dabc::PublisherRef::NeedAuth(const std::string &path)
{
   if (null()) return -1;

   dabc::Command cmd("CmdNeedAuth");
   cmd.SetStr("path", path);

   if (Execute(cmd) != cmd_true) return -1;

   return cmd.GetInt("need_auth");
}



dabc::Command dabc::PublisherRef::ExeCmd(const std::string &fullname, const std::string &query)
{
   dabc::Command res;
   if (null()) return res;

   dabc::Command cmd("CreateExeCmd");
   cmd.SetStr("path", fullname);
   cmd.SetStr("query", query);

   if (Execute(cmd) != cmd_true) return res;

   res = cmd.GetRef("ExeCmd");

   DOUT3("Produce command %p - now submit", res());

   if (res.null()) return res;

   dabc::mgr.Execute(res);
   return res;
}


dabc::Buffer dabc::PublisherRef::GetBinary(const std::string &fullname, const std::string &kind, const std::string &query, double tmout)
{
   if (null()) return nullptr;

   CmdGetBinary cmd(fullname, kind, query);
   cmd.SetTimeout(tmout);

   if (Execute(cmd) == cmd_true)
      return cmd.GetRawData();

   return nullptr;
}

dabc::Hierarchy dabc::PublisherRef::GetItem(const std::string &fullname, const std::string &query, double tmout)
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

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

#include "root/Monitor.h"

#include "TImage.h"

#include "dabc/Url.h"
#include "dabc/Iterator.h"
#include "dabc/Publisher.h"

#include "THttpServer.h"
#include "TRootSniffer.h"
#include "TRootSnifferStore.h"
#include "RVersion.h"

class TRootSnifferStoreDabc : public TRootSnifferStore {
public:
   dabc::Hierarchy& top;
   std::string prefix;
   dabc::Hierarchy curr;

   TRootSnifferStoreDabc(dabc::Hierarchy& _top, const std::string &_prefix) :
      TRootSnifferStore(),
      top(_top),
      prefix(_prefix.c_str()),
      curr() {}

   virtual ~TRootSnifferStoreDabc() {}

   void CreateNode(Int_t lvl, const char *nodename) override
   {
      (void) lvl;
      if (curr.null()) {
         top.Create(nodename);
         curr = top;
      } else {
         curr = curr.CreateHChild(nodename, true);
      }
   }
   void SetField(Int_t lvl, const char *field, const char *value, Bool_t with_quotes) override
   {
      (void) lvl;
      if (!with_quotes) {
         if (!strcmp("value", "true")) { curr.SetField(field, true); return; }
         if (!strcmp("value", "false")) { curr.SetField(field, false); return; }
         if ((strlen(value)>2) && (*value=='[')) {
            dabc::RecordField f(value);
            if (f.AsIntVect().size()>0) {
               curr.SetField(field,f.AsIntVect());
               return;
            }
         }
      }

      if ((strcmp(field,"_autoload") == 0) && (*value == '/')) {
         TString res = "/";
         res.Append(prefix);
         res.Append(value);
         res.ReplaceAll(";/", std::string("/") + prefix);
         curr.SetField(field, res.Data());
      } else
      if ((strcmp(field,"_icon") == 0) && (*value == '/'))
         curr.SetField(field, std::string("/") + prefix + value);
      else
         curr.SetField(field, value);
   }

   void BeforeNextChild(Int_t, Int_t, Int_t) override
   {
   }

   void CloseNode(Int_t, Int_t) override
   {
      curr = curr.GetParentRef();
   }
};


// ==============================================================================

root::Monitor::Monitor(const std::string &name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fRoot(),
   fHierarchy(),
   fRootCmds(dabc::CommandsQueue::kindPostponed),
   fLastUpdate()
{
   fEnabled = Cfg("enabled", cmd).AsBool(false);
   if (!fEnabled) return;
   fPrefix = Cfg("prefix", cmd).AsStr("ROOT");
}

root::Monitor::~Monitor()
{
}


void root::Monitor::OnThreadAssigned()
{
   dabc::Worker::OnThreadAssigned();

   if (!IsEnabled()) {
      EOUT("sniffer was not enabled - why it is started??");
      return;
   }

   fHierarchy.Create("ROOT", true);
   InitializeHierarchy();

   Publish(fHierarchy, fPrefix);

   // if timer not installed, emulate activity in ROOT by regular timeouts
}

double root::Monitor::ProcessTimeout(double)
{
   return 10.;
}


int root::Monitor::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetBinary::CmdName()) ||
       cmd.IsName(dabc::CmdGetNamesList::CmdName()) ||
       cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      fRootCmds.Push(cmd);
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


void root::Monitor::RescanHierarchy(TRootSniffer* sniff, dabc::Hierarchy& main, const char *path)
{
   main.Release();

   TRootSnifferStoreDabc store(main, fPrefix);

   sniff->ScanHierarchy("DABC", path, &store);
}


int root::Monitor::ProcessGetBinary(THttpServer* serv, TRootSniffer* sniff, dabc::Command cmd)
{
   // command executed in ROOT context without locked mutex,
   // one can use as much ROOT as we want

   std::string itemname = cmd.GetStr("subitem");
   std::string binkind = cmd.GetStr("Kind");
   std::string query = cmd.GetStr("Query");

   dabc::Buffer buf;

   TString realfilename;
   std::string filename = std::string("/") + itemname + std::string("/") + binkind;

   if (serv->IsFileRequested(filename.c_str(), realfilename)) {
      DOUT0("Item %s Send file %s ", filename.c_str(), realfilename.Data());

      Int_t len = 0;
      char *ptr = THttpServer::ReadFileContent(realfilename.Data(), len);
      buf = dabc::Buffer::CreateBuffer(ptr, (unsigned) len, true);
      cmd.SetRawData(buf);
      return dabc::cmd_true;
   }

   // for root.bin request verify that object must be really requested - it may be not changed at all
   if (binkind == "root.bin") {

      // check if version was specified in query
      uint64_t version = 0;
      dabc::Url url(std::string("getbin?") + query);
      if (url.IsValid() && url.HasOption("version"))
         version = (unsigned) url.GetOptionInt("version", 0);

      ULong_t objhash = sniff->GetItemHash(itemname.c_str());

      bool binchanged = true;
      if (version > 0) {
         dabc::LockGuard lock(fHierarchy.GetHMutex());
         binchanged = fHierarchy.IsBinItemChanged(itemname, objhash, version);
      }

      if (!binchanged) {
         buf = dabc::Buffer::CreateBuffer(8); // dummy data of 8 bytes long
         {
            dabc::LockGuard lock(fHierarchy.GetHMutex());
            // here correct version number for item and master item will be filled
            fHierarchy.FillBinHeader(itemname, cmd);
         }

         cmd.SetRawData(buf);
         return dabc::cmd_true;
      }
   }

#if ROOT_VERSION_CODE >= ROOT_VERSION(6,13,0)

   std::string str;

   if (!sniff->Produce(itemname, binkind, query, str)) {
       EOUT("ROOT sniffer producer fails for item %s kind %s", itemname.c_str(), binkind.c_str());
       return dabc::cmd_false;
   }

   buf = dabc::Buffer::CreateBuffer(str.data(), str.length(), false, true);

#else

   void* ptr = nullptr;
   Long_t length = 0;
   TString str;

   // use sniffer method to generate data

   if (!sniff->Produce(itemname.c_str(), binkind.c_str(), query.c_str(), ptr, length, str)) {
       EOUT("ROOT sniffer producer fails for item %s kind %s", itemname.c_str(), binkind.c_str());
       return dabc::cmd_false;
   }

   if (ptr)
      buf = dabc::Buffer::CreateBuffer(ptr, (unsigned) length, true);
   else
      buf = dabc::Buffer::CreateBuffer(str.Data(), str.Length(), false, true);

#endif

   // for binary data set correct version into header
   if (binkind == "root.bin") {

      ULong_t mhash = sniff->GetStreamerInfoHash();

      dabc::LockGuard lock(fHierarchy.GetHMutex());
      // here correct version number for item and master item will be filled
      fHierarchy.FillBinHeader(itemname, cmd, mhash);
   }

   cmd.SetRawData(buf);

   return dabc::cmd_true;
}


void root::Monitor::ProcessActionsInRootContext(THttpServer* serv, TRootSniffer* sniff)
{
   // DOUT0("ROOOOOOOT sniffer ProcessActionsInRootContext %p %s active %s", this, ClassName(), DBOOL(fWorkerActive));

   if (fLastUpdate.null() || fLastUpdate.Expired(3.)) {
      DOUT3("Update ROOT structures");
      RescanHierarchy(sniff, fRoot);
      fLastUpdate.GetNow();

      // we lock mutex only at the moment when synchronize hierarchy with main
      dabc::LockGuard lock(fHierarchy.GetHMutex());

      fHierarchy.Update(fRoot);
   }

   bool doagain = true;
   dabc::Command cmd;

   while (doagain) {
      if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {
         cmd.Reply(ProcessGetBinary(serv, sniff, cmd));
      } else
      if (cmd.IsName(dabc::CmdGetNamesList::CmdName())) {
         std::string item = cmd.GetStr("subitem");

         dabc::Hierarchy res;

         if (item.empty()) {
            if (fLastUpdate.null() || fLastUpdate.Expired(3.)) {
               RescanHierarchy(sniff, fRoot);
               fLastUpdate.GetNow();
               dabc::LockGuard lock(fHierarchy.GetHMutex());
               fHierarchy.Update(fRoot);
            }

            res = fRoot;
         } else {
            RescanHierarchy(sniff, res, item.c_str());
         }

         DOUT3("Extend result %s", DBOOL(!res.null()));

         if (res.null()) cmd.ReplyFalse();

         dabc::CmdGetNamesList::SetResNamesList(cmd, res);

         cmd.ReplyTrue();
      } else
      if (cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {
         if (ProcessHCommand(cmd.GetStr("Item"), cmd)) cmd.ReplyTrue();
      }

      if (!cmd.null()) {
         EOUT("Not processed command %s", cmd.GetName());
         cmd.ReplyFalse();
      }

      dabc::LockGuard lock(fHierarchy.GetHMutex());

      if (fRootCmds.Size()>0) cmd = fRootCmds.Pop();
      doagain = !cmd.null();
   }
}


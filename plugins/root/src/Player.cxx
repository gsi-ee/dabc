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

#include "root/Player.h"

#include "TImage.h"
#include "TROOT.h"
#include "TTimer.h"
#include "TClass.h"

#include "dabc/Url.h"
#include "dabc/Iterator.h"
#include "dabc/Publisher.h"

#include "TRootSniffer.h"
#include "TRootSnifferStore.h"


class TDabcTimer : public TTimer {
   public:

      root::Player* fSniffer;

      TDabcTimer(Long_t milliSec, Bool_t mode) : TTimer(milliSec, mode), fSniffer(0) {}

      virtual ~TDabcTimer() {
         if (fSniffer) fSniffer->fTimer = 0;
      }

      void SetSniffer(root::Player* sniff) { fSniffer = sniff; }

      virtual void Timeout() {
         if (fSniffer) fSniffer->ProcessActionsInRootContext();
      }

};

class TRootSnifferStoreDabc : public TRootSnifferStore {
public:
   dabc::Hierarchy& top;
   dabc::Hierarchy curr;

   TRootSnifferStoreDabc(dabc::Hierarchy& _top) :
      TRootSnifferStore(),
      top(_top),
      curr() {}

   virtual ~TRootSnifferStoreDabc() {}

   virtual void CreateNode(Int_t lvl, const char* nodename)
   {
      if (curr.null()) {
         top.Create(nodename);
         curr = top;
      } else {
         curr = curr.CreateChild(nodename);
      }
      //DOUT0("Create node %s", nodename);
   }
   virtual void SetField(Int_t lvl, const char* field, const char* value, Int_t nfld)
   {
      curr.SetField(field, value);
      //DOUT0("SetField curr:%s %s = %s", curr.GetName(), field, value);
   }
   virtual void BeforeNextChild(Int_t lvl, Int_t nchld, Int_t nfld)
   {
   }

   virtual void CloseNode(Int_t lvl, const char* nodename, Int_t numchilds)
   {
      curr = curr.GetParentRef();
      //DOUT0("Close node %s", nodename ? nodename : "---");
   }
};




// ==============================================================================

root::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fBatch(true),
   fSyncTimer(true),
   fCompression(5),
   fNewSniffer(0),
   fTimer(0),
   fRoot(),
   fHierarchy(),
   fRootCmds(dabc::CommandsQueue::kindPostponed),
   fLastUpdate(),
   fStartThrdId(0)
{
   fEnabled = Cfg("enabled", cmd).AsBool(false);
   if (!fEnabled) return;

   fBatch = Cfg("batch", cmd).AsBool(true);
   fSyncTimer = Cfg("synctimer", cmd).AsBool(true);
   fCompression = Cfg("compress", cmd).AsInt(5);
   fPrefix = Cfg("prefix", cmd).AsStr("ROOT");

   if (fBatch) gROOT->SetBatch(kTRUE);

   fNewSniffer = new TRootSniffer("DabcRoot","dabc");
   fNewSniffer->SetCompression(fCompression);
}

root::Player::~Player()
{
   if (fTimer) fTimer->fSniffer = 0;

   if (fNewSniffer) { delete fNewSniffer; fNewSniffer = 0; }
}

void root::Player::SetObjectSniffer(TRootSniffer* sniff)
{
   if (fNewSniffer) delete fNewSniffer;
   fNewSniffer = sniff;
}


void root::Player::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("sniffer was not enabled - why it is started??");
      return;
   }

   fHierarchy.Create("ROOT", true);
   // identify ourself as bin objects producer
   // fHierarchy.Field(dabc::prop_producer).SetStr(WorkerAddress());
   InitializeHierarchy();

   if (fTimer==0) ActivateTimeout(0);

   Publish(fHierarchy, fPrefix);

   // if timer not installed, emulate activity in ROOT by regular timeouts
}

double root::Player::ProcessTimeout(double last_diff)
{
   // this is just historical code, normally ROOT hierarchy will be scanned per ROOT timer

   dabc::Hierarchy h;

   RescanHierarchy(h);

   DOUT2("ROOT %p hierarchy = \n%s", gROOT, h.SaveToXml().c_str());

   dabc::LockGuard lock(fHierarchy.GetHMutex());

   fHierarchy.Update(h);

   //h.Destroy();

   return 10.;
}


int root::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetBinary::CmdName()) ||
       cmd.IsName(dabc::CmdGetNamesList::CmdName())) {
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      fRootCmds.Push(cmd);
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


void root::Player::RescanHierarchy(dabc::Hierarchy& main, const char* path)
{
   main.Release();

   TRootSnifferStoreDabc store(main);

   fNewSniffer->ScanHierarchy(path, &store);
}

void root::Player::InstallSniffTimer()
{
   if (fTimer==0) {
      fTimer = new TDabcTimer(100, fSyncTimer);
      fTimer->SetSniffer(this);
      fTimer->TurnOn();

      fStartThrdId = dabc::PosixThread::Self();
   }
}

int root::Player::ProcessGetBinary(dabc::Command cmd)
{
   // command executed in ROOT context without locked mutex,
   // one can use as much ROOT as we want

   std::string itemname = cmd.GetStr("subitem");
   std::string binkind = cmd.GetStr("Kind");
   std::string query = cmd.GetStr("Query");

   dabc::Buffer buf;

   if (binkind == "png") {
      void* ptr(0);
      Long_t length(0);

      if (!fNewSniffer->ProduceImage(TImage::kPng, itemname.c_str(), query.c_str(), ptr, length)) {
         EOUT("Image producer fails for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      buf = dabc::Buffer::CreateBuffer(ptr, (unsigned) length, true);
   } else {

      TString objhash;
      bool binchanged = true;

      if (itemname!="StreamerInfo") {
         TObject* obj = fNewSniffer->FindTObjectInHierarchy(itemname.c_str());

         if (obj==0) {
            EOUT("Object for item %s not found", itemname.c_str());
            return dabc::cmd_false;
         }

         DOUT2("OBJECT FOUND %s %p", itemname.c_str(), obj);

         objhash = fNewSniffer->GetObjectHash(obj);

         uint64_t version = 0;
         dabc::Url url(std::string("getbin?") + query);
         if (url.IsValid() && url.HasOption("version"))
            version = (unsigned) url.GetOptionInt("version", 0);

         dabc::LockGuard lock(fHierarchy.GetHMutex());
         binchanged = fHierarchy.IsBinItemChanged(itemname, objhash.Data(), version);
      }

      if (binchanged) {

         void* ptr(0);
         Long_t length(0);

         if (!fNewSniffer->ProduceBinary(itemname.c_str(), query.c_str(), ptr, length)) {
            EOUT("Binary producer fails for item %s", itemname.c_str());
            return dabc::cmd_false;
         }

         buf = dabc::Buffer::CreateBuffer(ptr, (unsigned) length, true);
      } else {
         buf = dabc::Buffer::CreateBuffer(sizeof(dabc::BinDataHeader)); // only header is required
         dabc::BinDataHeader* hdr = (dabc::BinDataHeader*) buf.SegmentPtr();
         hdr->reset();
      }

      TString mhash = fNewSniffer->GetStreamerInfoHash();

      {
         dabc::LockGuard lock(fHierarchy.GetHMutex());

         // here correct version number for item and master item will be filled
         fHierarchy.FillBinHeader(itemname, buf, mhash.Data(), "StreamerInfo");
      }
   }

   cmd.SetRawData(buf);

   return dabc::cmd_true;
}


void root::Player::ProcessActionsInRootContext()
{
   // DOUT0("ROOOOOOOT sniffer ProcessActionsInRootContext %p %s active %s", this, ClassName(), DBOOL(fWorkerActive));

   if ((fStartThrdId!=0) && ( fStartThrdId != dabc::PosixThread::Self())) {
      EOUT("Called from other thread as when timer was started");
   }

   if (fLastUpdate.null() || fLastUpdate.Expired(3.)) {
      DOUT3("Update ROOT structures");
      RescanHierarchy(fRoot);
      fLastUpdate.GetNow();

      // we lock mutex only at the moment when synchronize hierarchy with main
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      fHierarchy.Update(fRoot);

      // DOUT0("Main ROOT hierarchy %p has producer %s", fHierarchy(), DBOOL(fHierarchy.HasField(dabc::prop_producer)));

      DOUT5("ROOT hierarchy ver %u \n%s", fHierarchy.GetVersion(), fHierarchy.SaveToXml().c_str());
   }

   bool doagain(true);
   dabc::Command cmd;

   while (doagain) {

      if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {
         cmd.Reply(ProcessGetBinary(cmd));
      } else
      if (cmd.IsName(dabc::CmdGetNamesList::CmdName())) {
         std::string item = cmd.GetStr("subitem");

         DOUT3("Request extended ROOT hierarchy %s", item.c_str());

         dabc::Hierarchy res;

         if (item.empty()) {
            if (fLastUpdate.null() || fLastUpdate.Expired(3.)) {
               RescanHierarchy(fRoot);
               fLastUpdate.GetNow();
               dabc::LockGuard lock(fHierarchy.GetHMutex());
               fHierarchy.Update(fRoot);
            }

            res = fRoot;
         } else {
            RescanHierarchy(res, item.c_str());
         }

         if (res.null()) cmd.ReplyFalse();

         dabc::CmdGetNamesList::SetResNamesList(cmd, res);

         cmd.ReplyTrue();
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


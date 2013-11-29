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

#include "dabc_root/RootSniffer.h"

#include <math.h>
#include <stdlib.h>

#include "TH1.h"
#include "TGraph.h"
#include "TProfile.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TList.h"
#include "TMemFile.h"
#include "TStreamerInfo.h"
#include "TBufferFile.h"
#include "TROOT.h"
#include "TTimer.h"
#include "TFolder.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"

#include "dabc_root/BinaryProducer.h"

#include "dabc/Iterator.h"
#include "dabc/Publisher.h"

class TDabcTimer : public TTimer {
   public:

      dabc_root::RootSniffer* fSniffer;

      TDabcTimer(Long_t milliSec, Bool_t mode) : TTimer(milliSec, mode), fSniffer(0) {}

      virtual ~TDabcTimer() {
         if (fSniffer) fSniffer->fTimer = 0;
      }

      void SetSniffer(dabc_root::RootSniffer* sniff) { fSniffer = sniff; }

      virtual void Timeout() {
         if (fSniffer) fSniffer->ProcessActionsInRootContext();
      }

};


// ==============================================================================

dabc_root::RootSniffer::RootSniffer(const std::string& name, dabc::Command cmd) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fBatch(true),
   fSyncTimer(true),
   fCompression(5),
   fProducer(0),
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
}

dabc_root::RootSniffer::~RootSniffer()
{
   if (fTimer) fTimer->fSniffer = 0;

   if (fProducer) { delete fProducer; fProducer = 0; }

   //fHierarchy.Destroy();
   //fRoot.Destroy();
}

void dabc_root::RootSniffer::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("sniffer was not enabled - why it is started??");
      return;
   }

   fProducer = new dabc_root::BinaryProducer("producer", fCompression);

   fHierarchy.Create("ROOT", true);
   // identify ourself as bin objects producer
   // fHierarchy.Field(dabc::prop_producer).SetStr(WorkerAddress());
   InitializeHierarchy();

   if (fTimer==0) ActivateTimeout(0);

   Publish(fHierarchy, fPrefix);

   // if timer not installed, emulate activity in ROOT by regular timeouts
}

double dabc_root::RootSniffer::ProcessTimeout(double last_diff)
{
   // this is just historical code, normally ROOT hierarchy will be scanned per ROOT timer

   dabc::Hierarchy h;
   h.Create("ROOT");
   // this is fake element, which is need to be requested before first
   h.CreateChild("StreamerInfo").Field(dabc::prop_kind).SetStr("ROOT.TList");

   ScanRootHierarchy(h);

   DOUT2("ROOT %p hierarchy = \n%s", gROOT, h.SaveToXml().c_str());

   dabc::LockGuard lock(fHierarchy.GetHMutex());

   fHierarchy.Update(h);

   //h.Destroy();

   return 10.;
}


int dabc_root::RootSniffer::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      fRootCmds.Push(cmd);
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


void* dabc_root::RootSniffer::AddObjectToHierarchy(dabc::Hierarchy& parent, const char* searchpath, TObject* obj, int lvl)
{
   if (obj==0) return 0;

   void *res = 0;

   dabc::Hierarchy chld;

   if (searchpath==0) {

      std::string itemname = obj->GetName();

      size_t pos = (itemname.find_last_of("/"));
      if (pos!=std::string::npos) itemname = itemname.substr(pos+1);
      if (itemname.empty()) itemname = "item";
      while ((pos = itemname.find_first_of("/#<>:")) != std::string::npos)
         itemname.replace(pos, 1, "_");

      int cnt = 0;
      std::string basename = itemname;

      // prevent that same item appears many time
      while (!parent.FindChild(itemname.c_str()).null()) {
         itemname = basename + dabc::format("%d", cnt++);
      }

      chld = parent.CreateChild(itemname);

      if (itemname.compare(obj->GetName()) != 0)
         chld.Field(dabc::prop_realname).SetStr(obj->GetName());

      if (IsDrawableClass(obj->IsA())) {
         chld.Field(dabc::prop_kind).SetStr(dabc::format("ROOT.%s", obj->ClassName()));

         std::string master;
         for (int n=0;n<lvl;n++) master +="../";

         chld.Field(dabc::prop_masteritem).SetStr(master+"StreamerInfo");

//         if (obj->InheritsFrom(TH1::Class())) {
//            chld.Field(dabc::prop_hash).SetDouble(((TH1*)obj)->GetEntries());
//         }
      } else
      if (IsBrowsableClass(obj->IsA())) {
         chld.Field(dabc::prop_kind).SetStr(dabc::format("ROOT.%s", obj->ClassName()));
      }
   } else {
      chld = parent;
   }

   if (obj->InheritsFrom(TFolder::Class())) {
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TFolder*) obj)->GetListOfFolders(), lvl+1);
   } else
   if (obj->InheritsFrom(TDirectory::Class())) {
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TDirectory*) obj)->GetList(), lvl+1);
   } else
   if (obj->InheritsFrom(TTree::Class())) {
      // if (!res) res = ScanListHierarchy(chld, searchpath, ((TTree*) obj)->GetListOfBranches(), lvl+1);
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TTree*) obj)->GetListOfLeaves(), lvl+1);
   } else
   if (obj->InheritsFrom(TBranch::Class())) {
      // if (!res) res = ScanListHierarchy(chld, searchpath, ((TBranch*) obj)->GetListOfBranches(), lvl+1);
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TBranch*) obj)->GetListOfLeaves(), lvl+1);
   }
   return res;
}


void* dabc_root::RootSniffer::ScanListHierarchy(dabc::Hierarchy& parent, const char* searchpath, TCollection* lst, int lvl, const std::string& foldername)
{
   if ((lst==0) || (lst->GetSize()==0)) return 0;

   void* res = 0;

   dabc::Hierarchy top = parent;

   if (!foldername.empty()) {

      if (searchpath) {

         // DOUT0("SEARCH folder %s in path %s", foldername.c_str(), searchpath);

         if (strncmp(searchpath, foldername.c_str(), foldername.length())!=0) return 0;
         searchpath+=foldername.length();
         if (*searchpath!='/') return 0;
         searchpath++;
         top = parent.FindChild(foldername.c_str());
         if (top.null()) return 0;

         // DOUT0("FOUND folder %s ", foldername.c_str());


      } else {
         top = parent.CreateChild(foldername);
      }
      lvl++;
   }

   TIter iter(lst);
   TObject* obj(0);

   if (searchpath==0) {
      while ((obj = iter())!=0)
         AddObjectToHierarchy(top, searchpath, obj, lvl);
   } else {
      // while item name can be changed, we find first item itself and than
      // try to find object name

      const char* separ = strchr(searchpath, '/');
      std::string itemname = searchpath;
      if (separ!=0) {
         itemname.resize(separ - searchpath);
      }

      // DOUT0("SEARCH item %s in path %s", itemname.c_str(), searchpath);

      dabc::Hierarchy chld = top.FindChild(itemname.c_str());
      if (chld.null()) return 0;

      // DOUT0("FOUND item %s ", itemname.c_str());

      if (chld.HasField(dabc::prop_realname)) itemname = chld.Field(dabc::prop_realname).AsStr();

      // DOUT0("SEARCH OBJECT with name %s ", itemname.c_str());

      while ((obj = iter())!=0) {
         if (obj->GetName() && (itemname == obj->GetName())) {
            if (separ==0) return obj;
            res = AddObjectToHierarchy(chld, separ+1, obj, lvl);
            if (!res) return res;
         }
      }
   }

   return res;
}

void* dabc_root::RootSniffer::ScanRootHierarchy(dabc::Hierarchy& h, const char* searchpath)
{
   void *res = 0;

   if (h.null()) return res;

//   if (searchpath) DOUT0("ROOT START SEARCH %s ", searchpath);
//              else DOUT0("ROOT START SCAN");

//   if (topf!=0)
//      return ScanListHierarchy(h, searchpath, topf->GetListOfFolders(), 0);

   TFolder* topf = dynamic_cast<TFolder*> (gROOT->FindObject("//root/dabc"));

   if (searchpath==0) h.Field(dabc::prop_kind).SetStr("ROOT.Session");

   if (!res) res = ScanListHierarchy(h, searchpath, gROOT->GetList(), 0);

   if (!res) res = ScanListHierarchy(h, searchpath, gROOT->GetListOfCanvases(), 0, "Canvases");

   if (!res) res = ScanListHierarchy(h, searchpath, gROOT->GetListOfFiles(), 0, "Files");

   if (!res && topf) res = ScanListHierarchy(h, searchpath, topf->GetListOfFolders(), 0, "Objects");

   return res;
}


bool dabc_root::RootSniffer::IsDrawableClass(TClass* cl)
{
   if (cl==0) return false;
   if (cl->InheritsFrom(TH1::Class())) return true;
   if (cl->InheritsFrom(TGraph::Class())) return true;
   if (cl->InheritsFrom(TCanvas::Class())) return true;
   if (cl->InheritsFrom(TProfile::Class())) return true;
   return false;
}



bool dabc_root::RootSniffer::IsBrowsableClass(TClass* cl)
{
   if (cl==0) return false;

   if (cl->InheritsFrom(TTree::Class())) return true;
   if (cl->InheritsFrom(TBranch::Class())) return true;
   if (cl->InheritsFrom(TLeaf::Class())) return true;
   if (cl->InheritsFrom(TFolder::Class())) return true;

   return false;
}


void dabc_root::RootSniffer::InstallSniffTimer()
{
   if (fTimer==0) {
      fTimer = new TDabcTimer(100, fSyncTimer);
      fTimer->SetSniffer(this);
      fTimer->TurnOn();

      fStartThrdId = dabc::PosixThread::Self();
   }
}

int dabc_root::RootSniffer::ProcessGetBinary(dabc::Command cmd)
{
   // command executed in ROOT context without locked mutex,
   // one can use as much ROOT as we want

   std::string itemname = cmd.GetStr("subitem");
   uint64_t version = cmd.GetUInt("version");

   dabc::Buffer buf;

   std::string objhash;

   if (itemname=="StreamerInfo") {
      buf = fProducer->GetStreamerInfoBinary();
   } else {

      TObject* obj = (TObject*) ScanRootHierarchy(fRoot, itemname.c_str());

      if (obj==0) {
         EOUT("Object for item %s not found", itemname.c_str());
         return dabc::cmd_false;
      }

      objhash = fProducer->GetObjectHash(obj);

      bool binchanged = false;

      {
         dabc::LockGuard lock(fHierarchy.GetHMutex());
         binchanged = fHierarchy.IsBinItemChanged(itemname, objhash, version);
      }

      if (binchanged) {
         buf = fProducer->GetBinary(obj, cmd.GetBool("image", false));
      } else {
         buf = dabc::Buffer::CreateBuffer(sizeof(dabc::BinDataHeader)); // only header is required
         dabc::BinDataHeader* hdr = (dabc::BinDataHeader*) buf.SegmentPtr();
         hdr->reset();
      }
   }

   std::string mhash = fProducer->GetStreamerInfoHash();

   {
      dabc::LockGuard lock(fHierarchy.GetHMutex());

      // here correct version number for item and master item will be filled
      fHierarchy.FillBinHeader(itemname, buf, mhash);
   }

   cmd.SetRawData(buf);

   return dabc::cmd_true;
}


void dabc_root::RootSniffer::ProcessActionsInRootContext()
{
   // DOUT0("ROOOOOOOT sniffer ProcessActionsInRootContext %p %s active %s", this, ClassName(), DBOOL(fWorkerActive));

   if ((fStartThrdId!=0) && ( fStartThrdId != dabc::PosixThread::Self())) {
      EOUT("Called from other thread as when timer was started");
   }

   if (fLastUpdate.null() || fLastUpdate.Expired(3.)) {
      DOUT3("Update ROOT structures");
      fRoot.Release();
      fRoot.Create("ROOT");
      // fRoot.Field(dabc::prop_producer).SetStr(WorkerAddress());

      // this is fake element, which is need to be requested before first
      dabc::Hierarchy si = fRoot.CreateChild("StreamerInfo");
      si.Field(dabc::prop_kind).SetStr("ROOT.TList");
      si.Field(dabc::prop_hash).SetStr(fProducer->GetStreamerInfoHash());

      ScanRootHierarchy(fRoot);

      fLastUpdate.GetNow();

      // we lock mutex only at the moment when synchronize hierarchy with main
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      fHierarchy.Update(fRoot);

      // DOUT0("Main ROOT hierarchy %p has producer %s", fHierarchy(), DBOOL(fHierarchy.HasField(dabc::prop_producer)));

      // DOUT0("ROOT hierarchy ver %u \n%s", fHierarchy.GetVersion(), fHierarchy.SaveToXml().c_str());
   }

   bool doagain(true);
   dabc::Command cmd;

   while (doagain) {

      if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {
         cmd.Reply(ProcessGetBinary(cmd));
      } else
      if (!cmd.null()) {
         EOUT("Not processed command %s", cmd.GetName());
         cmd.ReplyFalse();
      }

      dabc::LockGuard lock(fHierarchy.GetHMutex());

      if (fRootCmds.Size()>0) cmd = fRootCmds.Pop();
      doagain = !cmd.null();
   }
}

bool dabc_root::RootSniffer::RegisterObject(const char* subfolder, TObject* obj)
{
   if (obj==0) return false;

   TFolder* topf = gROOT->GetRootFolder();

   if (topf==0) {
      EOUT("Not found top ROOT folder!!!");
      return false;
   }

   TFolder* dabcfold = dynamic_cast<TFolder*> (topf->FindObject("dabc"));
   if (dabcfold==0) {
      dabcfold = topf->AddFolder("dabc", "Top DABC folder");
      dabcfold->SetOwner(kFALSE);
   }

   if ((subfolder!=0) && (strlen(subfolder)>0)) {

      TObjArray* arr = TString(subfolder).Tokenize("/");
      for (Int_t i=0; i <= (arr ? arr->GetLast() : -1); i++) {

         const char* subname = arr->At(i)->GetName();
         if (strlen(subname)==0) continue;

         TFolder* fold = dynamic_cast<TFolder*> (dabcfold->FindObject(subname));
         if (fold==0) {
            fold = dabcfold->AddFolder(subname, "DABC sub-folder");
            fold->SetOwner(kFALSE);
         }
         dabcfold = fold;
      }

   }

   // If object will be destroyed, it must be removed from the folders automatically
   obj->SetBit(kMustCleanup);

   dabcfold->Add(obj);

   // register folder for cleanup
   if (!gROOT->GetListOfCleanups()->FindObject(dabcfold))
      gROOT->GetListOfCleanups()->Add(dabcfold);

   return true;
}

bool dabc_root::RootSniffer::UnregisterObject(TObject* obj)
{
   if (obj==0) return true;

   TFolder* topf = gROOT->GetRootFolder();

   if (topf==0) {
      EOUT("Not found top ROOT folder!!!");
      return false;
   }

   TFolder* dabcfold = dynamic_cast<TFolder*> (topf->FindObject("dabc"));

   if (dabcfold) dabcfold->RecursiveRemove(obj);

   return true;
}


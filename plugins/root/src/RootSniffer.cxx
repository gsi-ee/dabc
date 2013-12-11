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
#include "TClass.h"
#include "TDataMember.h"
#include "TDataType.h"
#include "TBaseClass.h"


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

   RescanHierarchy(h);

   DOUT2("ROOT %p hierarchy = \n%s", gROOT, h.SaveToXml().c_str());

   dabc::LockGuard lock(fHierarchy.GetHMutex());

   fHierarchy.Update(h);

   //h.Destroy();

   return 10.;
}


int dabc_root::RootSniffer::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetBinary::CmdName()) ||
       cmd.IsName("GetGlobalNamesList")) {
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      fRootCmds.Push(cmd);
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

void dabc_root::RootSniffer::ScanObjectMemebers(ScanRec& rec, TClass* cl, char* ptr, unsigned long int cloffset)
{
   if ((cl==0) || (ptr==0)) return;

   //DOUT0("SCAN CLASS %s mask %u", cl->GetName(), rec.mask);

   ScanRec chld;
   if (!chld.MakeChild(rec)) return;

   // first of all expand base classes
   TIter cliter(cl->GetListOfBases());
   TObject* obj = 0;
   while (((obj=cliter()) != 0) && !chld.Done()) {
      TBaseClass* baseclass = dynamic_cast<TBaseClass*>(obj);
      if (baseclass==0) continue;
      TClass* bclass = baseclass->GetClassPointer();
      if (bclass==0) continue;

      if (chld.TestObject(baseclass))
          ScanObjectMemebers(chld, bclass, ptr, cloffset + baseclass->GetDelta());
   }

   //DOUT0("SCAN MEMBERS %s %u mask %u done %s", cl->GetName(), cl->GetListOfDataMembers()->GetSize(), chld.mask, DBOOL(chld.Done()));

   // than expand data members
   TIter iter(cl->GetListOfDataMembers());
   while (((obj=iter()) != 0) && !chld.Done()) {
      TDataMember* member = dynamic_cast<TDataMember*>(obj);
      if (member==0) continue;

      //printf("MEMBER %s mask %u\n", member->GetName(), chld.mask);

      char* member_ptr = ptr + cloffset + member->GetOffset();

      if (chld.TestObject(member)) {
         // set more property
         bool islist = strcmp(member->GetTypeName(),"TList")==0;
         if (islist) chld.SetField(dabc::prop_more, "true");

         //printf("LOCATE MEMBER %s mask %u objname %s canexpand %s path %s\n", member->GetName(), chld.mask, chld.objname.c_str(), DBOOL(chld.CanExpandItem()), chld.searchpath ? chld.searchpath : "-null-");

         if (chld.CanExpandItem()) {
            if (islist) {

               //printf("!!! EXPAND LIST %s mask %u \n", member->GetName(), chld.mask);

               chld.SetField("#members", "true");
               if (member->IsaPointer()) member_ptr = *((char**) member_ptr);
               //printf("SCAN LIST %s\n", member->GetName());
               ScanList(chld, (TList*) member_ptr);
            }
         }
      }
   }
}


void dabc_root::RootSniffer::ScanObject(ScanRec& rec, TObject* obj)
{
   if (obj==0) return;

   //DOUT0("SCAN OBJECT %s can expand %s mask %u", obj->GetName(), DBOOL(rec.CanExpandItem()), rec.mask);

   if (rec.SetResult(obj)) return;

   if (IsDrawableClass(obj->IsA())) {
      rec.SetField(dabc::prop_kind, dabc::format("ROOT.%s", obj->ClassName()));

      std::string master;
      for (int n=1;n<rec.lvl;n++) master.append("../");
      rec.SetField(dabc::prop_masteritem, master+"StreamerInfo");

   } else
   if (IsBrowsableClass(obj->IsA())) {
      rec.SetField(dabc::prop_kind, dabc::format("ROOT.%s", obj->ClassName()));
   }

   if (obj->InheritsFrom(TFolder::Class())) {
      // starting from special folder, we automatically scan members

      TFolder* fold = ((TFolder*) obj);
      if (fold->TestBit(BIT(19)) && (rec.mask & mask_Scan))
         rec.mask = rec.mask | mask_ChldMemb;
      ScanList(rec, fold->GetListOfFolders());
   } else
   if (obj->InheritsFrom(TDirectory::Class())) {
      ScanList(rec, ((TDirectory*) obj)->GetList());
   } else
   if (obj->InheritsFrom(TTree::Class())) {
      // if (!res) res = ScanList(mask, chld, searchpath, ((TTree*) obj)->GetListOfBranches(), lvl+1);
      ScanList(rec, ((TTree*) obj)->GetListOfLeaves());
   } else
   if (obj->InheritsFrom(TBranch::Class())) {
      // if (!res) res = ScanList(mask, chld, searchpath, ((TBranch*) obj)->GetListOfBranches(), lvl+1);
      ScanList(rec, ((TBranch*) obj)->GetListOfLeaves());
   } else
   if (rec.CanExpandItem()) {
      rec.SetField("#members", "true");
      ScanObjectMemebers(rec, obj->IsA(), (char*) obj, 0);
   }
}

bool dabc_root::RootSniffer::ScanRec::MakeChild(ScanRec& super, const std::string& foldername)
{
   // if parent done, no need to start with new childs
   if (super.Done()) return false;

   prnt = super.top;

   lvl = super.lvl;
   searchpath = super.searchpath;
   mask = super.mask;
   parent_rec = &super;

   if (mask & mask_ChldMemb) {
      mask = (mask & ~mask_ChldMemb) | mask_Members;
   } else
   if (mask & mask_Members) {
      mask = mask & ~mask_Members;
   }

   if (!foldername.empty()) {
      if (searchpath) {
         //DOUT0("SEARCH folder %s in path %s", foldername.c_str(), searchpath);
         if (strncmp(searchpath, foldername.c_str(), foldername.length())!=0) return false;
         searchpath += foldername.length();
         if (*searchpath != '/') return false;
         searchpath++;
         prnt = super.top.FindChild(foldername.c_str());

         if (prnt.null()) {
            EOUT("Did not found folder %s in items", foldername.c_str());
            return false;
         }

      } else {
         prnt = super.top.CreateChild(foldername);
         //printf("CREATE FOLDER %s %p\n", foldername.c_str(), prnt());
      }
      lvl++;
   }

   if (searchpath==0) {
      // after item is located, we just normally scan
      if (mask & mask_Expand) {
         mask = mask_Scan;
         if (prnt.NumChilds()>0)
            EOUT("Non-empty element which want to be expanded");
      }

   } else {
      // while item name can be changed, we find first item itself and than
      // try to find object name

      //DOUT0("Extract item name from %s", searchpath);

      const char* separ = strchr(searchpath, '/');

      if (separ == 0) {
         itemname = searchpath;
         searchpath = 0;
      } else {
         itemname.assign(searchpath, separ - searchpath);
         searchpath = separ+1;
         if (*searchpath == 0) searchpath = 0;
      }

      sub = prnt.FindChild(itemname.c_str());

      //DOUT0("SEARCH item %s result %p", itemname.c_str(), sub());

      if (!sub.null()) {
         // we use created hierarchy to reconstruct real object name
         if (sub.HasField(dabc::prop_realname))
            objname = sub.Field(dabc::prop_realname).AsStr();
         else
            objname = itemname;

         //DOUT0("OBJECT NAME is %s ", objname.c_str());
      }
   }

   lvl++;

   return true;
}

bool dabc_root::RootSniffer::ScanRec::Done()
{
   if (res==0) return false;

   // in such special case event when result assigned, we must continue looping to create all childs
   if (((mask & (mask_Expand | mask_Search)) != 0) && objname.empty()) return false;

   return true;
}


bool dabc_root::RootSniffer::ScanRec::TestObject(TObject* obj)
{
   if (obj==0) return false;

   top.Release();

   const char* obj_name = obj->GetName();

   // exclude zero names
   if ((obj_name==0) || (*obj_name==0)) return false;

   // when scanning hierarchy, just add new item and signal it
   if ((mask & mask_Scan) != 0) {
      top = prnt.CreateChild(obj_name);
      //printf("CREATE SCAN %s %p\n", obj_name, top());
      return true;
   }

   // if searched name is known, sub-item is exists and we just need to identify object
   if (!objname.empty()) {
      if (objname != obj_name) return false;
      top = sub;
      return true;
   }

   // when trying to expand or search without knowing of exact object name
   // we will try to create all subitems to reproduce finally itemname we are searching
   if ((mask & (mask_Expand | mask_Search)) != 0) {

      if (itemname.empty()) return false;

      dabc::Hierarchy h = prnt.CreateChild(obj_name);

      //printf("CREATE EXPAND %s %p\n", obj_name, h());

      if (itemname == h.GetName()) {
         top = h;
         return true;
      }
   }

   return false;
}

bool dabc_root::RootSniffer::ScanRec::SetResult(TObject* obj)
{
   // if result already set, nothing to do
   if (res != 0) return true;

   // only when doing search, result will be propagated
   if ((mask & mask_Search) == 0) return false;

   //DOUT0("Set RESULT obj = %p search path = %s", obj, searchpath ? searchpath : "-null-");

   // only when full search path is scanned
   if (searchpath!=0) return false;

   ScanRec* rec = this;
   while (rec) {
      rec->res = obj;
      rec = rec->parent_rec;
   }

   return true;
}


void dabc_root::RootSniffer::ScanList(ScanRec& rec,
                                      TCollection* lst,
                                      const std::string& foldername)
{
   if ((lst==0) || (lst->GetSize()==0)) return;

   ScanRec chld;
   if (!chld.MakeChild(rec, foldername)) return;

   TIter iter(lst);
   TObject* obj(0);

   // DOUT0("SEARCH OBJECT with name %s ", itemname.c_str());

   while (((obj = iter()) != 0) && !chld.Done()) {

      if (chld.TestObject(obj))
         ScanObject(chld, obj);
   }
}

void dabc_root::RootSniffer::ScanRec::SetField(const std::string& name, const std::string& value)
{
   if (((mask & (mask_Scan | mask_Expand)) != 0) && !top.null()) top.SetField(name, value);
}


void dabc_root::RootSniffer::ScanRoot(ScanRec& rec)
{
   rec.SetField(dabc::prop_kind, "ROOT.Session");

   TFolder* topf = dynamic_cast<TFolder*> (gROOT->FindObject("//root/dabc"));

   ScanList(rec, gROOT->GetList());

   ScanList(rec, gROOT->GetListOfCanvases(), "Canvases");

   ScanList(rec, gROOT->GetListOfFiles(), "Files");

   ScanList(rec, topf ? topf->GetListOfFolders() : 0, "Objects");
}


void dabc_root::RootSniffer::RescanHierarchy(dabc::Hierarchy& main)
{
   main.Release();
   main.Create("ROOT");

   // this is fake element, which is need to be requested before first
   dabc::Hierarchy si = main.CreateChild("StreamerInfo");
   si.SetField(dabc::prop_kind, "ROOT.TList");
   si.SetField(dabc::prop_hash, fProducer->GetStreamerInfoHash());

   ScanRec rec;
   rec.top = main;
   rec.mask = mask_Scan;

   ScanRoot(rec);
}

void dabc_root::RootSniffer::ExpandHierarchy(dabc::Hierarchy& main, const std::string& itemname)
{
   ScanRec rec;
   rec.top = main;
   rec.searchpath = itemname.c_str();
   if (*rec.searchpath == '/') rec.searchpath++;

   rec.mask = mask_Expand;

   dabc::Hierarchy h = main.FindChild(rec.searchpath);
   if (h.NumChilds()>0)
      h.RemoveChilds();

   ScanRoot(rec);
}

void* dabc_root::RootSniffer::FindInHierarchy(dabc::Hierarchy& main, const std::string& itemname)
{
   ScanRec rec;
   rec.top = main;
   rec.searchpath = itemname.c_str();
   rec.mask = mask_Search;

   if (*rec.searchpath == '/') rec.searchpath++;

   ScanRoot(rec);

   return rec.res;
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

      TObject* obj = (TObject*) FindInHierarchy(fRoot, itemname);

      if (obj==0) {
         EOUT("Object for item %s not found", itemname.c_str());
         return dabc::cmd_false;
      }

      DOUT2("OBJECT FOUND %s %p", itemname.c_str(), obj);

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
      fHierarchy.FillBinHeader(itemname, buf, mhash, "StreamerInfo");
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
      RescanHierarchy(fRoot);

/*      DOUT0("ROOT\n%s", fRoot.SaveToXml().c_str());
      void *res = FindInHierarchy(fRoot, "/Files/hsimple.root/hpxpy[0]/");
      DOUT0("FOUND res = %p", res);
      res = FindInHierarchy(fRoot, "/Objects/extra/c1/TPad/fPrimitives/hpx/");
      DOUT0("FOUND2 res = %p", res);
      dabc::Hierarchy h = fRoot.FindChild("/Objects/extra/c1/TPad/fPrimitives/");
      DOUT0("BEFORE num = %u", h.NumChilds());
      ExpandHierarchy(fRoot, "/Objects/extra/c1/TPad/fPrimitives/");
      h = fRoot.FindChild("/Objects/extra/c1/TPad/fPrimitives/");
      DOUT0("AFTER num = %u", h.NumChilds());
      ExpandHierarchy(fRoot, "/Objects/extra/c1/TPad/fPrimitives/");
      h = fRoot.FindChild("/Objects/extra/c1/TPad/fPrimitives/");
      DOUT0("AFTER num = %u", h.NumChilds());
      if (!h.null()) DOUT0("AFTER  \n%s", h.SaveToXml().c_str());
      res = FindInHierarchy(fRoot, "/Objects/extra/c1/TPad/fPrimitives/hpx/");
      DOUT0("FOUND3 res = %p", res);
      exit(1);
*/

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
      if (cmd.IsName("GetGlobalNamesList")) {
         std::string item = cmd.GetStr("subitem");

         DOUT2("Request extended ROOT hierarchy %s", item.c_str());

         ExpandHierarchy(fRoot, item);

         dabc::Hierarchy res = fRoot.FindChild(item.c_str());

         if (res.null()) cmd.ReplyFalse();

         if (cmd.GetBool("asxml")) {
            std::string str = res.SaveToXml(dabc::xmlmask_TopDabc, cmd.GetStr("path"));
            DOUT2("Request res = \n%s", str.c_str());
            cmd.SetStr("xml", str);
         } else {
            dabc::Buffer buf = res.SaveToBuffer(dabc::stream_NamesList);
            cmd.SetRawData(buf);
         }

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

bool dabc_root::RootSniffer::RegisterObject(const char* subfolder, TObject* obj)
{
   if (obj==0) return false;

   TFolder* topf = gROOT->GetRootFolder();

   if (topf==0) {
      EOUT("Not found top ROOT folder!!!");
      return false;
   }

   TFolder* topdabcfold = dynamic_cast<TFolder*> (topf->FindObject("dabc"));
   if (topdabcfold==0) {
      topdabcfold = topf->AddFolder("dabc", "Top DABC folder");
      topdabcfold->SetOwner(kFALSE);
   }

   TFolder* dabcfold = topdabcfold;

   if ((subfolder!=0) && (strlen(subfolder)>0)) {

      TObjArray* arr = TString(subfolder).Tokenize("/");
      for (Int_t i=0; i <= (arr ? arr->GetLast() : -1); i++) {

         const char* subname = arr->At(i)->GetName();
         if (strlen(subname)==0) continue;

         TFolder* fold = dynamic_cast<TFolder*> (dabcfold->FindObject(subname));
         if (fold==0) {
            fold = dabcfold->AddFolder(subname, "DABC sub-folder");
            fold->SetOwner(kFALSE);

            if ((dabcfold == topdabcfold) && (strcmp(subname, "extra")==0))
               fold->SetBit(BIT(19), kTRUE);
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


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

extern "C" void R__zipMultipleAlgorithm(int cxlevel, int *srcsize, char *src, int *tgtsize, char *tgt, int *irep, int compressionAlgorithm);

#include "dabc/Iterator.h"

dabc_root::RootBinDataContainer::RootBinDataContainer(TBufferFile* buf) :
   dabc::BinDataContainer(),
   fBuf(buf)
{
}

dabc_root::RootBinDataContainer::~RootBinDataContainer()
{
    delete fBuf;
    fBuf = 0;
}

void* dabc_root::RootBinDataContainer::data() const
{
   return fBuf ? fBuf->Buffer() : 0;
}

unsigned dabc_root::RootBinDataContainer::length() const
{
   return fBuf ? fBuf->Length() : 0;
}

// =========================================================================

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
   fCompression(5),
   fTimer(0),
   fRoot(),
   fHierarchy(),
   fHierarchyMutex(),
   fRootCmds(dabc::CommandsQueue::kindPostponed),
   fMemFile(0),
   fSinfoSize(0),
   fLastUpdate()
{
   fEnabled = Cfg("enabled", cmd).AsBool(false);
   if (!fEnabled) return;

   fBatch = Cfg("batch", cmd).AsBool(true);
   fCompression = Cfg("compress", cmd).AsInt(5);

   if (fBatch) gROOT->SetBatch(kTRUE);
}

dabc_root::RootSniffer::~RootSniffer()
{
   if (fTimer) fTimer->fSniffer = 0;
}

void dabc_root::RootSniffer::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("sniffer was not enabled - why it is started??");
      return;
   }
   // identify ourself as bin objects producer
   fHierarchy.Create("ROOT");
   DOUT2("Root sniffer %s is bin producer!!!", ItemName().c_str());

   printf("ROOOOOOOT sniffer assign to the thread timer = %p!!!!!", fTimer);


   if (fTimer==0) {
      ActivateTimeout(0);

      TH1* h1 = new TH1F("histo1","Tilte of histo1", 100, -10., 10.);
      h1->FillRandom("gaus", 10000);

      Double_t x[100], y[100];
      Int_t n = 20;
      for (Int_t i=0;i<n;i++) {
        x[i] = i*0.1;
        y[i] = 10*sin(x[i]+0.2);
      }
      TGraph* gr = new TGraph(n,x,y);
      gr->SetName("graph");
      gROOT->Add(gr);
   }


   // if timer not installed, emulate activity in ROOT by regular timeouts
}

double dabc_root::RootSniffer::ProcessTimeout(double last_diff)
{

   printf("ROOOOOOOT sniffer process timeout");

   TH1* h1 = (TH1*) gROOT->FindObject("histo1");
   if (h1) h1->FillRandom("gaus", 100000);

   dabc::Hierarchy h;
   h.Create("ROOT");
   // this is fake element, which is need to be requested before first
   h.CreateChild("StreamerInfo").Field(dabc::prop_kind).SetStr("ROOT.TList");

   ScanRootHierarchy(h);

   DOUT2("ROOT %p hierarchy = \n%s", gROOT, h.SaveToXml().c_str());

   dabc::LockGuard lock(fHierarchyMutex);

   fHierarchy.Update(h);

   return 10.;
}

int dabc_root::RootSniffer::SimpleGetBinary(dabc::Command cmd)
{
   std::string item = cmd.GetStdStr("Item");

   TBufferFile* sbuf = 0;

   if (item=="StreamerInfo") {
      sbuf = ProduceStreamerInfos();
   } else {
      TObject* obj = gROOT->FindObject(item.c_str());

      if (obj!=0) {

         // TH1* h1 = dynamic_cast<TH1*> (obj);

         sbuf = new TBufferFile(TBuffer::kWrite, 100000);
         gFile = 0;
         sbuf->MapObject(obj);
         obj->Streamer(*sbuf);
      }
   }

   if (sbuf!=0) {
      //         dabc::Hierarchy h = fHierarchy.FindChild(item.c_str());
      //         DOUT0("BINARY Item %s version %u", item.c_str(), h.GetVersion());
      DOUT0("Produced buffer length %d", sbuf->Length());

      cmd.SetRef("#BinData", dabc::BinData(new RootBinDataContainer(sbuf)));
   }

   return dabc::cmd_true;
}


int dabc_root::RootSniffer::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetBinary")) {

      // DOUT0("COMMAND GETBINARY RECEIVED BY dabc_root::RootSniffer");

      // keep simple case for testing
      if (fTimer==0) return SimpleGetBinary(cmd);

      dabc::LockGuard lock(fHierarchyMutex);

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

      size_t pos = (itemname.find_last_of("/#<>"));
      if (pos!=std::string::npos) itemname = itemname.substr(pos+1);
      if (itemname.empty()) itemname = "item";
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

         if (obj->InheritsFrom(TH1::Class())) {
            chld.Field(dabc::prop_content_hash).SetDouble(((TH1*)obj)->Integral());
         }
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
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TTree*) obj)->GetListOfBranches(), lvl+1);
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TTree*) obj)->GetListOfLeaves(), lvl+1);
   } else
   if (obj->InheritsFrom(TBranch::Class())) {
      if (!res) res = ScanListHierarchy(chld, searchpath, ((TBranch*) obj)->GetListOfBranches(), lvl+1);
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

      if (chld.HasField(dabc::prop_realname)) itemname = chld.Field(dabc::prop_realname).AsStdStr();

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

//   TFolder* topf = dynamic_cast<TFolder*> (gROOT->FindObject("//root"));

//   if (topf!=0)
//      return ScanListHierarchy(h, searchpath, topf->GetListOfFolders(), 0);

   if (searchpath==0) h.Field(dabc::prop_kind).SetStr("ROOT.Session");

   if (!res) res = ScanListHierarchy(h, searchpath, gROOT->GetList(), 0);

   if (!res) res = ScanListHierarchy(h, searchpath, gROOT->GetListOfCanvases(), 0, "Canvases");

   if (!res) res = ScanListHierarchy(h, searchpath, gROOT->GetListOfFiles(), 0, "Files");

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

TBufferFile* dabc_root::RootSniffer::ProduceStreamerInfos()
{

   dabc::Iterator iter(fHierarchy);

   dabc::HierarchyContainer* cont = 0;

   TObjArray arr;

   while (iter.next_cast(cont)) {

      dabc::Hierarchy item(cont);

      const char* kind = item.GetField(dabc::prop_kind);
      if (kind==0) continue;

      //DOUT0("KIND = %s", kind);

      if (strstr(kind,"ROOT.")!=kind) continue;

      //DOUT0("Serach for = %s", kind+5);

      TClass* cl = TClass::GetClass(kind+5, kFALSE);

      if (!IsDrawableClass(cl)) continue;

      if (!arr.FindObject(cl)) {
         arr.Add(cl);
         DOUT0("FOUND ROOT CLASS %s", cl->GetName());
      }
   }

   if (arr.GetSize()==0) arr.Add(TH1F::Class());

   TDirectory* olddir = gDirectory; gDirectory = 0;
   TFile* oldfile = gFile; gFile = 0;

   TMemFile* mem = new TMemFile("dummy.file", "RECREATE");
   gROOT->GetListOfFiles()->Remove(mem);

   // here we are creating dummy objects to fill streamer infos
   // works for TH1/TGraph, should always be tested with new classes
   for (Int_t n = 0; n<=arr.GetLast(); n++) {
      TClass* cl = (TClass*) arr[n];

      void* obj = cl->New();

      mem->WriteObjectAny(obj, cl, Form("obj%d",n));

      cl->Destructor(obj);
   }

   /*
   TH1F* d = new TH1F("d","d", 10, 0, 10);
   mem->WriteObject(d, "h1");
   TGraph* gr = new TGraph(10);
   gr->SetName("abc");
   gr->Draw("AC*");
   mem->WriteObject(gr, "gr1");
   */

   mem->WriteStreamerInfo();

   TList* l = mem->GetStreamerInfoList();
   //l->Print("*");

   TBufferFile* sbuf = new TBufferFile(TBuffer::kWrite, 100000);
   // sbuf->SetParent(mem);
   sbuf->MapObject(l);
   l->Streamer(*sbuf);

   delete l;

   delete mem;

   DOUT0("Finish with streamer info");

   gDirectory = olddir;
   gFile  = oldfile;

   return sbuf;
}

void dabc_root::RootSniffer::BuildWorkerHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects

   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy(cont).Field(dabc::prop_binary_producer).SetStr(ItemName());

   // we protect hierarchy with mutex, while it may be changed from ROOT process
   dabc::LockGuard lock(fHierarchyMutex);

   if (!fHierarchy.null()) fHierarchy()->BuildHierarchy(cont);
}


void dabc_root::RootSniffer::InstallSniffTimer()
{
   if (fTimer==0) {
      fTimer = new TDabcTimer(100, kTRUE);
      fTimer->SetSniffer(this);
      fTimer->TurnOn();
   }


   if (fMemFile==0) {

      TDirectory* olddir = gDirectory; gDirectory = 0;
      TFile* oldfile = gFile; gFile = 0;

      fMemFile = new TMemFile("dummy.file", "RECREATE");
      gROOT->GetListOfFiles()->Remove(fMemFile);

      TH1F* d = new TH1F("d","d", 10, 0, 10);
      fMemFile->WriteObject(d, "h1");
      delete d;

      TGraph* gr = new TGraph(10);
      gr->SetName("abc");
      //      // gr->SetDrawOptions("AC*");
      fMemFile->WriteObject(gr, "gr1");
      delete gr;

      fMemFile->WriteStreamerInfo();

      // make primary list of streamer infos
      TList* l = new TList();

      l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TGraph"));
      l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TH1F"));
      l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TH1"));
      l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TNamed"));
      l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TObject"));

      fMemFile->WriteObject(l, "ll");
      delete l;

      fMemFile->WriteStreamerInfo();

      l = fMemFile->GetStreamerInfoList();
      // l->Print("*");
      fSinfoSize = l->GetSize();
      delete l;

      gDirectory = olddir;
      gFile = oldfile;
   }
}

TBufferFile* dabc_root::RootSniffer::ProduceStreamerInfosMem()
{
   // method called from ROOT context

   TDirectory* olddir = gDirectory; gDirectory = 0;
   TFile* oldfile = gFile; gFile = 0;

   if (fMemFile==0) {

      EOUT("No mem file is exists!!!!");
      return 0;
   }

   fMemFile->WriteStreamerInfo();
   TList* l = fMemFile->GetStreamerInfoList();
   //l->Print("*");

   fSinfoSize = l->GetSize();

   TBufferFile* sbuf = new TBufferFile(TBuffer::kWrite, 100000);
   sbuf->SetParent(fMemFile);
   sbuf->MapObject(l);
   l->Streamer(*sbuf);

   delete l;

   gDirectory = olddir;
   gFile = oldfile;

   return sbuf;
}


int dabc_root::RootSniffer::ProcessGetBinary(dabc::Command cmd)
{
   // command executed in ROOT context without locked mutex,
   // one can use as much ROOT as we want

   std::string itemname = cmd.GetStdStr("Item");

   TBufferFile* sbuf = 0;

   if (itemname=="StreamerInfo") {
      sbuf = ProduceStreamerInfosMem();
   } else {

      TObject* obj = (TObject*) ScanRootHierarchy(fRoot, itemname.c_str());

      if (obj==0) {
         EOUT("Object for item %s not specified", itemname.c_str());
         return dabc::cmd_false;
      }

      if (fMemFile == 0) {
         EOUT("Mem file is not exists");
      } else {

         TDirectory* olddir = gDirectory; gDirectory = 0;
         TFile* oldfile = gFile; gFile = 0;

         TList* l1 = fMemFile->GetStreamerInfoList();

         sbuf = new TBufferFile(TBuffer::kWrite, 100000);
         sbuf->SetParent(fMemFile);
         sbuf->MapObject(obj);
         obj->Streamer(*sbuf);

         bool believe_not_changed = false;

         if ((fMemFile->GetClassIndex()==0) || (fMemFile->GetClassIndex()->fArray[0] == 0)) {
            DOUT0("SEEMS to be, streamer info was not changed");
            believe_not_changed = true;
         }

         fMemFile->WriteStreamerInfo();
         TList* l2 = fMemFile->GetStreamerInfoList();

         DOUT0("STREAM LIST Before %d After %d", l1->GetSize(), l2->GetSize());

         //DOUT0("=================== BEFORE ========================");
         //l1->Print("*");
         //DOUT0("=================== AFTER ========================");
         //l2->Print("*");

         if (believe_not_changed && (l1->GetSize() != l2->GetSize())) {
            EOUT("StreamerInfo changed when we were expecting no changes!!!!!!!!!");
            exit(444);
         }

         if (believe_not_changed && (l1->GetSize() == l2->GetSize())) {
            DOUT0("++ STREAMER INFO NOT CHANGED AS EXPECTED +++ ");
         }

         fSinfoSize = l2->GetSize();

         delete l1; delete l2;

         gDirectory = olddir;
         gFile = oldfile;
      }
   }

   DOUT0("GETBINARY item %s  data %p", itemname.c_str(), sbuf);

   if (sbuf!=0) {

      bool with_zip = true;

      Int_t noutot = 0;
      char* fBuffer = 0;

      if (with_zip) {
         Int_t cxAlgorithm = 0;
         const Int_t kMAXBUF = 0xffffff;
         Int_t fObjlen = sbuf->Length();
         Int_t nbuffers = 1 + (fObjlen - 1)/kMAXBUF;
         Int_t buflen = TMath::Max(512,fObjlen + 9*nbuffers + 28);

         fBuffer = (char*) malloc(buflen);

         char *objbuf = sbuf->Buffer();
         char *bufcur = fBuffer; // start writing from beginning

         Int_t nzip   = 0;
         Int_t bufmax = 0;
         Int_t nout = 0;

         for (Int_t i = 0; i < nbuffers; ++i) {
            if (i == nbuffers - 1) bufmax = fObjlen - nzip;
                else               bufmax = kMAXBUF;
            R__zipMultipleAlgorithm(fCompression, &bufmax, objbuf, &bufmax, bufcur, &nout, cxAlgorithm);
            if (nout == 0 || nout >= fObjlen) { //this happens when the buffer cannot be compressed
               DOUT0("Fail to zip buffer");
               free(fBuffer);
               fBuffer = 0;
               noutot = 0;
               break;
            }
            bufcur += nout;
            noutot += nout;
            objbuf += kMAXBUF;
            nzip   += kMAXBUF;
         }
      }

      if (fBuffer!=0) {
         DOUT0("ZIP compression produced buffer of total length %d from %d", noutot, sbuf->Length());
         cmd.SetRef("#BinData", dabc::BinData(new dabc::BinDataContainer(fBuffer, noutot, true)));
         delete sbuf;
      } else {
         DOUT0("Produce uncompressed data of length %d", sbuf->Length());
         cmd.SetRef("#BinData", dabc::BinData(new RootBinDataContainer(sbuf)));
      }

      cmd.SetInt("MasterHash", fSinfoSize);
   }

   return dabc::cmd_true;
}


void dabc_root::RootSniffer::ProcessActionsInRootContext()
{
//   printf("ROOOOOOOT sniffer ProcessActionsInRootContext\n");

   if (fLastUpdate.null() || fLastUpdate.Expired(3.)) {
      DOUT3("Update ROOT structures");
      fRoot.Release();
      fRoot.Create("ROOT");

      // this is fake element, which is need to be requested before first
      dabc::Hierarchy si = fRoot.CreateChild("StreamerInfo");
      si.Field(dabc::prop_kind).SetStr("ROOT.TList");
      si.Field(dabc::prop_content_hash).SetInt(fSinfoSize);

      ScanRootHierarchy(fRoot);

      fLastUpdate.GetNow();

      // DOUT0("ROOT %p hierarchy = \n%s", gROOT, fRoot.SaveToXml().c_str());

      dabc::LockGuard lock(fHierarchyMutex);
      fHierarchy.Update(fRoot);

      // DOUT0("ROOT hierarchy \n%s", fHierarchy.SaveToXml(false, (uint64_t) -1).c_str());
   }

   bool doagain(true);
   dabc::Command cmd;

   while (doagain) {

      if (cmd.IsName("GetBinary")) {
         cmd.Reply(ProcessGetBinary(cmd));
      } else
      if (!cmd.null()) {
         EOUT("Not processed command %s", cmd.GetName());
         cmd.ReplyFalse();
      }

      dabc::LockGuard lock(fHierarchyMutex);

      if (fRootCmds.Size()>0) cmd = fRootCmds.Pop();
      doagain = !cmd.null();
   }
}

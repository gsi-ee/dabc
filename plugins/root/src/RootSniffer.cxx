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

// ==============================================================================

dabc_root::RootSniffer::RootSniffer(const std::string& name) :
   dabc::Worker(MakePair(name)),
   fEnabled(false),
   fHierarchy(),
   fHierarchyMutex()
{
   fEnabled = Cfg("enabled").AsBool(true);
   if (!fEnabled) return;
}

dabc_root::RootSniffer::~RootSniffer()
{
}


void dabc_root::RootSniffer::OnThreadAssigned()
{
   if (!IsEnabled()) {
      EOUT("sniffer was not enabled - why it is started??");
      return;
   }
   // identify ourself as bin objects producer
   fHierarchy.Create("ROOT");
   DOUT0("Root sniffer %s is bin producer!!!", ItemName().c_str());

   gROOT->SetBatch(kTRUE);


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
   DOUT0("***************************** Graph histogram = %p", gr->GetHistogram());
   gROOT->Add(gr);
}

double dabc_root::RootSniffer::ProcessTimeout(double last_diff)
{
   dabc::Hierarchy h;
   h.Create("ROOT");
   // this is fake element, which is need to be requested before first
   h.CreateChild("StreamerInfo").Field(dabc::prop_kind).SetStr("ROOT.TList");

   FillHieararchy(h, gROOT);

   DOUT0("ROOT %p hierarchy = \n%s", gROOT, h.SaveToXml().c_str());

   fHierarchy.Update(h);

//   dabc::LockGuard lock(fHierarchyMutex);

//   dabc::Hierarchy local = fHierarchy.FindChild("localhost");

//   if (local.null()) {
//      EOUT("Did not find localhost in hierarchy");
//   } else {
//      // TODO: make via XML file like as for remote node!!
//   }

   return 10.;
}

int dabc_root::RootSniffer::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetBinary")) {

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

         DOUT0("Produced buffer length %d", sbuf->Length());
      }

      if (sbuf!=0) {
//         dabc::Hierarchy h = fHierarchy.FindChild(item.c_str());
//         DOUT0("BINARY Item %s version %u", item.c_str(), h.GetVersion());

         cmd.SetRef("#BinData", dabc::BinData(new RootBinDataContainer(sbuf)));
      }

      return dabc::cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


void dabc_root::RootSniffer::FillHieararchy(dabc::Hierarchy& h, TDirectory* dir)
{
   if (dir==0) return;

   TIter iter(dir ? dir->GetList() : 0);
   TObject* obj(0);
   while ((obj = iter())!=0) {
      DOUT0("Find ROOT object %s", obj->GetName());
      dabc::Hierarchy chld = h.CreateChild(obj->GetName());

      if (IsSupportedClass(obj->IsA())) {
         chld.Field(dabc::prop_kind).SetStr(dabc::format("ROOT.%s", obj->ClassName()));

         if (obj->InheritsFrom(TH1::Class())) {
            TH1* h1 = (TH1*) obj;
            h1->FillRandom("gaus", 100000);

            chld.Field(dabc::prop_content_hash).SetDouble(h1->Integral());
         }
      }
   }
}

bool dabc_root::RootSniffer::IsSupportedClass(TClass* cl)
{
   if (cl==0) return false;

   if (cl->InheritsFrom(TH1::Class())) return true;
   if (cl->InheritsFrom(TGraph::Class())) return true;
   if (cl->InheritsFrom(TCanvas::Class())) return true;
   if (cl->InheritsFrom(TProfile::Class())) return true;

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

      if (!IsSupportedClass(cl)) continue;

      if (!arr.FindObject(cl)) {
         arr.Add(cl);
         DOUT0("FOUND ROOT CLASS %s", cl->GetName());
      }
   }

   if (arr.GetSize()==0) arr.Add(TH1F::Class());


   TMemFile* mem = new TMemFile("dummy.file", "RECREATE");


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
   l->Print("*");

   TBufferFile* sbuf = new TBufferFile(TBuffer::kWrite, 100000);
   sbuf->MapObject(l);
   l->Streamer(*sbuf);

   delete l;

   delete mem;

   return sbuf;
}

void dabc_root::RootSniffer::BuildHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects

   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy(cont).Field(dabc::prop_binary_producer).SetStr(ItemName());

   if (!fHierarchy.null()) fHierarchy()->BuildHierarchy(cont);
}


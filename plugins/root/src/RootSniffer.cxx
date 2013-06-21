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

#include "TH1.h"
#include "TFile.h"
#include "TList.h"
#include "TMemFile.h"
#include "TStreamerInfo.h"
#include "TBufferFile.h"
#include "TROOT.h"


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

   ActivateTimeout(0);
   TH1* h1 = new TH1F("histo1","Tilte of histo1", 100, -10., 10.);
   h1->FillRandom("gaus", 10000);
}

double dabc_root::RootSniffer::ProcessTimeout(double last_diff)
{
   dabc::Hierarchy h;
   h.Create("ROOT");
   // this is fake element, which is need to be requested before first
   h.CreateChild("StreamerInfo").Field("kind").SetStr("ROOT.TList");

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
         TMemFile* mem = new TMemFile("dummy.file", "RECREATE");
         TH1F* d = new TH1F("d","d", 10, 0, 10);
         d->Write();
         mem->WriteStreamerInfo();

         TList* l = mem->GetStreamerInfoList();
         l->Print("*");

         sbuf = new TBufferFile(TBuffer::kWrite, 100000);
         sbuf->MapObject(l);
         l->Streamer(*sbuf);

         delete l;

         delete mem;
      } else {
         TObject* obj = gROOT->FindObject(item.c_str());

         if (obj!=0) {

            TH1* h1 = dynamic_cast<TH1*> (obj);

            if (h1) h1->FillRandom("gaus", 10000);

            sbuf = new TBufferFile(TBuffer::kWrite, 100000);

            gFile = 0;
            sbuf->MapObject(obj);
            obj->Streamer(*sbuf);
         }

         DOUT0("Produced buffer length %d", sbuf->Length());
      }

      if (sbuf!=0) cmd.SetRef("#BinData", dabc::BinData(new RootBinDataContainer(sbuf)));

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

      if (obj->InheritsFrom(TH1::Class())) {
         chld.Field("kind").SetStr(dabc::format("ROOT.%s", obj->ClassName()));
      }
   }
}


void dabc_root::RootSniffer::BuildHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects
   dabc::Hierarchy(cont).Field("#bin_producer").SetStr(ItemName());

   if (!fHierarchy.null()) fHierarchy()->BuildHierarchy(cont);
}


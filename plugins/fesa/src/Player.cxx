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

#include "fesa/Player.h"

#include "fesa/defines.h"

#include <stdlib.h>
#include <math.h>

#ifdef WITH_FESA
#include <cmw-rda/RDAService.h>
#include <cmw-rda/Config.h>
#include <cmw-rda/DeviceHandle.h>
#include <cmw-rda/ReplyHandler.h>
#endif

#ifdef WITH_ROOT
#include "dabc_root/BinaryProducer.h"
#include "TH2.h"
#include "TRandom.h"
#include "TCanvas.h"
#include "TROOT.h"
#endif

#include "dabc/Publisher.h"

#include "dabc/CommandDef.h"

fesa::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fHierarchyMutex(),
   fHierarchy(),
   fCounter(0),
   fProducer(),
   fHist(0),
   fCanvas(0),
   fRDAService(0),
   fDevice(0)
{
   fHierarchy.Create("FESA");

   // this is just emulation, later one need list of real variables
   fHierarchy.CreateChild("BeamProfile").Field(dabc::prop_kind).SetStr("FESA.2D");

   fHierarchy.CreateChild("BeamRate").Field(dabc::prop_kind).SetStr("rate");

   dabc::Hierarchy item = fHierarchy.CreateChild("BeamRate2");
   item.Field(dabc::prop_kind).SetStr("rate");
   item.EnableHistory(100);

   item = fHierarchy.CreateChild("TestRate");
   item.Field(dabc::prop_kind).SetStr("rate");
   item.EnableHistory(100);
   
   dabc::CommandDef def = fHierarchy.CreateCmdDef("CmdReset");

   fServerName = Cfg("Server", cmd).AsStdStr();
   fDeviceName = Cfg("Device", cmd).AsStdStr();
   fCycles = Cfg("Cycles", cmd).AsStdStr();
   fService = Cfg("Service", cmd).AsStdStr();
   fField = Cfg("Field", cmd).AsStdStr();

   CreateTimer("update", 1., false);

   fCounter = 0;
   
   #ifdef WITH_FESA
   if (!fServerName.empty() && !fDeviceName.empty()) {
      fRDAService = rdaRDAService::init();

      try {
         fDevice = fRDAService->getDeviceHandle(fDeviceName.c_str(), fServerName.c_str());
      } catch (...) {
         EOUT("Device %s on server %s not found", fDeviceName.c_str(), fServerName.c_str());
      }
      
      if ((fDevice!=0) && !fService.empty()) {
         item = fHierarchy.CreateChild(fService);
         item.Field(dabc::prop_kind).SetStr("rate");
         item.EnableHistory(100);
      }
      
   }
   #endif

   #ifdef WITH_ROOT

   fProducer = new dabc_root::BinaryProducer("root_bin");
   fProducer.SetAutoDestroy(true);

   fHierarchy.CreateChild("StreamerInfo").Field(dabc::prop_kind).SetStr("ROOT.TList");

   dabc::Hierarchy h1 = fHierarchy.CreateChild("BeamRoot");
   h1.Field(dabc::prop_kind).SetStr("ROOT.TH2I");
   h1.Field(dabc::prop_masteritem).SetStr("StreamerInfo");

   fHierarchy.CreateChild("ImageRoot").Field(dabc::prop_kind).SetStr("image.png");

   TH2I* h2 = new TH2I("BeamRoot","Root beam profile", 32, 0, 32, 32, 0, 32);
   h2->SetDirectory(0);
   fHist = h2;

   gROOT->SetBatch(kTRUE);
   TCanvas* can = new TCanvas("can1", "can1", 3);
   fCanvas = can;
   #endif

   Publish(fHierarchy, "FESA/Test", &fHierarchyMutex);
}

fesa::Player::~Player()
{
  
   #ifdef WITH_ROOT
   if (fCanvas) {
      delete (TCanvas*) fCanvas;
      fCanvas = 0;
   }
   if (fHist) {
      delete (TH2I*) fHist;
      fHist = 0;
   }
   #endif

   //fHierarchy.Destroy();
}

void fesa::Player::ProcessTimerEvent(unsigned timer)
{
//   DOUT0("Process timer");

   fCounter++;

   dabc::Buffer buf = dabc::Buffer::CreateBuffer(sizeof(dabc::BinDataHeader)+sizeof(fesa::BeamProfile));

   dabc::BinDataHeader* hdr = (dabc::BinDataHeader*) buf.SegmentPtr();
   hdr->reset();
   hdr->zipped = 0; // no ziping (yet)
   hdr->payload = sizeof(fesa::BeamProfile);

   fesa::BeamProfile* rec = (fesa::BeamProfile*) (((char*) buf.SegmentPtr()) + sizeof(dabc::BinDataHeader));
   rec->fill(fCounter % 7);

#ifdef WITH_FESA
   if ((fDevice!=0) && !fService.empty()) {
      double res = doGet(fService, fField);
      dabc::LockGuard lock(fHierarchyMutex);
      fHierarchy.FindChild(fService.c_str()).Field("value").SetDouble(res);
      // DOUT0("GET FESA field %s = %5.3f", fService.c_str(), res);
   }
#endif

   dabc::LockGuard lock(fHierarchyMutex);
   
   dabc::Hierarchy item = fHierarchy.FindChild("BeamProfile");

   // DOUT0("Set binary buffer %u to item %s %p", buf.GetTotalSize(), item.GetName(), item.GetObject());

   item()->bindata() = buf;
   item.Field(dabc::prop_hash).SetInt(fCounter);

   double v1 = 100. * (1.3 + sin(dabc::Now().AsDouble()/5.));
   fHierarchy.FindChild("BeamRate").Field("value").SetStr(dabc::format("%4.2f", v1));
   int64_t arr[5] = {1,7,4,2,3};
   fHierarchy.FindChild("BeamRate").Field("arr").SetArrInt(5, arr);

//   std::vector<int> res;
//   res = fHierarchy.FindChild("BeamRate").Field("arr").AsIntVector();
//   DOUT0("Get vector of size %u", res.size());
//   for (unsigned n=0;n<res.size();n++) DOUT0("   arr[%u] = %d", n, res[n]);

   v1 = 100. * (1.3 + cos(dabc::Now().AsDouble()/8.));
   fHierarchy.FindChild("BeamRate2").Field("value").SetStr(dabc::format("%4.2f", v1));

   int test = fCounter % 100;
   v1 = 20 + (test & 0xfffffc) + (test & 3)*0.01;
   fHierarchy.FindChild("TestRate").Field("value").SetStr(dabc::format("%4.2f", v1));

#ifdef WITH_ROOT

   dabc_root::BinaryProducer* pr = (dabc_root::BinaryProducer*) fProducer();

   item = fHierarchy.FindChild("StreamerInfo");
   item.Field(dabc::prop_hash).SetStr(pr->GetStreamerInfoHash());

   item = fHierarchy.FindChild("BeamRoot");
   TH2I* h2 = (TH2I*) fHist;
   if (h2!=0) {
      for (int n=0;n<100;n++)
         h2->Fill(gRandom->Gaus(16,2), gRandom->Gaus(16,1));
      item.Field(dabc::prop_hash).SetInt(h2->GetEntries());
   }

   TCanvas* can = (TCanvas*) fCanvas;
   if ((can!=0) && (h2!=0)) {
      can->cd();
      h2->Draw("col");
      can->Modified();
      can->Update();
   }
#endif

   fHierarchy.MarkChangedItems();

//   if (fCounter % 10 == 0)
//      DOUT0("History BeamRate2 \n%s", fHierarchy.FindChild("BeamRate2").GetObject()->RequestHistoryAsXml().c_str());

#ifdef DABC_EXTRA_CHECKS
//   if (fCounter % 10 == 0)
//      dabc::Object::DebugObject();
#endif

}


int fesa::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {

      std::string itemname = cmd.GetStr("subitem");

      dabc::LockGuard lock(fHierarchyMutex);

      dabc::Hierarchy item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("No find item %s to get binary", itemname.c_str());
         return dabc::cmd_false;
      }

      dabc::Buffer buf;
      std::string mhash;

      std::string kind = item.Field(dabc::prop_kind).AsStdStr();
      DOUT0("GetBinary for item %s kind %s", itemname.c_str(), kind.c_str());
      if ((kind.find("ROOT.")==0) || (kind=="image.png")) {
#ifdef WITH_ROOT
         dabc_root::BinaryProducer* pr = (dabc_root::BinaryProducer*) fProducer();
         if (itemname=="StreamerInfo") {
            buf = pr->GetStreamerInfoBinary();
         } else {
            TObject* obj = (TH2I*) fHist;
            if (itemname =="ImageRoot") obj = (TCanvas*) fCanvas;
            buf = pr->GetBinary(obj, (kind=="image.png"));
            mhash = pr->GetStreamerInfoHash();
         }
#endif
      } else
         buf = item()->bindata();

      if (buf.null()) {
         EOUT("No find binary data for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      item.FillBinHeader("", buf, mhash);

      cmd.SetRawData(buf);

      return dabc::cmd_true;
   } else
   if (cmd.IsName("CmdReset")) {

      DOUT0("****************** CmdReset ****************");
      fCounter = 0;
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

double fesa::Player::doGet(const std::string& service, const std::string& field)
{
   double res = 0.;
  
   #ifdef WITH_FESA
      if (fDevice==0) return 0.;
      try {
        rdaData context;
        rdaData* value = fDevice->get(service.c_str(), fCycles.c_str(), context);
        if (value!=0) {
          res = value->extractDouble(field.c_str());
          delete value;
        }
   }
   catch (const rdaException& ex)
   {
     EOUT("Exception caught in doGet: %s  %s", ex.getType(), ex.getMessage());
   }
   catch (...)
   {
        EOUT("Uncknown exception caught in doGet");;
   }
   #endif
   return res;
}


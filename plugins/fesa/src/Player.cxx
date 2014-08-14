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

#include "fesa/Player.h"

#include <stdlib.h>
#include <math.h>

#include "dabc/Publisher.h"

#ifdef WITH_FESA
#include <cmw-rda/RDAService.h>
#include <cmw-rda/Config.h>
#include <cmw-rda/DeviceHandle.h>
#include <cmw-rda/ReplyHandler.h>
#endif

#ifdef WITH_ROOT
#include "TH2.h"
#include "TRandom.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TRootSniffer.h"

class MySniffer : public TRootSniffer {
   public:
      TH2* fHist;
      MySniffer(TH2* hist) : TRootSniffer("MySniffer"), fHist(hist) {}

      virtual void* FindInHierarchy(const char* path, TClass** cl=0, TDataMember** member = 0, Int_t* chld = 0)
      {
         if (path==0) return 0;
         if (!strcmp(path, "ImageRoot") || !strcmp(path, "BeamRoot")) {
            if (cl) *cl = fHist->IsA();
            return fHist;
         }
         return 0;
      }

};

#endif


namespace fesa {

   struct BeamProfile {
      enum { sizex = 16, sizey = 16 };

      uint32_t arr[sizex][sizey];

      void clear()
      {
         for (unsigned x=0;x<sizex;x++)
            for (unsigned y=0;y<sizey;y++)
               arr[x][y] = 0;
      }

      void fill(int kind = 0)
      {
         for (unsigned x=0;x<sizex;x++)
            for (unsigned y=0;y<sizey;y++)
               switch (kind) {
                  case 0: arr[x][y] = x+y; break;
                  case 1: arr[x][y] = sizex - x + sizey-y; break;
                  case 2: arr[x][y] = sizex - x + y; break;
                  case 3: arr[x][y] = x + sizey-y; break;
                  default: {
                     arr[x][y] = (unsigned) (sizex + sizey - 3*sqrt((x-sizex/2)*(x-sizex/2) + (y-sizey/2)*(y-sizey/2)));
                  }
               }
      }
   };

}



fesa::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fCounter(0),
   fSniffer(0),
   fHist(0),
   fRDAService(0),
   fDevice(0)
{
   fWorkerHierarchy.Create("FESA", true);

   // this is just emulation, later one need list of real variables
   fWorkerHierarchy.CreateHChild("BeamProfile").SetField(dabc::prop_kind, "FESA.2D");

   fWorkerHierarchy.CreateHChild("BeamRate").SetField(dabc::prop_kind, "rate");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("BeamRate2");
   item.SetField(dabc::prop_kind, "rate");
   item.EnableHistory(100);

   item = fWorkerHierarchy.CreateHChild("TestRate");
   item.SetField(dabc::prop_kind, "rate");
   item.EnableHistory(100);
   
   CreateCmdDef("CmdReset").AddArg("counter", "int", true, "5");

   fServerName = Cfg("Server", cmd).AsStr();
   fDeviceName = Cfg("Device", cmd).AsStr();
   fCycles = Cfg("Cycles", cmd).AsStr();
   fService = Cfg("Service", cmd).AsStr();
   fField = Cfg("Field", cmd).AsStr();

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
         item = fWorkerHierarchy.CreateHChild(fService);
         item.SetField(dabc::prop_kind,"rate");
         item.EnableHistory(100);
      }
      
   }
   #endif

   #ifdef WITH_ROOT

   fWorkerHierarchy.CreateHChild("StreamerInfo").SetField(dabc::prop_kind, "ROOT.TStreamerInfoList");

   dabc::Hierarchy h1 = fWorkerHierarchy.CreateHChild("BeamRoot");
   h1.SetField(dabc::prop_kind, "ROOT.TH2I");

   h1 = fWorkerHierarchy.CreateHChild("ImageRoot");
   h1.SetField(dabc::prop_kind, "ROOT.TH2I");
   h1.SetField(dabc::prop_view, "png");

   TH2I* h2 = new TH2I("BeamRoot","Root beam profile", 32, 0, 32, 32, 0, 32);
   h2->SetDirectory(0);
   fHist = h2;

   fSniffer = new MySniffer(h2);

   #endif

   PublishPars("FESA/Test");
}

fesa::Player::~Player()
{
  
   #ifdef WITH_ROOT
   if (fSniffer) {
      delete fSniffer;
      fSniffer = 0;
   }
   if (fHist) {
      delete (TH2I*) fHist;
      fHist = 0;
   }
   #endif

   //fWorkerHierarchy.Destroy();
}

void fesa::Player::ProcessTimerEvent(unsigned timer)
{
//   DOUT0("Process timer");

   fCounter++;

   dabc::Buffer buf = dabc::Buffer::CreateBuffer(sizeof(fesa::BeamProfile));

   fesa::BeamProfile* rec = (fesa::BeamProfile*) buf.SegmentPtr();
   rec->fill(fCounter % 7);

#ifdef WITH_FESA
   if ((fDevice!=0) && !fService.empty()) {
      double res = doGet(fService, fField);
      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());
      fWorkerHierarchy.GetHChild(fService).SetField("value", res);
      // DOUT0("GET FESA field %s = %5.3f", fService.c_str(), res);
   }
#endif

   dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());
   
   dabc::Hierarchy item = fWorkerHierarchy.GetHChild("BeamProfile");

   // DOUT0("Set binary buffer %u to item %s %p", buf.GetTotalSize(), item.GetName(), item.GetObject());

   item()->bindata() = buf;
   item.SetField(dabc::prop_hash, fCounter);

   double v1 = 100. * (1.3 + sin(dabc::Now().AsDouble()/5.));
   fWorkerHierarchy.GetHChild("BeamRate").SetField("value", dabc::format("%4.2f", v1));
   // int64_t arr[5] = {1,7,4,2,3};
   //fWorkerHierarchy.FindChild("BeamRate").Field("arr").SetArrInt(5, arr);

//   std::vector<int> res;
//   res = fWorkerHierarchy.FindChild("BeamRate").Field("arr").AsIntVector();
//   DOUT0("Get vector of size %u", res.size());
//   for (unsigned n=0;n<res.size();n++) DOUT0("   arr[%u] = %d", n, res[n]);

   v1 = 100. * (1.3 + cos(dabc::Now().AsDouble()/8.));
   fWorkerHierarchy.GetHChild("BeamRate2").SetField("value", dabc::format("%4.2f", v1));

   int test = fCounter % 100;
   v1 = 20 + (test & 0xfffffc) + (test & 3)*0.01;
   fWorkerHierarchy.GetHChild("TestRate").SetField("value", dabc::format("%4.2f", v1));

#ifdef WITH_ROOT

   fWorkerHierarchy.GetHChild("StreamerInfo").SetField(dabc::prop_hash, fSniffer->GetStreamerInfoHash());

   TH2I* h2 = (TH2I*) fHist;
   if (h2!=0) {
      for (int n=0;n<100;n++)
         h2->Fill(gRandom->Gaus(16,4), gRandom->Gaus(16,2));
      fWorkerHierarchy.GetHChild("BeamRoot").SetField(dabc::prop_hash, (uint64_t) h2->GetEntries());
   }
#endif

   fWorkerHierarchy.MarkChangedItems();

#ifdef DABC_EXTRA_CHECKS
//   if (fCounter % 10 == 0)
//      dabc::Object::DebugObject();
#endif

}


int fesa::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {

      std::string itemname = cmd.GetStr("subitem");
      std::string binkind = cmd.GetStr("Kind");
      std::string query = cmd.GetStr("Query");

      // DOUT0("fesa::Player CmdGetBinary item %s kind %s", itemname.c_str(), binkind.c_str());

      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());

      dabc::Hierarchy item = fWorkerHierarchy.GetHChild(itemname);

      if (item.null()) {
         EOUT("Item %s not found to get binary", itemname.c_str());
         return dabc::cmd_false;
      }

      dabc::Buffer buf;

      if (binkind == "dabc.bin") {
         buf = item()->bindata();
         item.FillBinHeader("", cmd);
      } else {

#ifdef WITH_ROOT
        void* ptr(0);
        Long_t length(0);

        if (fSniffer->Produce(itemname.c_str(), binkind.c_str(), query.c_str(), ptr, length)) {
           buf = dabc::Buffer::CreateBuffer(ptr, (unsigned) length, true);
           item.FillBinHeader("", cmd, fSniffer->GetStreamerInfoHash());
        } else {
           EOUT("Player fail to produce item %s", itemname.c_str());
        }
#endif
      }

      if (buf.null()) {
         EOUT("No find binary data for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      cmd.SetRawData(buf);

      return dabc::cmd_true;
   } else
   if (cmd.IsName("CmdReset")) {

      fCounter = cmd.GetInt("counter", 0);
      DOUT0("****************** CmdReset counter=%d ****************", fCounter);
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


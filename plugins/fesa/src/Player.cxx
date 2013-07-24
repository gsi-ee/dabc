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


#ifdef WITH_ROOT
#include "dabc_root/BinaryProducer.h"
#include "TH2.h"
#include "TRandom.h"
#endif

fesa::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProducer(),
   fHist(0)
{
   fHierarchy.Create("FESA");

   // this is just emulation, later one need list of real variables
   fHierarchy.CreateChild("BeamProfile").Field(dabc::prop_kind).SetStr("FESA.2D");

   fHierarchy.CreateChild("BeamRate").Field(dabc::prop_kind).SetStr("rate");

   dabc::Hierarchy item = fHierarchy.CreateChild("BeamRate2");
   item.Field(dabc::prop_kind).SetStr("rate");
   item.EnableHistory(100,"value");

   CreateTimer("update", 1., false);

   fCounter = 0;

   #ifdef WITH_ROOT

   fProducer = new dabc_root::BinaryProducer("root_bin");
   fProducer.SetAutoDestroy(true);

   fHierarchy.CreateChild("StreamerInfo").Field(dabc::prop_kind).SetStr("ROOT.TList");

   dabc::Hierarchy h1 = fHierarchy.CreateChild("BeamRoot");
   h1.Field(dabc::prop_kind).SetStr("ROOT.TH2I");
   h1.Field(dabc::prop_masteritem).SetStr("StreamerInfo");

   TH2I* h2 = new TH2I("BeamRoot","Root beam profile", 32, 0, 32, 32, 0, 32);
   h2->SetDirectory(0);
   fHist = h2;

   #endif

}

fesa::Player::~Player()
{
#ifdef WITH_ROOT
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

   dabc::Hierarchy item = fHierarchy.FindChild("BeamProfile");

   // DOUT0("Set binary buffer %u to item %s %p", buf.GetTotalSize(), item.GetName(), item.GetObject());

   item()->bindata() = buf;
   item.Field(dabc::prop_content_hash).SetInt(fCounter);


   double v1 = 100. * (1.3 + sin(dabc::Now().AsDouble()/5.));

   item = fHierarchy.FindChild("BeamRate");
   item.Field("value").SetStr(dabc::format("%4.2f", v1));

   v1 = 100. * (1.3 + cos(dabc::Now().AsDouble()/8.));
   item = fHierarchy.FindChild("BeamRate2");
   item.Field("value").SetStr(dabc::format("%4.2f", v1));


#ifdef WITH_ROOT

   dabc_root::BinaryProducer* pr = (dabc_root::BinaryProducer*) fProducer();

   item = fHierarchy.FindChild("StreamerInfo");
   item.Field(dabc::prop_content_hash).SetInt(pr->GetStreamerInfoHash());

   item = fHierarchy.FindChild("BeamRoot");
   TH2I* h2 = (TH2I*) fHist;
   if (h2!=0) {
      for (int n=0;n<100;n++)
         h2->Fill(gRandom->Gaus(16,2), gRandom->Gaus(16,1));
      item.Field(dabc::prop_content_hash).SetInt(h2->GetEntries());
   }
#endif


//#ifdef DABC_EXTRA_CHECKS
//   dabc::Object::DebugObject();
//#endif

}



int fesa::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetBinary")) {

      DOUT0("Process getbinary");

      std::string itemname = cmd.GetStdStr("Item");

      dabc::Hierarchy item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("No find item %s to get binary", itemname.c_str());
         return dabc::cmd_false;
      }

      dabc::Buffer buf;

      if (cmd.GetBool("history"))
         buf = item.ExecuteHistoryRequest(cmd);
      else {
         std::string kind = item.Field(dabc::prop_kind).AsStdStr();
         if (kind.find("ROOT.")==0) {
#ifdef WITH_ROOT
            dabc_root::BinaryProducer* pr = (dabc_root::BinaryProducer*) fProducer();
            if (itemname=="StreamerInfo")
               buf = pr->GetStreamerInfoBinary();
            else
               buf = pr->GetBinary((TH2I*)fHist);

            cmd.SetInt("MasterHash", pr->GetStreamerInfoHash());
#endif
         } else
            buf = item()->bindata();
      }

      if (buf.null()) {
         EOUT("No find binary data for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      cmd.SetRawData(buf);

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


void fesa::Player::BuildWorkerHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects

   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy(cont).Field(dabc::prop_binary_producer).SetStr(ItemName());

   fHierarchy()->BuildHierarchy(cont);
}


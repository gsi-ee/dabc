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

#include <cstdlib>
#include <cmath>

#include "dabc/Publisher.h"

#ifdef WITH_FESA
#include <cmw-rda/RDAService.h>
#include <cmw-rda/Config.h>
#include <cmw-rda/DeviceHandle.h>
#include <cmw-rda/ReplyHandler.h>
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



fesa::Player::Player(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fCounter(0),
   fSniffer(0),
   fHist(0),
   fRDAService(0),
   fDevice(0)
{
   fWorkerHierarchy.Create("FESA", true);

   // this is just emulation, later one need list of real variables
   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("BeamProfile");
   item.SetField(dabc::prop_kind, "FESA.2D");
   item.SetField("_autoload", "dabcsys/plugins/fesa/fesa.js");
   item.SetField("_can_draw", true);


   fWorkerHierarchy.CreateHChild("BeamRate").SetField(dabc::prop_kind, "rate");

   item = fWorkerHierarchy.CreateHChild("BeamRate2");
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

   CreateTimer("update", 1.);

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

   Publish(fWorkerHierarchy, "FESA/Player");
}

fesa::Player::~Player()
{
}

void fesa::Player::ProcessTimerEvent(unsigned)
{
//   DOUT0("Process timer");

   fCounter++;

   dabc::Buffer buf = dabc::Buffer::CreateBuffer(sizeof(fesa::BeamProfile));

   fesa::BeamProfile* rec = (fesa::BeamProfile*) buf.SegmentPtr();
   rec->fill(fCounter % 7);

   dabc::Buffer buf2;

   if (fCounter == 10) {
      DOUT0("Create BeamProfile2 10 seconds later");
      // this is just emulation, later one need list of real variables
      dabc::LockGuard lock(fWorkerHierarchy.GetHMutex());
      dabc::Hierarchy item2 = fWorkerHierarchy.CreateHChild("BeamProfile2");
      item2.SetField(dabc::prop_kind, "FESA.2D");
      item2.SetField("_autoload", "dabcsys/plugins/fesa/fesa.js");
      item2.SetField("_can_draw", true);
   }

   if (fCounter > 10) {
      buf2 = dabc::Buffer::CreateBuffer(sizeof(fesa::BeamProfile));
      fesa::BeamProfile *rec2 = (fesa::BeamProfile*) buf2.SegmentPtr();
      rec2->fill((fCounter+4) % 7);
   }


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


   if (fCounter > 10) {
      // fill also second profile
      dabc::Hierarchy item2 = fWorkerHierarchy.GetHChild("BeamProfile2");
      item2()->bindata() = buf2;
      item2.SetField(dabc::prop_hash, fCounter);
   }

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

double fesa::Player::doGet(const std::string &service, const std::string &field)
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
        EOUT("Uncknown exception caught in doGet");
   }
   #else
   (void) service;
   (void) field;
   #endif
   return res;
}


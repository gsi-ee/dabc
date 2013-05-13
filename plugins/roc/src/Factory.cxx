/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#include "roc/Factory.h"

#include "roc/CombinerModule.h"
#include "roc/ReadoutModule.h"
#include "roc/ReadoutApplication.h"
#include "roc/UdpDevice.h"
#include "roc/NxCalibrModule.h"

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Url.h"

#include "mbs/LmdInput.h"
#include "roc/FileInput.h"

dabc::FactoryPlugin rocfactory(new roc::Factory("roc"));



roc::Factory::Factory(const std::string& name) :
   dabc::Factory(name),
   base::BoardFactory(),
   fDevs()
{
}

dabc::Application* roc::Factory::CreateApplication(const std::string& classname, dabc::Command cmd)
{
   if (classname == xmlReadoutAppClass)
      return new roc::ReadoutApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* roc::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname =="roc::CombinerModule")
      return new roc::CombinerModule(modulename, cmd);

   if (classname == "roc::ReadoutModule")
      return new roc::ReadoutModule(modulename, cmd);

   if (classname == "roc::NxCalibrModule")
      return new roc::NxCalibrModule(modulename, cmd);

   return 0;
}

dabc::Device* roc::Factory::CreateDevice(const std::string& classname, const std::string& devname, dabc::Command cmd)
{
   const char* thrdname = cmd.Field("Thread").AsStr();

   if (classname == roc::typeUdpDevice) {
      DOUT2("roc::Factory::CreateDevice - Creating ROC UdpDevice %s ...", devname.c_str());

      roc::UdpDevice* dev = new roc::UdpDevice(devname, thrdname, cmd);

      if (!dev->IsConnected()) {
         dabc::Object::Destroy(dev);
         return 0;
      }

      return dev;
   }

   return 0;
}


void roc::Factory::ShowDebug(int lvl, const char* msg)
{
   if (lvl<0) EOUT(msg);
         else DOUT0(msg);
}

bool roc::Factory::IsFactoryFor(const char* name)
{
   if (roc::Board::IsLmdFileAddress(name)) return true;

   if (roc::Board::IsUdpAddress(name)) return true;

   return false;
}


base::Board* roc::Factory::DoConnect(const char* name, base::ClientRole role)
{
   if (dabc::mgr.null()) {
      if (!dabc::Factory::CreateManager("dabc")) return 0;
      dabc::SetDebugLevel(0);
      dabc::lgr()->SetLogLimit(1000000);
      dabc::lgr()->SetDebugMask(dabc::lgr()->GetDebugMask() & ~dabc::Logger::lTStamp);
      dabc::Manager::SetAutoDestroy(true);
   }

   if (roc::Board::IsLmdFileAddress(name)) {

      dabc::Url url(name);

      int selectroc = url.GetOptionInt("roc", -1);

      for (int n=0;(n<255) && (selectroc<0);n++) {
         std::string rname = dabc::format("roc%d",n);
         if (url.HasOption(rname)) selectroc = n;
      }

      DOUT0("Creating LMD-file input %s selectroc = %d", url.GetFullName().c_str(), selectroc);

      mbs::LmdInput* inp = new mbs::LmdInput(url);

      if (!inp->Read_Init(0,0)) {
         EOUT("Cannot create LMD input %s", url.GetFullName().c_str());
         delete inp;
         return 0;
      }

      return new roc::FileInput(inp, selectroc);
   }

   if (!roc::Board::IsUdpAddress(name)) return 0;

   static int brdcnt = 0;

   std::string devname = dabc::format("/roc/Udp%d", brdcnt++);
   if (!dabc::mgr.FindDevice(devname).null()) {
      EOUT("Device with name %s already exists!!!", devname.c_str());
      return 0;
   }

   DOUT3("Create device");

   std::string brdname = name;

   dabc::Url url(name);

   if (url.IsValid()) {

      brdname = url.GetHostName();
      if (url.GetPort()>0) {
         brdname += ":";
         brdname += url.GetPortStr();
      }

      if (url.HasOption(roleToString(base::roleDAQ))) role = base::roleDAQ; else
      if (url.HasOption(roleToString(base::roleControl)))  role = base::roleControl; else
      if (url.HasOption(roleToString(base::roleObserver))) role = base::roleObserver;
   }

   if (role==base::roleNone) role = base::roleObserver;

   dabc::CmdCreateDevice cmd(typeUdpDevice, devname);
   cmd.SetStr("Thread", devname);
   cmd.SetStr(roc::xmlBoardAddr, brdname);
   cmd.SetStr(roc::xmlRole, base::roleToString(role));
   if (!dabc::mgr.Execute(cmd)) return 0;

   UdpDeviceRef dev = dabc::mgr.FindDevice(devname);

   DOUT3("Create device dev = %p", dev());

   if (dev.null()) {
      EOUT("UDP Device for board %s cannot be created!!!", brdname.c_str());
      return 0;
   }

   if (!dev()->InitAsBoard()) {
      dev.Destroy();
      return 0;
   }

   roc::Board* brd = static_cast<roc::Board*> (dev());

   fDevs.Add(dev);

   return brd;

}

bool roc::Factory::DoClose(base::Board* brd)
{
   roc::FileInput* inp = dynamic_cast<roc::FileInput*> (brd);
   if (inp!=0) {
      delete inp;
      return true;
   }

   if (dabc::mgr.null()) return false;

   for (unsigned n=0;n<fDevs.GetSize();n++) {
      UdpDevice* dev = (UdpDevice*) fDevs.GetObject(n);

      if (static_cast<roc::Board*>(dev) == brd) {

         dev->CloseAsBoard();
         dev->Execute(dabc::Command(roc::UdpDevice::CmdPutDisconnect()).SetTimeout(4.));
         fDevs.TakeRef(n).Destroy();

         return true;
      }
   }


   EOUT("Board::Close FAIL - brd %p not registered. Check if board closed second time", brd);
   return false;
}

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

#include "verbs/Factory.h"

#include "dabc/Manager.h"

#include "verbs/Device.h"
#include "verbs/Thread.h"

dabc::FactoryPlugin verbsfactory(new verbs::Factory("verbs"));

dabc::Reference verbs::Factory::CreateObject(const std::string &, const std::string &, dabc::Command)
{
   return nullptr;
}

dabc::Device* verbs::Factory::CreateDevice(const std::string &classname,
                                           const std::string &devname,
                                           dabc::Command)
{
   if (classname == verbs::typeDevice) {
      DOUT1("Creating verbs device");
      return new verbs::Device(devname);
   }

   return nullptr;
}

dabc::Reference verbs::Factory::CreateThread(dabc::Reference parent, const std::string &classname, const std::string &thrdname, const std::string &thrddev, dabc::Command cmd)
{
   if (classname != verbs::typeThread) return dabc::Reference();

   if (thrddev.empty()) {
      EOUT("Device name not specified to create verbs thread");
      return dabc::Reference();
   }

   verbs::DeviceRef dev = dabc::mgr.FindDevice(thrddev);

   if (dev.null()) {
      EOUT("Did not found verbs device with name %s", thrddev.c_str());
      return dabc::Reference();
   }

   verbs::Thread* thrd = new verbs::Thread(parent, thrdname, cmd, dev()->IbContext());

   return dabc::Reference(thrd);
}

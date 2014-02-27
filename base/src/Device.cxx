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

#include "dabc/Device.h"

#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Transport.h"


dabc::Device::Device(const std::string& name) :
   Worker(MakePair(name)),
   fDeviceDestroyed(false)
{
   DOUT3("Device %p %s %s constructor prior:%d", this, GetName(), ItemName().c_str(), WorkerPriority());
}

dabc::Device::~Device()
{
   DOUT3("Device %s destr prior:%d", GetName(), WorkerPriority());
}


void dabc::Device::ObjectCleanup()
{
   fDeviceDestroyed = true;

   dabc::Worker::ObjectCleanup();
}


int dabc::Device::ExecuteCommand(Command cmd)
{
   if (cmd.IsName(CmdCreateTransport::CmdName())) {
      CmdCreateTransport crcmd = cmd;

      PortRef port = dabc::mgr.FindPort(crcmd.PortName());

      if (port.null()) return cmd_false;

      TransportRef tr = CreateTransport(cmd, port);

      DOUT1("Device %s creates transport %p", GetName(), tr());

      if (tr.null()) return cmd_false;

      std::string thrdname = port.Cfg(xmlThreadAttr, cmd).AsStr();
      if (thrdname.empty()) thrdname = ThreadName();

      DOUT1("Device %s Create thread %s for transport", GetName(), thrdname.c_str());

      // set reference on device
      tr()->fTransportDevice = this;

      if (!tr.MakeThreadForWorker(thrdname)) {
         EOUT("Fail to create thread for transport");
         tr.Destroy();
         return cmd_false;
      }

      tr.ConnectPoolHandles();
      if (port.IsInput())
         dabc::LocalTransport::ConnectPorts(tr.OutputPort(), port);
      if (port.IsOutput())
         dabc::LocalTransport::ConnectPorts(port, tr.InputPort());

      DOUT1("Device %s transport is created for port %p %s", GetName(), port(), port.ItemName().c_str());

      return cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


bool dabc::Device::Find(ConfigIO &cfg)
{
   DOUT4("Device::Find %p name = %s parent %p", this, GetName(), GetParent());

   if (GetParent()==0) return false;

   // module will always have tag "Device", class could be specified with attribute
   while (cfg.FindItem(xmlDeviceNode)) {
      if (cfg.CheckAttr(xmlNameAttr, GetName())) return true;
   }

   return false;
}


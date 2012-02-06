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
#include "dabc/Port.h"

dabc::Device::Device(const char* name) :
   Worker(MakePair(name), true),
   fDeviceMutex(true)
{
   DOUT3(("Device %p %s %s constructor prior:%d", this, GetName(), ItemName().c_str(), WorkerPriority()));
}

dabc::Device::~Device()
{
   // by this call we synchronise us with anything else
   // that can happen on the device

   DOUT3(("Device %s destr prior:%d", GetName(), WorkerPriority()));
}

int dabc::Device::ExecuteCommand(Command cmd)
{
   if (cmd.IsName(CmdCreateTransport::CmdName())) {

      Reference portref = dabc::mgr.FindPort(cmd.GetStdStr("PortName"));
      Port* port = (Port*) portref();
      if (port==0) return cmd_false;

      portref.SetTransient(false);
      Transport* tr = CreateTransport(cmd, portref);
      if (tr==0) return cmd_false;

      Reference trhandle(tr->GetWorker());

      std::string newthrdname = port->Cfg(xmlTrThread, cmd).AsStdStr(ThreadName());

      if (dabc::mgr()->MakeThreadFor(tr->GetWorker(), newthrdname.c_str())) {
         port->AssignTransport(trhandle, tr);
         return cmd_true;
      } else {
         EOUT(("No thread for transport"));
         trhandle.Destroy();
      }

      return cmd_false;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

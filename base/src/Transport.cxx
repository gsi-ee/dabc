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

#include "dabc/Transport.h"

#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"

const unsigned dabc::AcknoledgeQueueLength = 2;


std::string dabc::Transport::MakeName(const PortRef& inpport, const PortRef& outport)
{
   std::string name = inpport.ItemName();
   if (name.empty()) name = outport.ItemName();
   name += "_Transport";

   size_t pos;
   while ((pos=name.find("/")) != name.npos)
      name[pos] = '_';

   return std::string("#") + name;
}



dabc::Transport::Transport(dabc::Command cmd, const PortRef& inpport, const PortRef& outport) :
   ModuleAsync(MakeName(inpport, outport)),
   fState(stInit),
   fIsInputTransport(false),
   fIsOutputTransport(false)
{
   std::string poolname;

   if (!inpport.null()) {
      poolname = inpport.Cfg(dabc::xmlPoolName, cmd).AsStdStr(poolname);

//      DOUT0("DataTransport %s creates port Output pool %s", ItemName().c_str(), poolname.c_str());

      CreateOutput("Output", inpport.QueueCapacity());

      fIsInputTransport = true;
   }

   if (!outport.null()) {

      poolname = outport.Cfg(dabc::xmlPoolName, cmd).AsStdStr(poolname);

//      DOUT0("DataTransport %s creates port Input pool %s", ItemName().c_str(), poolname.c_str());

      CreateInput("Input", outport.QueueCapacity());

      fIsOutputTransport = true;
   }

   if (fIsInputTransport && poolname.empty()) poolname = dabc::xmlWorkPool;

   if (!poolname.empty()) {
//      DOUT0("Create hadnle in transport %s isinp:%s isout:%s", ItemName().c_str(), DBOOL(fIsInputTransport), DBOOL(fIsOutputTransport));

      // TODO: one should be able to configure if transport use pool requests or not
      CreatePoolHandle(poolname);
   }
}

dabc::Transport::~Transport()
{
}


void dabc::Transport::ProcessConnectionActivated(const std::string& name, bool on)
{
   // ignore connect/disconnect from pool handles
   if (IsValidPool(FindPool(name))) return;

   if (on) {
      if ((GetState()==stInit) || (GetState()==stStopped)) {
         DOUT0("Connection %s activated by transport %s - make start", name.c_str(), GetName());

         if (StartTransport())
            fState = stRunning;
         else {
            fState = stError;
            DeleteThis();
         }
      } else {
         DOUT0("dabc::Transport %s is running, ignore start message from port %s", GetName(), name.c_str());
      }

   } else {
      if (GetState()==stRunning) {
         if (StopTransport())
           fState = stStopped;
         else {
           fState = stError;
           DeleteThis();
         }
      }
   }
}

void dabc::Transport::ProcessConnectEvent(const std::string& name, bool on)
{
   // ignore connect event
   if (on) return;

   if (IsInputTransport() && (name == OutputName())) {
      DOUT2("Transport %s port %s is disconnected - automatic transport destroyment is started", GetName(), name.c_str());
      DeleteThis();
      return;
   }

   if (IsOutputTransport() && (name == InputName())) {
      DOUT2("Transport %s port %s is disconnected - automatic transport destroyment is started", GetName(), name.c_str());
      DeleteThis();
      return;
   }

}

void dabc::Transport::CloseTransport(bool witherr)
{
   DeleteThis();
}

int dabc::Transport::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("CloseTransport")) {
      CloseTransport(cmd.GetBool("IsError", true));
      return cmd_true;
   }

   return ModuleAsync::ExecuteCommand(cmd);
}


bool dabc::Transport::StartTransport()
{
   DOUT0("Start transport %s", GetName());

   fAddon.Notify("StartTransport");
   return Start();
}

bool dabc::Transport::StopTransport()
{
   fAddon.Notify("StopTransport");
   return Stop();
}



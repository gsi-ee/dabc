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
   fTransportState(stInit),
   fIsInputTransport(false),
   fIsOutputTransport(false),
   fTransportInfoName(),
   fTransportInfoInterval(-1.),
   fTransportInfoTm()
{
   std::string poolname;

   if (!inpport.null()) {
      poolname = inpport.Cfg(dabc::xmlPoolName, cmd).AsStr(poolname);

//      DOUT0("Inpport %s pool %s", inpport.ItemName(false).c_str(), poolname.c_str());

      CreateOutput("Output", inpport.QueueCapacity());

      fTransportInfoName = inpport.InfoParName();

      fIsInputTransport = true;
   }

   if (!outport.null()) {

      poolname = outport.Cfg(dabc::xmlPoolName, cmd).AsStr(poolname);

//      DOUT0("Outport %s pool %s cmdhaspool %s", outport.ItemName(false).c_str(), poolname.c_str(), DBOOL(cmd.HasField(dabc::xmlPoolName)));

      CreateInput("Input", outport.QueueCapacity());

      if (fTransportInfoName.empty())
         fTransportInfoName = outport.InfoParName();

      fIsOutputTransport = true;
   }

   if (!fTransportInfoName.empty()) {
      fTransportInfoInterval = 1;
      fTransportInfoTm.GetNow();
   }


   if (fIsInputTransport && poolname.empty()) poolname = dabc::xmlWorkPool;

//   DOUT0("Create transport inp %s out %s poolname %s", DBOOL(fIsInputTransport), DBOOL(fIsOutputTransport), poolname.c_str());

   if (!poolname.empty() && (NumPools()==0)) {
      // TODO: one should be able to configure if transport use pool requests or not

      // for output transport one not need extra memory, just link to pool for special cases like verbs
      CreatePoolHandle(poolname, fIsInputTransport ? 10 : 0);
   }
}

dabc::Transport::~Transport()
{
}

bool dabc::Transport::InfoExpected() const
{
   if (fTransportInfoName.empty() || (fTransportInfoInterval<=0)) return false;

   return fTransportInfoTm.Expired(fTransportInfoInterval);
}

void dabc::Transport::ProvideInfo(int lvl, const std::string& info)
{
   if (fTransportInfoName.empty()) return;

   InfoParameter par = dabc::mgr.FindPar(fTransportInfoName);

   fTransportInfoTm.GetNow();

   if (par.null()) return;

   par.SetInfo(info);
}


void dabc::Transport::ProcessConnectionActivated(const std::string& name, bool on)
{
   // ignore connect/disconnect from pool handles
   if (IsValidPool(FindPool(name))) return;

   if (on) {
      if ((GetTransportState()==stInit) || (GetTransportState()==stStopped)) {
         DOUT2("Connection %s activated in transport %s - start it", name.c_str(), GetName());

         if (StartTransport()) {
            fTransportState = stRunning;
         } else {
            fTransportState = stError;
            DeleteThis();
         }
      } else {
         DOUT2("Transport %s is running, ignore start message from port %s", GetName(), name.c_str());
      }

   } else {
      if (GetTransportState()==stRunning) {
         if (StopTransport()) {
           fTransportState = stStopped;
         } else {
           fTransportState = stError;
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
   DOUT2("Start transport %s", GetName());

   fAddon.Notify("StartTransport");
   return Start();
}

bool dabc::Transport::StopTransport()
{
   DOUT2("Stop transport %s", GetName());

   fAddon.Notify("StopTransport");
   return Stop();
}



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

#include "dabc/MultiplexerModule.h"

dabc::MultiplexerModule::MultiplexerModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fQueue(100),
   fDataRateName()
{
   EnsurePorts(1, 1);

   fDataRateName = Cfg("DataRateName", cmd).AsStr();

   if (!fDataRateName.empty())
      CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");

   for (unsigned n=0;n<NumInputs();n++) {
      SetPortSignaling(InputName(n), dabc::Port::SignalEvery);
      if (!fDataRateName.empty())
         SetPortRatemeter(InputName(n), Par(fDataRateName));
   }

   DOUT0("Create multiplexer module %s with %u inputs and %u outputs", name.c_str(), NumInputs(), NumOutputs());
}

void dabc::MultiplexerModule::ProcessInputEvent(unsigned port)
{
   fQueue.Push(port);

   if (port >= NumInputs())
      EOUT("MultiplexerModule %s process inpevent from not existing input %u total %u", GetName(), port, NumInputs());

   CheckDataSending();
}

void dabc::MultiplexerModule::ProcessOutputEvent(unsigned)
{
   CheckDataSending();
}

void dabc::MultiplexerModule::CheckDataSending()
{
//   DOUT0("MultiplexerModule %s CheckDataSending queue %u canallsend:%s numout %u",
//         GetName(), fQueue.Size(), DBOOL(CanSendToAllOutputs()), NumOutputs());

   while (CanSendToAllOutputs() && (fQueue.Size() > 0)) {
      unsigned id = fQueue.Pop();

      if (id >= NumInputs()) {
         EOUT("MultiplexerModule %s wrong input port id %u, use 0", GetName(), id);
         id = 0;
      }

      dabc::Buffer buf = Recv(id);

      if (buf.null())
         EOUT("Fail to get buffer from input %u", id);
//      else
//         DOUT0("Sending buffer %u to all outputs", buf.GetTotalSize());

      SendToAllOutputs(buf);
   }
}

// ========================================================================================



dabc::RepeaterModule::RepeaterModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   EnsurePorts(1, 1);

   std::string ratemeter = Cfg("DataRateName", cmd).AsStr();

   if (!ratemeter.empty()) {
      CreatePar(ratemeter).SetRatemeter(false, 3.).SetUnits("MB");

      for (unsigned n=0;n<NumInputs();n++)
         SetPortRatemeter(InputName(n), Par(ratemeter));
   }

   DOUT1("Create dabc::RepeaterModule:: %s", GetName());
}

bool dabc::RepeaterModule::ProcessRecv(unsigned port)
{
   bool isout = (port < NumOutputs());

   if (isout && !CanSend(port)) return false;

   dabc::Buffer buf = Recv(port);
   if (isout) Send(port, buf);
         else buf.Release();

   return true;
}

bool dabc::RepeaterModule::ProcessSend(unsigned port)
{
   if (!CanRecv(port)) return false;

   dabc::Buffer buf = Recv(port);
   Send(port, buf);

   return true;
}

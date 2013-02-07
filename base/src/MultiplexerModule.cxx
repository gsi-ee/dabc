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

#include "dabc/Port.h"


dabc::MultiplexerModule::MultiplexerModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fQueue(100),
   fDataRateName()
{
   std::string poolname = Cfg(dabc::xmlPoolName, cmd).AsStdStr("Pool");

   CreatePoolHandle(poolname.c_str());

   int numinp = Cfg(dabc::xmlNumInputs, cmd).AsInt(1);
   int numout = Cfg(dabc::xmlNumOutputs, cmd).AsInt(1);
   fDataRateName = Cfg("DataRateName", cmd).AsStdStr();

   for (int n=0;n<numinp;n++)
      CreateInput(dabc::format(dabc::xmlInputMask, n).c_str(), Pool(), 10);

   for (int n=0;n<numout;n++)
      CreateOutput(dabc::format(dabc::xmlOutputMask, n).c_str(), Pool(), 10);

   if (!fDataRateName.empty())
      CreatePar(fDataRateName).SetRatemeter(false, 3.).SetUnits("MB");
   
   DOUT0(("Create multiplexer module %s with %d inputs and %d outputs", name, numinp, numout));
   
}

void dabc::MultiplexerModule::ProcessInputEvent(Port* port)
{
   unsigned id = InputNumber(port);
   fQueue.Push(id);

   CheckDataSending();
}

void dabc::MultiplexerModule::ProcessOutputEvent(Port* port)
{
   CheckDataSending();
}

void dabc::MultiplexerModule::CheckDataSending()
{
   while (CanSendToAllOutputs() && (fQueue.Size()>0)) {
      unsigned id = fQueue.Pop();

      dabc::Buffer buf = Input(id)->Recv();

      if (buf.null()) EOUT(("Fail to get buffer from input %u", id));

      if (!fDataRateName.empty())
         Par(fDataRateName).SetDouble(buf.GetTotalSize()/1024./1024.);

      SendToAllOutputs(buf);
   }
}


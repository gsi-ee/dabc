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

#include "hadaq/SorterModule.h"

#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"


hadaq::SorterModule::SorterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fFlushCnt(0)
{
   // we need at least one input and one output port
   EnsurePorts(1, 1, dabc::xmlWorkPool);

   double flushtime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(3);

   if (flushtime > 0.)
      CreateTimer("FlushTimer", flushtime, false);

   fFlushCnt = 2;
}


bool hadaq::SorterModule::retransmit()
{
   // method performs reads at maximum one event from input and
   // push it with MBS header to output

   if (CanSend() && CanRecv()) {
      dabc::Buffer buf = Recv();
      Send(buf);
      fFlushCnt = 2;
      return true;
   }

   return false;
}


void hadaq::SorterModule::ProcessTimerEvent(unsigned)
{
   if (fFlushCnt>0)
      fFlushCnt--;
   else {
      // flush buffer
   }
}

int hadaq::SorterModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

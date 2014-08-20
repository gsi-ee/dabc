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

#include "hadaq/TerminalModule.h"

#include <stdlib.h>

#include "dabc/Manager.h"

#include "hadaq/CombinerModule.h"
#include "hadaq/UdpTransport.h"


hadaq::TerminalModule::TerminalModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   double period = Cfg("period", cmd).AsDouble(1);

   fDoClear = Cfg("clear", cmd).AsBool(false);

   CreateTimer("update", period, false);

   fLastTm.Reset();
}

void hadaq::TerminalModule::BeforeModuleStart()
{
   dabc::SetDebugLevel(-1);
   if (fDoClear) system("clear");
}


void hadaq::TerminalModule::ProcessTimerEvent(unsigned timer)
{
   dabc::ModuleRef m = dabc::mgr.FindModule("Combiner");

   hadaq::CombinerModule* comb = dynamic_cast<hadaq::CombinerModule*> (m());
   if (comb==0) return;

   double delta = fLastTm.SpentTillNow(true);

   double rate1(0.), rate2(0.), rate3(0), rate4(0);

   if (delta>0) {
      for (unsigned n=0;n<comb->fCfg.size()+2;n++)
         fputs("\033[A\033[2K",stdout);
      rewind(stdout);
      ftruncate(1,0);

      rate1 = (comb->fTotalBuildEvents - fTotalBuildEvents) / delta;
      rate2 = (comb->fTotalRecvBytes - fTotalRecvBytes) / delta;
      rate3 = (comb->fTotalDiscEvents - fTotalDiscEvents) / delta;
      rate4 = (comb->fTotalDroppedData - fTotalDroppedData) / delta;
   }

   fTotalBuildEvents = comb->fTotalBuildEvents;
   fTotalRecvBytes = comb->fTotalRecvBytes;
   fTotalDiscEvents = comb->fTotalDiscEvents;
   fTotalDroppedData = comb->fTotalDroppedData;

   fprintf(stdout, "Events: %5lu   Rate: %5.1f ev/s  Data: %10s  Rate:%6.3f MB/s\n",
         (long unsigned) fTotalBuildEvents, rate1,
         dabc::size_to_str(fTotalRecvBytes).c_str(), rate2/1024./1024.);
   fprintf(stdout, "Lost:   %5lu   Rate: %5.1f ev/s  Data: %10s  Rate:%6.3f MB/s\n",
         (long unsigned) fTotalDiscEvents, rate3,
         dabc::size_to_str(fTotalDroppedData).c_str(), rate4/1024./1024.);

   for (unsigned n=0;n<comb->fCfg.size();n++) {
      fprintf(stdout,"inp:%2u", n);
      hadaq::DataSocketAddon* addon = (hadaq::DataSocketAddon*) comb->fCfg[n].fAddon;

      if (addon==0) { fprintf(stdout,"  addon:null\n"); continue; }

      fprintf(stdout, "  port:%5d pkt:%6lu data:%9s disc:%4lu data:%9s err32:%4lu  buf:%5lu queue:%2d\n",
            addon->fNPort,
            (long unsigned) addon->fTotalRecvPacket,
            dabc::size_to_str(addon->fTotalRecvBytes).c_str(),
            (long unsigned) addon->fTotalDiscardPacket,
            dabc::size_to_str(addon->fTotalDiscardBytes).c_str(),
            (long unsigned) addon->fTotalDiscard32Packet,
            (long unsigned) addon->fTotalProducedBuffers,
            comb->fCfg[n].fNumCanRecv);
   }
}

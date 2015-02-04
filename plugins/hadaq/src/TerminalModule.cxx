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
#include <unistd.h>
#include <sys/types.h>

#include "dabc/Manager.h"

#include "hadaq/CombinerModule.h"
#include "hadaq/UdpTransport.h"
#include "hadaq/TdcCalibrationModule.h"

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
      for (unsigned n=0;n<comb->fCfg.size()+3;n++)
         fputs("\033[A\033[2K",stdout);
      rewind(stdout);
      ftruncate(1,0);

      rate1 = (comb->fTotalBuildEvents - fTotalBuildEvents) / delta;
      rate2 = (comb->fTotalRecvBytes - fTotalRecvBytes) / delta;
      rate3 = (comb->fTotalDiscEvents - fTotalDiscEvents) / delta;
      rate4 = (comb->fTotalDroppedData - fTotalDroppedData) / delta;
   } else {
      printf("HADAQ terminal info:\n");
      printf("  disc - data discarded due to header error\n");
      printf("  err32 - crc32 error\n");
      printf("  bufs  - number of produced buffers\n");
      printf("  qu    - input queue of combiner module\n");
      printf("  lost  - lost subevents (recognized by combiner)\n");
      printf("  progr - progress of TDC calibration\n");
   }

   fTotalBuildEvents = comb->fTotalBuildEvents;
   fTotalRecvBytes = comb->fTotalRecvBytes;
   fTotalDiscEvents = comb->fTotalDiscEvents;
   fTotalDroppedData = comb->fTotalDroppedData;

   fprintf(stdout, "Events:%7lu   Rate:%7.1f ev/s  Data: %10s  Rate:%6.3f MB/s\n",
         (long unsigned) fTotalBuildEvents, rate1,
         dabc::size_to_str(fTotalRecvBytes).c_str(), rate2/1024./1024.);
   fprintf(stdout, "Lost:  %7lu   Rate:%7.1f ev/s  Data: %10s  Rate:%6.3f MB/s\n",
         (long unsigned) fTotalDiscEvents, rate3,
         dabc::size_to_str(fTotalDroppedData).c_str(), rate4/1024./1024.);

   fprintf(stdout, "inp port     pkt      data disc err32  bufs qu lost    TRB         TDC        progr state\n");

   for (unsigned n=0;n<comb->fCfg.size();n++) {

      std::string sbuf = dabc::format("%2u", n);

      hadaq::DataSocketAddon* addon = (hadaq::DataSocketAddon*) comb->fCfg[n].fAddon;

      if (addon==0) {
         sbuf.append("  missing addon ");
      } else {
         sbuf.append(dabc::format(" %5d %7lu %9s %4lu %5lu %5lu",
               addon->fNPort,
               (long unsigned) addon->fTotalRecvPacket,
               dabc::size_to_str(addon->fTotalRecvBytes).c_str(),
               (long unsigned) addon->fTotalDiscardPacket,
               (long unsigned) addon->fTotalDiscard32Packet,
               (long unsigned) addon->fTotalProducedBuffers));
      }

      sbuf.append(dabc::format(" %2d %4u",comb->fCfg[n].fNumCanRecv, comb->fCfg[n].fLostTrig));

      TdcCalibrationModule* cal = (TdcCalibrationModule*) comb->fCfg[n].fCalibr;

      if (cal) {
         sbuf.append(dabc::format(" 0x%04x", cal->fTRB));

         std::string tdc = " [";
         for (unsigned j=0;j<cal->fTDCs.size();j++) {
            if (j>0) tdc.append(",");
            tdc.append(dabc::format("%04x", cal->fTDCs[j]));
         }
         tdc.append("]");
         while (tdc.length()<22) tdc.append(" ");
         sbuf.append(tdc);

         sbuf.append(dabc::format(" %2d %s", cal->fProgress, cal->fState.c_str()));
      }

      fprintf(stdout, sbuf.c_str());
      fprintf(stdout, "\n");
   }
}

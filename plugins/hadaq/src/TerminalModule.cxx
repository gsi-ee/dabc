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
#include <vector>

#include "dabc/Manager.h"

#include "hadaq/CombinerModule.h"
#include "hadaq/UdpTransport.h"

hadaq::TerminalModule::TerminalModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fTotalBuildEvents(0),
   fTotalRecvBytes(0),
   fTotalDiscEvents(0),
   fTotalDroppedData(0),
   fDoClear(false),
   fLastTm(),
   fCalibr(),
   fFilePort(1),
   fLastFileCmd(),
   fFileReqRunning(false)
{
   double period = Cfg("period", cmd).AsDouble(1);
   fFilePort = Cfg("fileport", cmd).AsInt(1);

   fDoClear = Cfg("clear", cmd).AsBool(false);
   fRingSize = Cfg("showtrig", cmd).AsInt(0);
   if (fRingSize > HADAQ_RINGSIZE) fRingSize = HADAQ_RINGSIZE;

   CreateTimer("update", period, false);

   fLastTm.Reset();

   fWorkerHierarchy.Create("Term");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("State");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "Init");

   item = fWorkerHierarchy.CreateHChild("Output");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "");

   item = fWorkerHierarchy.CreateHChild("Data");
   item.SetField("value", "");
   item.SetField("_hidden", "true");

   Publish(fWorkerHierarchy, "$CONTEXT$/Terminal");
}

bool hadaq::TerminalModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetTransportStatistic")) {
      fFileReqRunning = false;
      if (cmd.GetResult() == dabc::cmd_true) fLastFileCmd = cmd;
                                        else fLastFileCmd.Release();
      return true;
   }

   if (!cmd.IsName("GetCalibrState")) return true;

   unsigned n = cmd.GetUInt("indx");
   if (n < fCalibr.size()) {
      fCalibr[n].trb = cmd.GetUInt("trb");
      fCalibr[n].tdcs = cmd.GetField("tdcs").AsUIntVect();
      fCalibr[n].progress = cmd.GetInt("progress");
      fCalibr[n].state = cmd.GetStr("state");
      fCalibr[n].send_request = false;
   }

   return true;
}

void hadaq::TerminalModule::BeforeModuleStart()
{
   dabc::SetDebugLevel(-1);
   if (fDoClear) system("clear");
}

std::string hadaq::TerminalModule::rate_to_str(double r)
{
   if (r<1e4) return dabc::format("%6.1f ev/s",r);
   return dabc::format("%5.1f kev/s",r/1e3);
}

void hadaq::TerminalModule::ProcessTimerEvent(unsigned timer)
{
   dabc::ModuleRef m = dabc::mgr.FindModule("Combiner");

   hadaq::CombinerModule* comb = dynamic_cast<hadaq::CombinerModule*> (m());
   if (comb==0) return;

   double delta = fLastTm.SpentTillNow(true);

   double rate1(0.), rate2(0.), rate3(0), rate4(0);

   if (delta>0) {
      unsigned nlines = comb->fCfg.size()+4;
      if (fFilePort>=0) nlines++;
      for (unsigned n=0;n<nlines;n++)
         fputs("\033[A\033[2K",stdout);
      rewind(stdout);
      ftruncate(1,0);

      rate1 = (comb->fTotalBuildEvents - fTotalBuildEvents) / delta;
      rate2 = (comb->fTotalRecvBytes - fTotalRecvBytes) / delta;
      rate3 = (comb->fTotalDiscEvents - fTotalDiscEvents) / delta;
      rate4 = (comb->fTotalDroppedData - fTotalDroppedData) / delta;
   } else {
      printf("HADAQ terminal info:\n");
      printf("  disc  - all discarded packets in the UDP receiver\n");
      printf("  err32 - 32-byte header does not match with 32-bytes footer\n");
      printf("  bufs  - number of produced buffers\n");
      printf("  qu    - input queue of combiner module\n");
      printf("  drop  - dropped subevents (received by combiner but not useful)\n");
      printf("  lost  - lost subevents (never seen by combiner)\n");
      printf("  progr - progress of TDC calibration\n");
   }

   fTotalBuildEvents = comb->fTotalBuildEvents;
   fTotalRecvBytes = comb->fTotalRecvBytes;
   fTotalDiscEvents = comb->fTotalDiscEvents;
   fTotalDroppedData = comb->fTotalDroppedData;

   std::string s;

   s += "---------------------------------------------\n";
   s += dabc::format("Events:%8s   Rate:%12s  Data: %10s  Rate:%6.3f MB/s\n",
                        dabc::number_to_str(fTotalBuildEvents, 1).c_str(),
                        rate_to_str(rate1).c_str(),
                        dabc::size_to_str(fTotalRecvBytes).c_str(), rate2/1024./1024.);
   s += dabc::format("Dropped:%7s   Rate:%12s  Data: %10s  Rate:%6.3f MB/s",
                        dabc::number_to_str(fTotalDiscEvents, 1).c_str(),
                        rate_to_str(rate3).c_str(),
                        dabc::size_to_str(fTotalDroppedData).c_str(), rate4/1024./1024.);

   if (comb->fTotalFullDrops>0)
      s += dabc::format(" Total:%s\n", dabc::size_to_str(comb->fTotalFullDrops, 1).c_str());
   else
      s += "\n";


   if (fFilePort>=0) {
      if (fLastFileCmd.null()) {
         s += dabc::format("File: missing, failed or not found on Combiner/Output%d\n", fFilePort);
      } else {
         std::string state = fLastFileCmd.GetStr("OutputState");
         if (state!="Ready") state = std::string(" State: ") + state;
                        else state.clear();
         s += dabc::format("File:  %8s   Curr:  %10s  Data: %10s  Name: %s%s\n",
                           dabc::number_to_str(fLastFileCmd.GetDouble("OutputFileEvents"),1).c_str(),
                           dabc::size_to_str(fLastFileCmd.GetDouble("OutputCurrFileSize")).c_str(),
                           dabc::size_to_str(fLastFileCmd.GetDouble("OutputFileSize")).c_str(),
                           fLastFileCmd.GetStr("OutputCurrFileName").c_str(),
                           state.c_str());
      }
   }

   if (comb->fCfg.size() != fCalibr.size())
      fCalibr.resize(comb->fCfg.size(), CalibrRect());

   bool istdccal = false;
   for (unsigned n=0;n<comb->fCfg.size();n++)
      if (comb->fCfg[n].fCalibr.length()>0) {
        istdccal = true;
        if (!fCalibr[n].send_request) {
           dabc::Command cmd("GetCalibrState");
           cmd.SetInt("indx",n);
           cmd.SetReceiver(comb->fCfg[n].fCalibr);
           dabc::mgr.Submit(Assign(cmd));
           fCalibr[n].send_request = true;
        }
      }

   s += "inp port     pkt      data    MB/s   disc  err32   bufs  qu  drop  lost";
   if (istdccal) s += "    TRB         TDC        progr state\n";
            else s += "\n";

   bool isready = true;

   dabc::Hierarchy ditem = fWorkerHierarchy.GetHChild("Data");
   ditem.SetField("BuildEvents", fTotalBuildEvents);
   ditem.SetField("BuildData", fTotalRecvBytes);
   ditem.SetField("EventsRate", rate1);
   ditem.SetField("DataRate", rate2);
   ditem.SetField("LostEvents", fTotalDiscEvents);
   ditem.SetField("LostData", fTotalDroppedData);
   ditem.SetField("LostEventsRate", rate3);
   ditem.SetField("LostDataRate", rate4);

   std::vector<int64_t> ports, recvbytes, inpdrop, inplost;
   std::vector<double> inprates;

   for (unsigned n=0;n<comb->fCfg.size();n++) {

      std::string sbuf = dabc::format("%2u", n);

      hadaq::CombinerModule::InputCfg& cfg = comb->fCfg[n];

      hadaq::DataSocketAddon* addon = (hadaq::DataSocketAddon*) cfg.fAddon;

      if (addon==0) {
         sbuf.append("  missing addon ");
         fCalibr[n].lastrecv = 0;
      } else {

         double rate = (addon->fTotalRecvBytes - fCalibr[n].lastrecv)/delta;

         sbuf.append(dabc::format(" %5d %7s %9s %7.3f %6s %6s %6s",
               addon->fNPort,
               dabc::number_to_str(addon->fTotalRecvPacket,1).c_str(),
               dabc::size_to_str(addon->fTotalRecvBytes).c_str(),
               rate/1024./1024.,
               dabc::number_to_str(addon->fTotalDiscardPacket).c_str(),
               dabc::number_to_str(addon->fTotalDiscard32Packet).c_str(),
               dabc::number_to_str(addon->fTotalProducedBuffers).c_str()));
         fCalibr[n].lastrecv = addon->fTotalRecvBytes;

         ports.push_back(addon->fNPort);
         recvbytes.push_back(addon->fTotalRecvBytes);
         inprates.push_back(rate);
      }

      sbuf.append(dabc::format(" %3d %5s %5s",
                   cfg.fNumCanRecv,
                   dabc::number_to_str(cfg.fDroppedTrig,0).c_str(),
                   dabc::number_to_str(cfg.fLostTrig,0).c_str()));

      inpdrop.push_back(cfg.fDroppedTrig);
      inplost.push_back(cfg.fLostTrig);

      if (cfg.fCalibr.length() > 0) {
         sbuf.append(dabc::format(" 0x%04x", fCalibr[n].trb));

         std::string tdc = " [";
         for (unsigned j=0;j<fCalibr[n].tdcs.size();j++) {
            if (j>0) tdc.append(",");
            tdc.append(dabc::format("%04x", (unsigned) fCalibr[n].tdcs[j]));
         }
         tdc.append("]");
         while (tdc.length()<22) tdc.append(" ");
         sbuf.append(tdc);

         sbuf.append(dabc::format(" %2d %s", fCalibr[n].progress, fCalibr[n].state.c_str()));

         if (fCalibr[n].state.find("Ready")!=0) isready = false;
      }

      s += sbuf;

      if (fRingSize>0) {
         sbuf = "";
         unsigned cnt = cfg.fRingCnt;
         for (int n=0;n<fRingSize;n++) {
            if (cnt>0) cnt--; else cnt = HADAQ_RINGSIZE-1;
            sbuf = dabc::format("0x%06x ",(unsigned)cfg.fTrigNumRing[cnt]) + sbuf;
         }
         s += " trig:" + sbuf;
      }

      s += "\n";
   }

   fprintf(stdout, s.c_str());

   fWorkerHierarchy.GetHChild("State").SetField("value", isready ? "Ready" : "Init");
   fWorkerHierarchy.GetHChild("Output").SetField("value", s);
   ditem.SetField("inputs", ports);
   ditem.SetField("inprecv", recvbytes);
   ditem.SetField("inprates", inprates);
   ditem.SetField("inplost", inplost);
   ditem.SetField("inpdrop", inpdrop);

   if (!fFileReqRunning && (fFilePort>=0)) {
      dabc::Command cmd("GetTransportStatistic");
      cmd.SetStr("_for_the_port_", dabc::format("Output%d", fFilePort));
      m.Submit(Assign(cmd));
   }
}

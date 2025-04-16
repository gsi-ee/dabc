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

#include "dogma/TerminalModule.h"

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <vector>

#include "dabc/Manager.h"

#include "dogma/CombinerModule.h"
#include "dogma/UdpTransport.h"

dogma::TerminalModule::TerminalModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fTotalBuildEvents(0),
   fTotalRecvBytes(0),
   fTotalDiscEvents(0),
   fTotalDroppedData(0),
   fDoClear(false),
   fDoShow(true),
   fLastTm(),
   fCalibr(),
   fServPort(0),
   fLastServCmd(),
   fServReqRunning(false),
   fFilePort(1),
   fLastFileCmd(),
   fFileReqRunning(false)
{
   double period = Cfg("period", cmd).AsDouble(1);
   double slow = Cfg("slow", cmd).AsDouble(-1);
   fServPort = Cfg("servport", cmd).AsInt(-1);
   fFilePort = Cfg("fileport", cmd).AsInt(-1);

   fDoClear = Cfg("clear", cmd).AsBool(false);
   fDoShow = Cfg("show", cmd).AsBool(true);
   fRingSize = Cfg("showtrig", cmd).AsInt(10);

   fModuleName = Cfg("mname", cmd).AsStr("Combiner");
   fFileModuleName = Cfg("mname2", cmd).AsStr("");

   if (fRingSize > DOGMA_RINGSIZE)
      fRingSize = DOGMA_RINGSIZE;

   CreateTimer("update", period);
   if ((slow > 0) && (period > 0) && (slow > 2 * period))
      fDoSlow = (int) slow / period;

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

bool dogma::TerminalModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetTransportStatistic")) {
      if (cmd.GetBool("_file_req")) {
         fFileReqRunning = false;
         if (cmd.GetResult() == dabc::cmd_true) fLastFileCmd = cmd;
                                           else fLastFileCmd.Release();
      } else
      if (cmd.GetBool("_serv_req")) {
         fServReqRunning = false;
         if (cmd.GetResult() == dabc::cmd_true) fLastServCmd = cmd;
                                           else fLastServCmd.Release();
      }
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

void dogma::TerminalModule::BeforeModuleStart()
{
   if (fDoShow) {
      dabc::SetDebugLevel(-1);
      if (fDoClear) {
         auto res = std::system("clear");
         (void) res; // just avoid compiler warnings
      }
   }
}

std::string dogma::TerminalModule::rate_to_str(double r)
{
   if (r<1e4) return dabc::format("%6.1f ev/s",r);
   return dabc::format("%5.1f kev/s",r/1e3);
}

void dogma::TerminalModule::ProcessTimerEvent(unsigned)
{
   dabc::ModuleRef m = dabc::mgr.FindModule(fModuleName);

   auto comb = dynamic_cast<dogma::CombinerModule*> (m());
   if (!comb) return;

   dabc::MemoryPoolRef pool = dabc::mgr.FindPool(dabc::xmlWorkPool);
   if (!pool.null()) {
      fMemorySize = pool.GetTotalSize();
      fMemoryUsage = pool.GetUsedRatio();
   }

   double delta = fLastTm.SpentTillNow(true);

   delta = (delta > 0.001) ? 1./delta : 0.;

   double rate1 = (comb->fAllBuildEvents > fTotalBuildEvents) ? (comb->fAllBuildEvents - fTotalBuildEvents) * delta : 0.,
          rate2 = (comb->fAllRecvBytes > fTotalRecvBytes) ? (comb->fAllRecvBytes - fTotalRecvBytes) * delta : 0.,
          rate3 = (comb->fAllDiscEvents > fTotalDiscEvents) ? (comb->fAllDiscEvents - fTotalDiscEvents) * delta : 0.,
          rate4 = (comb->fAllDroppedData > fTotalDroppedData) ? (comb->fAllDroppedData - fTotalDroppedData) * delta : 0.;

   std::string s,s1,s2,s3,s4;

   if (fDoSlow > 0) {
      auto calc = [this](std::vector<double> &vect, double rate) {
         if (vect.size() >= (unsigned) fDoSlow)
            vect.erase(vect.begin());
         vect.emplace_back(rate);
         double sum = 0.;
         for (auto v : vect)
            sum += v;
         return sum / vect.size();
      };
      double srate1 = calc(fSlowRate1, rate1),
             srate2 = calc(fSlowRate2, rate2),
             srate3 = calc(fSlowRate3, rate3),
             srate4 = calc(fSlowRate4, rate4);
      s1 = dabc::format(" (%12s)", rate_to_str(srate1).c_str());
      s2 = dabc::format(" (%6.3f)", srate2/1024./1024.);
      s3 = dabc::format(" (%12s)", rate_to_str(srate3).c_str());
      s4 = dabc::format(" (%6.3f)", srate4/1024./1024.);
   }

   if (fDoShow) {
      if (delta > 0) {
         unsigned nlines = comb->fCfg.size() + 5;
         if (fServPort >= 0) nlines++;
         if (fFilePort >= 0) nlines++;
         for (unsigned n = 0; n < nlines; n++)
            fputs("\033[A\033[2K", stdout);
         rewind(stdout);
         auto res = ftruncate(1,0);
         (void) res; // just avoid compiler warnings
      } else {
         fprintf(stdout,"DOGMA terminal info:\n"
                        "  disc  - all discarded packets in the UDP receiver\n"
                        "  err32 - 32-byte header does not match with 32-bytes footer\n"
                        "  errbits - error bits not 0 and not 1\n"
                        "  bufs  - number of produced buffers\n"
                        "  qu    - input queue of combiner module\n"
                        "  drop  - dropped subevents (received by combiner but not useful)\n"
                        "  lost  - lost subevents (never seen by combiner)\n"
                        "  trigger - last trigger values (after masking them in combiner module)\n"
                        "  progr - progress of TDC calibration\n");
      }
   }

   fTotalBuildEvents = comb->fAllBuildEvents;
   fTotalRecvBytes = comb->fAllRecvBytes;
   fTotalDiscEvents = comb->fAllDiscEvents;
   fTotalDroppedData = comb->fAllDroppedData;

   s += "---------------------------------------------\n";
   s += dabc::format("Memory pool: %8s Used: %4.1f%s\n", dabc::size_to_str(fMemorySize).c_str(), fMemoryUsage*100., "%");
   s += dabc::format("Events:%8s   Rate:%12s %s Data: %10s  Rate:%6.3f MB/s%s\n",
                        dabc::number_to_str(fTotalBuildEvents, 1).c_str(),
                        rate_to_str(rate1).c_str(), s1.c_str(),
                        dabc::size_to_str(fTotalRecvBytes).c_str(),
                        rate2/1024./1024., s2.c_str());
   s += dabc::format("Dropped:%7s   Rate:%12s %s Data: %10s  Rate:%6.3f MB/s%s",
                        dabc::number_to_str(fTotalDiscEvents, 1).c_str(),
                        rate_to_str(rate3).c_str(), s3.c_str(),
                        dabc::size_to_str(fTotalDroppedData).c_str(),
                        rate4/1024./1024., s4.c_str());

   if (comb->fAllFullDrops > 0)
      s += dabc::format(" Total:%s\n", dabc::size_to_str(comb->fAllFullDrops, 1).c_str());
   else
      s += "\n";

   if (fServPort >= 0) {
      if (fLastServCmd.null()) {
         s += dabc::format("Server: missing, failed or not found on %s/Output%d\n", fModuleName.c_str(), fServPort);
      } else {
         s += dabc::format("Server: clients:%d inpqueue:%d cansend:%s\n", fLastServCmd.GetInt("NumClients"), fLastServCmd.GetInt("NumCanRecv"), fLastServCmd.GetStr("NumCanSend").c_str());
      }
   } else if (comb->fBNETsend || comb->fBNETrecv) {
      s += comb->fBnetInfo;
      s += "\n";
   }

   if (fFilePort >= 0) {
      if (fLastFileCmd.null()) {
         s += dabc::format("File: missing, failed or not found on %s/Output%d\n", fFileModuleName.empty() ? fModuleName.c_str() : fFileModuleName.c_str(), fFilePort);
      } else {
         std::string state = fLastFileCmd.GetStr("OutputState");
         if (state != "Ready")
            state = std::string(" State: ") + state;
         else
            state.clear();
         s += dabc::format("File: %8s Data: %10s Queue: %u  Curr:  %10s    Name: %s%s\n",
                           dabc::number_to_str(fLastFileCmd.GetDouble("OutputFileEvents"),1).c_str(),
                           dabc::size_to_str(fLastFileCmd.GetDouble("OutputFileSize")).c_str(),
                           (unsigned) fLastFileCmd.GetUInt("InputQueue"),
                           dabc::size_to_str(fLastFileCmd.GetDouble("OutputCurrFileSize")).c_str(),
                           fLastFileCmd.GetStr("OutputCurrFileName").c_str(),
                           state.c_str());
      }
   } else if (comb->fBNETsend || comb->fBNETrecv) {
      s += comb->fBnetStat;
      s += "\n";
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

   s += "inp port     pkt      data    MB/s   disc  magic   bufs  qu errbits drop  lost";
   if (istdccal) s += "    TRB         TDC               progr   state";
   if (fRingSize>0) s += "   triggers";
   s += "\n";

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

   std::vector<int64_t> ports, recvbytes, inperrbits, inpdrop, inplost;
   std::vector<double> inprates;

   for (unsigned n=0;n<comb->fCfg.size();n++) {

      std::string sbuf = dabc::format("%2u", n);

      auto &cfg = comb->fCfg[n];

      auto info = (dogma::TransportInfo *) cfg.fInfo;

      if (!info) {
         sbuf.append("  missing transport-info                             ");
         fCalibr[n].lastrecv = 0;
      } else {

         double rate = (info->fTotalRecvBytes > fCalibr[n].lastrecv) ? (info->fTotalRecvBytes - fCalibr[n].lastrecv) * delta : 0.;

         sbuf.append(dabc::format(" %5d %7s %9s %7.3f %6s %6s %6s",
               info->fNPort,
               dabc::number_to_str(info->fTotalRecvPacket,1).c_str(),
               dabc::size_to_str(info->fTotalRecvBytes).c_str(),
               rate/1024./1024.,
               info->GetDiscardString().c_str(),
               info->GetDiscard32String().c_str(),
               dabc::number_to_str(info->fTotalProducedBuffers).c_str()));
         fCalibr[n].lastrecv = info->fTotalRecvBytes;

         ports.emplace_back(info->fNPort);
         recvbytes.emplace_back(info->fTotalRecvBytes);
         inprates.emplace_back(rate);
      }

      sbuf.append(dabc::format(" %3d %6s %5s %5s",
                   cfg.fNumCanRecv,
                   dabc::number_to_str(cfg.fErrorBitsCnt,0).c_str(),
                   dabc::number_to_str(cfg.fDroppedTrig,0).c_str(),
                   dabc::number_to_str(cfg.fLostTrig,0).c_str()));

      inperrbits.emplace_back(cfg.fErrorBitsCnt);
      inpdrop.emplace_back(cfg.fDroppedTrig);
      inplost.emplace_back(cfg.fLostTrig);

      if (cfg.fCalibr.length() > 0) {
         sbuf.append(dabc::format(" 0x%04x", fCalibr[n].trb));

         std::string tdc = " [";
         for (unsigned j=0;j<fCalibr[n].tdcs.size();j++) {
            if (j>0) tdc.append(",");
            if ((j>3) && (fCalibr[n].tdcs.size()>4)) { tdc.append(" ..."); break; }
            tdc.append(dabc::format("%04x", (unsigned) fCalibr[n].tdcs[j]));
         }
         tdc.append("]");
         while (tdc.length()<27) tdc.append(" ");
         sbuf.append(tdc);

         sbuf.append(dabc::format(" %3d %10s", fCalibr[n].progress, fCalibr[n].state.c_str()));

         if (fCalibr[n].state.find("Ready") != 0) isready = false;
      }

      s += sbuf;

      if (fRingSize>0) s += "  " + cfg.TriggerRingAsStr(fRingSize);

      s += "\n";
   }

   if (fDoShow)
      fprintf(stdout, "%s", s.c_str());

   fWorkerHierarchy.GetHChild("State").SetField("value", isready ? "Ready" : "Init");
   fWorkerHierarchy.GetHChild("Output").SetField("value", s);
   ditem.SetField("inputs", ports);
   ditem.SetField("inprecv", recvbytes);
   ditem.SetField("inprates", inprates);
   ditem.SetField("inperrbits", inperrbits);
   ditem.SetField("inpdrop", inpdrop);
   ditem.SetField("inplost", inplost);

   if (!fFileReqRunning && (fFilePort >= 0)) {
      fFileReqRunning = true;
      dabc::Command cmd("GetTransportStatistic");
      cmd.SetStr("_for_the_port_", dabc::format("Output%d", fFilePort));
      cmd.SetBool("_file_req", true);
      if (fFileModuleName.empty() || (fFileModuleName == fModuleName))
         m.Submit(Assign(cmd));
      else
         dabc::mgr.FindModule(fFileModuleName).Submit(Assign(cmd));
   }

   if (!fServReqRunning && (fServPort >= 0)) {
      fServReqRunning = true;
      dabc::Command cmd("GetTransportStatistic");
      cmd.SetStr("_for_the_port_", dabc::format("Output%d", fServPort));
      cmd.SetBool("_serv_req", true);
      m.Submit(Assign(cmd));
   }
}

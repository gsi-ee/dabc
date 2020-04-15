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

#include "hadaq/Factory.h"

#include "dabc/Manager.h"

#include "hadaq/HldInput.h"
#include "hadaq/HldOutput.h"
#include "hadaq/UdpTransport.h"
#include "hadaq/SorterModule.h"
#include "hadaq/CombinerModule.h"
#include "hadaq/TerminalModule.h"
#include "hadaq/BnetMasterModule.h"
#include "hadaq/api.h"


dabc::FactoryPlugin hadaqfactory(new hadaq::Factory("hadaq"));


dabc::Reference hadaq::Factory::CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd)
{
   if (classname=="hadaq_iter")
      return new hadaq::EventsIterator(objname);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}


dabc::DataInput* hadaq::Factory::CreateDataInput(const std::string &typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT1("HLD input file name %s", url.GetFullName().c_str());

      return new hadaq::HldInput(url);
   }

   return nullptr;
}

dabc::DataOutput* hadaq::Factory::CreateDataOutput(const std::string &typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT1("HLD output file name %s", url.GetFullName().c_str());
      return new hadaq::HldOutput(url);
   }

   return nullptr;
}

dabc::Module* hadaq::Factory::CreateTransport(const dabc::Reference& port, const std::string &typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (!portref.IsInput() ||  url.GetFullName().empty() ||
       ((url.GetProtocol()!="hadaq") && (url.GetProtocol()!="nhadaq") && (url.GetProtocol()!="ohadaq")))
      return dabc::Factory::CreateTransport(port, typ, cmd);

   unsigned trignum = portref.GetModule().Cfg(hadaq::xmlHadaqTrignumRange, cmd).AsUInt(0x1000000);

   std::string portname = portref.GetName();

   int calibr = url.GetOptionInt("calibr", -1);

   if (url.HasOption("trb") && (url.HasOption("tdc") || (calibr>=0))) {
      // first create TDC calibration module, connected to combiner

      std::string calname = dabc::format("TRB%04x_TdcCal", (unsigned) url.GetOptionInt("trb"));


      if (calibr>=0)
         DOUT0("Create calibration module %s AUTOMODE %d", calname.c_str(), calibr);
      else
         DOUT0("Create calibration module %s TDCS %s", calname.c_str(), url.GetOptionStr("tdc").c_str());

      dabc::CmdCreateModule mcmd("stream::TdcCalibrationModule", calname);
      mcmd.SetStr("TRB", url.GetOptionStr("trb"));
      if ((calibr>=0) && !url.HasOption("dummy")) {
         mcmd.SetInt("Mode", calibr);
      } else {
         mcmd.SetInt("Mode", -1);
         mcmd.SetStr("TDC", url.GetOptionStr("tdc"));
      }

      if (url.HasOption("hub")) mcmd.SetStr("HUB", url.GetOptionStr("hub"));
      if (url.HasOption("trig")) mcmd.SetStr("CalibrTrigger", url.GetOptionStr("trig"));
      if (url.HasOption("dummy")) mcmd.SetBool("Dummy", true);
      mcmd.SetInt("portid", portref.ItemSubId());
      mcmd.SetInt(dabc::xmlNumInputs, 1);
      mcmd.SetInt(dabc::xmlNumOutputs, 1);

      dabc::mgr.Execute(mcmd);

      dabc::ModuleRef calm = dabc::mgr.FindModule(calname);

      dabc::LocalTransport::ConnectPorts(calm.FindPort("Output0"), portref, cmd);

      portref = calm.FindPort("Input0");

      // workaround - we say manager that it should connect transport with other port
      cmd.SetStr(dabc::CmdCreateTransport::PortArg(), portref.ItemName());

      dabc::mgr.app().AddObject("module", calname);
   }

   if (url.HasOption("resort")) {
      // then create resort module, connected to combiner or TDC calibration

      std::string sortname = dabc::format("%sResort", portname.c_str());

      DOUT0("Create sort module %s trignum 0x%06x", sortname.c_str(), trignum);

      dabc::CmdCreateModule mcmd("hadaq::SorterModule", sortname);
      mcmd.SetUInt(hadaq::xmlHadaqTrignumRange, trignum);
      dabc::mgr.Execute(mcmd);

      dabc::ModuleRef sortm = dabc::mgr.FindModule(sortname);

      dabc::LocalTransport::ConnectPorts(sortm.FindPort("Output0"), portref, cmd);

      portref = sortm.FindPort("Input0");

      // workaround - we say manager that it should connect transport with other port
      cmd.SetStr(dabc::CmdCreateTransport::PortArg(), portref.ItemName());

      dabc::mgr.app().AddObject("module", sortname);
   }

   int nport = url.GetPort();
   if (nport<=0) { EOUT("Port not specified"); return 0; }

   int rcvbuflen = url.GetOptionInt("udpbuf", 200000);
   int fd = NewAddon::OpenUdp(url.GetHostName(), nport, rcvbuflen);
   if (fd<=0) { EOUT("Cannot open UDP socket for %s", url.GetHostNameWithPort().c_str()); return 0; }

   int mtu = url.GetOptionInt("mtu", 64512);
   int maxloop = url.GetOptionInt("maxloop", 100);
   double flush = url.GetOptionDouble(dabc::xml_flush, 1.);
   double reduce = url.GetOptionDouble("reduce", 1.);
   double lost_rate = url.GetOptionDouble("lost", 0);
   bool debug = url.HasOption("debug");
   int udp_queue = url.GetOptionInt("upd_queue", 0);

   if (udp_queue>0) cmd.SetInt("TransportQueue", udp_queue);

   DOUT0("Start HADAQ UDP transport on %s", url.GetHostNameWithPort().c_str());

   NewAddon* addon = new NewAddon(fd, nport, mtu, debug, maxloop, reduce, lost_rate);
	return new hadaq::NewTransport(cmd, portref, addon, flush);
}


dabc::Module* hadaq::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "hadaq::CombinerModule")
      return new hadaq::CombinerModule(modulename, cmd);

   if (classname == "hadaq::SorterModule")
      return new hadaq::SorterModule(modulename, cmd);

   if (classname == "hadaq::MbsTransmitterModule") {
      EOUT("MbsTransmitterModule class no longer supported");
      return nullptr;
   }

   if (classname == "hadaq::ReadoutModule")
      return new hadaq::ReadoutModule(modulename, cmd);

   if (classname == "hadaq::TerminalModule")
      return new hadaq::TerminalModule(modulename, cmd);

   if (classname == "hadaq::BnetMasterModule")
      return new hadaq::BnetMasterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

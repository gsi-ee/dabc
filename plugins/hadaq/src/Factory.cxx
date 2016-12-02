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

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/Configuration.h"
#include "dabc/Port.h"
#include "dabc/Url.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/HldInput.h"
#include "hadaq/HldOutput.h"
#include "hadaq/UdpTransport.h"
#include "hadaq/SorterModule.h"
#include "hadaq/CombinerModule.h"
#include "hadaq/TerminalModule.h"
#include "hadaq/Iterator.h"
#include "hadaq/Observer.h"
#include "hadaq/api.h"


dabc::FactoryPlugin hadaqfactory(new hadaq::Factory("hadaq"));


dabc::Reference hadaq::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname=="hadaq_iter")
      return new hadaq::EventsIterator(objname);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}


dabc::DataInput* hadaq::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT1("HLD input file name %s", url.GetFullName().c_str());

      return new hadaq::HldInput(url);
   }

   return 0;
}

dabc::DataOutput* hadaq::Factory::CreateDataOutput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT1("HLD output file name %s", url.GetFullName().c_str());
      return new hadaq::HldOutput(url);
   }

   return 0;
}

dabc::Module* hadaq::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (!portref.IsInput() || ((url.GetProtocol()!="hadaq") && (url.GetProtocol()!="nhadaq")) || url.GetFullName().empty())
      return dabc::Factory::CreateTransport(port, typ, cmd);

   unsigned trignum = portref.GetModule().Cfg(hadaq::xmlHadaqTrignumRange, cmd).AsUInt(0x1000000);

   std::string portname = portref.GetName();

   if (url.HasOption("tdc")) {
      // first create TDC calibration module, connected to combiner

      std::string calname = dabc::format("%sTdcCal", portname.c_str());

      DOUT0("Create calibration module %s TDCS %s", calname.c_str(), url.GetOptionStr("tdc").c_str());

      dabc::CmdCreateModule mcmd("stream::TdcCalibrationModule", calname);
      mcmd.SetStr("TDC", url.GetOptionStr("tdc"));
      if (url.HasOption("trb")) mcmd.SetStr("TRB", url.GetOptionStr("trb"));
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
   if (fd<=0) { EOUT("Cannot open UDP soocket for port %d", nport); return 0; }

   int mtu = url.GetOptionInt("mtu", 64512);
   int maxloop = url.GetOptionInt("maxloop", 100);
   double flush = url.GetOptionDouble(dabc::xml_flush, 1.);
   double reduce = url.GetOptionDouble("reduce", 1.);
   bool observer = url.GetOptionBool("observer", false);
   bool debug = url.HasOption("debug");
   int udp_queue = url.GetOptionInt("upd_queue", 0);

   if (udp_queue>0) cmd.SetInt("TransportQueue", udp_queue);

   if (url.GetProtocol()=="nhadaq") {
	   NewAddon* addon = new NewAddon(fd, nport, mtu, debug, maxloop, reduce);
	   return new hadaq::NewTransport(cmd, portref, addon, observer, flush);
   }

   DOUT0("Start UDP transport with port %d", nport);
   DataSocketAddon* addon = new DataSocketAddon(fd, nport, mtu, flush, debug, maxloop, reduce);
   return new hadaq::DataTransport(cmd, portref, addon, observer);
}


dabc::Module* hadaq::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "hadaq::CombinerModule")
      return new hadaq::CombinerModule(modulename, cmd);

   if (classname == "hadaq::SorterModule")
      return new hadaq::SorterModule(modulename, cmd);

   if (classname == "hadaq::MbsTransmitterModule") {
      EOUT("MbsTransmitterModule class no longer supported");
      return 0;
   }

   if (classname == "hadaq::ReadoutModule")
      return new hadaq::ReadoutModule(modulename, cmd);

   if (classname == "hadaq::TerminalModule")
      return new hadaq::TerminalModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


void hadaq::Factory::Initialize()
{
   dabc::XMLNodePointer_t node = 0;

   while (dabc::mgr()->cfg()->NextCreationNode(node, "Observer", true)) {

      // const char* name = dabc::Xml::GetAttr(node, dabc::xmlNameAttr);

      const char* thrdname = dabc::Xml::GetAttr(node, dabc::xmlThreadAttr);
      if ((thrdname==0) || (*thrdname==0)) thrdname = "ShmThread";

      dabc::WorkerRef w = new hadaq::Observer("/shm");
      w.MakeThreadForWorker(thrdname);
   }
}

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

#include "hadaq/MonitorModule.h"

#include "dabc/Publisher.h"
#include "dabc/Buffer.h"
#include "hadaq/Iterator.h"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <streambuf>

hadaq::MonitorModule::MonitorModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fInterval(1.),
   fTopFolder(),
   fAddrs(),
   fEventId(0)
{
   EnsurePorts(0, 1, dabc::xmlWorkPool);

   fSubevId = Cfg("SubevId", cmd).AsUInt(0);
   fTriggerType = Cfg("TriggerType", cmd).AsUInt(1);
   if (fTriggerType > 0xF) fTriggerType = 0xF;

   fInterval = Cfg("Period", cmd).AsDouble(1.);

   fTopFolder = Cfg("TopFolder", cmd).AsStr(fTopFolder);

   fAddrs0 = Cfg("Addrs0", cmd).AsUIntVect();
   fAddrs = Cfg("Addrs", cmd).AsUIntVect();

   fShellCmd = Cfg("ShellCmd", cmd).AsStr("du -d 0 .");

   fWorkerHierarchy.Create("TRBNET");

   for (unsigned ix = 0; ix < fAddrs.size(); ++ix) {
      dabc::Hierarchy item = fWorkerHierarchy.CreateHChild(GetItemName(fAddrs[ix]));
      DOUT0("Name = %s item %p", GetItemName(fAddrs[ix]).c_str(), item());
      item.SetField(dabc::prop_kind, "rate");
      item.EnableHistory(100);
   }

   if (fInterval > 0)
      CreateTimer("TrbNetRead", fInterval);

   if (fTopFolder.empty())
      Publish(fWorkerHierarchy, "TRBNET");
   else
      Publish(fWorkerHierarchy, std::string("TRBNET/") + fTopFolder);
}

std::string hadaq::MonitorModule::GetItemName(unsigned addr)
{
   return dabc::format("addr_%04x", addr);
}


void hadaq::MonitorModule::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

}

uint32_t hadaq::MonitorModule::DoRead(uint32_t addr0, uint32_t addr)
{
   std::string tmpfile = "output.log";

   std::string cmd;
   if (addr0 > 0)
      cmd = dabc::format(fShellCmd.c_str(), addr0, addr);
   else
      cmd = dabc::format(fShellCmd.c_str(), addr);

   cmd.append(dabc::format(" >%s 2<&1", tmpfile.c_str()));

   // execute command
   std::system(cmd.c_str());

   std::ifstream t(tmpfile);
   std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());

   // remove temporary file
   std::remove(tmpfile.c_str());

   unsigned value1 = 0, value2 = 0;

   std::sscanf(str.c_str(), "0x%x 0x%x", &value1, &value2);

   // DOUT0("Reading addr %x result %s 0x%x", addr, str.c_str(), value2);

   return value2;
}


bool hadaq::MonitorModule::ReadAllVariables(dabc::Buffer &buf)
{
   if (buf.null()) return false;

   buf.SetTypeId(hadaq::mbt_HadaqEvents);

   // initialzie buffer
   int block_size = (fAddrs0.size() == 0) ? 2 : 3;

   hadaq::WriteIterator iter(buf);
   if (!iter.NewEvent(fEventId, 0 ,fAddrs.size() * 4 * block_size + sizeof(hadaq::RawSubevent))) return false;
   if (!iter.NewSubevent(fAddrs.size() * 4 * block_size, fEventId)) return false;

   // iter.evnt()->SetTrigTypeTrb3(0xC);

   uint32_t id = iter.evnt()->GetId();
   iter.evnt()->SetId((id & 0xfffffff0) | (fTriggerType & 0xf));

   iter.subevnt()->SetDecodingDirect(0x11000200); // swapped mode, must be first
   iter.subevnt()->SetTrigNr(fEventId); // assign again, otherwise swapping applyed wrong
   iter.subevnt()->SetTrigTypeTrb3(fTriggerType); // trigger type
   iter.subevnt()->SetId(fSubevId);  // subevent id

   // write raw data
   uint32_t *rawdata = (uint32_t *) iter.rawdata();
   for (unsigned n=0; n < fAddrs.size(); ++n) {

      unsigned addr0 = 0;
      if (block_size == 3) {
         addr0 = (fAddrs0.size() >= fAddrs.size()) ? fAddrs0[n] : fAddrs0[0];
         iter.subevnt()->SetValue(rawdata++, addr0); // write address0
      }

      iter.subevnt()->SetValue(rawdata++, fAddrs[n]); // write address
      // iter.subevnt()->SetValue(rawdata++, 0x1234567); // write value

      iter.subevnt()->SetValue(rawdata++, DoRead(addr0, fAddrs[n])); // write value
   }

   // closing buffer
   iter.FinishSubEvent(fAddrs.size() * 4 * block_size);
   iter.FinishEvent();

   fEventId++;

   buf = iter.Close();

   return !buf.null();
}


void hadaq::MonitorModule::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) != "TrbNetRead")
      return;

   dabc::Buffer buf = TakeBuffer();

   if (ReadAllVariables(buf))
      SendToAllOutputs(buf);
}

void hadaq::MonitorModule::BeforeModuleStart()
{
   if (fInterval > 0) return;

   dabc::Buffer buf = TakeBuffer();

   if (ReadAllVariables(buf))
      SendToAllOutputs(buf);
}

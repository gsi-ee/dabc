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

hadaq::MonitorModule::MonitorModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fInterval(1.),
   fTopFolder(),
   fAddrs(),
   fEventId(0)
{
   EnsurePorts(0, 1, dabc::xmlWorkPool);


   fInterval = Cfg("Period", cmd).AsDouble(1.);

   fTopFolder = Cfg("TopFolder", cmd).AsStr(fTopFolder);

   fAddrs = Cfg("Addrs", cmd).AsUIntVect();

   fWorkerHierarchy.Create("TRBNET");

   for (unsigned ix = 0; ix < fAddrs.size(); ++ix) {
      dabc::Hierarchy item = fWorkerHierarchy.CreateHChild(GetItemName(fAddrs[ix]));
      DOUT0("Name = %s item %p", GetItemName(fAddrs[ix]).c_str(), item());
      item.SetField(dabc::prop_kind, "rate");
      item.EnableHistory(100);
   }

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

uint32_t hadaq::MonitorModule::DoRead(uint32_t addr)
{
   return addr;
}


bool hadaq::MonitorModule::ReadAllVariables(dabc::Buffer &buf)
{
   if (buf.null()) return false;

   buf.SetTypeId(hadaq::mbt_HadaqEvents);

   // initialzie buffer
   hadaq::WriteIterator iter(buf);
   if (!iter.NewEvent(fEventId, 0 ,fAddrs.size() * 8 + sizeof(hadaq::RawSubevent))) return false;
   if (!iter.NewSubevent(fAddrs.size() * 8, fEventId)) return false;

   // iter.subevnt()->SetDecoding(0x020011);

   iter.subevnt()->SetDecoding(0x11000200); // swapped mode?

   // write raw data
   uint32_t *rawdata = (uint32_t *) iter.rawdata();
   for (unsigned n=0; n < fAddrs.size(); ++n) {

      iter.subevnt()->SetValue(rawdata++, fAddrs[n]); // write address
      iter.subevnt()->SetValue(rawdata++, 0x1234567); // write value

      // iter.subevnt()->SetValue(rawdata++, DoRead(fAddrs[n])); // write value
   }

   // closing buffer
   iter.FinishSubEvent(fAddrs.size() * 8);
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

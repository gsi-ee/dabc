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

#include "hadaq/FilterModule.h"

#include "hadaq/Iterator.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"

#include <cstdlib>

hadaq::FilterModule::FilterModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   // we need at least one input and one output port
   EnsurePorts(1, 1, dabc::xmlWorkPool);

   double flushtime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(0.3);

   if (flushtime > 0.)
      CreateTimer("FlushTimer", flushtime);

   CreatePar("FilterData").SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar("FilterEvents").SetRatemeter(false, 3.).SetUnits("Ev");

   std::string file_name = Cfg("FilterCode", cmd).AsStr("");

   if (!file_name.empty()) {
      const char *dabcsys = std::getenv("DABCSYS");

#if defined(__MACH__) /* Apple OSX section */
      const char *compiler = "clang++";
      const char *ldflags = "";
#else
      const char *compiler = "g++";
      const char *ldflags = "-Wl,--no-as-needed";
#endif

      std::string exec = dabc::format("%s %s -O2 -fPIC -Wall -std=c++11 -I. -I%s/include"
            " -shared -Wl,-soname,librunfilter.so %s -Wl,-rpath,%s/lib -L%s/lib -lDabcBase -lDabcMbs -lDabcHadaq -o librunfilter.so",
            compiler, file_name.c_str(), dabcsys,
            ldflags, dabcsys, dabcsys);

      DOUT0("Executing %s", exec.c_str());

      int res = std::system(exec.c_str());

      if (res != 0) {
         EOUT("Fail to compile first.C/second.C scripts. Abort");
         dabc::mgr.StopApplication();
         return;
      }

      if (!dabc::Factory::LoadLibrary("librunfilter.so")) {
         EOUT("Fail to load generated librunfilter.so library");
         dabc::mgr.StopApplication();
         return;
      }

      fFilterFunc = dabc::Factory::FindSymbol("filter_func");
      if (!fFilterFunc) {
         EOUT("Fail to find filter_func function in librunfilter.so library");
         dabc::mgr.StopApplication();
         return;
      }

   }

}

typedef bool filter_func_t(void *);



bool hadaq::FilterModule::retransmit()
{
   int cnt = 20;

   filter_func_t *func = (filter_func_t *) fFilterFunc;

   while (CanSendToAllOutputs() && CanRecv() && CanTakeBuffer() && (cnt-- > 0)) {

      auto buf = Recv();
      if (buf.null()) break;

      if (buf.GetTypeId() == dabc::mbt_EOF) {
         SendToAllOutputs(buf);
         continue;
      }

      hadaq::ReadIterator iter(buf);

      hadaq::WriteIterator iter2(TakeBuffer());

      int numevents = 0;
      while (iter.NextSubeventsBlock()) {
         bool accept = true;

         if (func)
            accept = func(iter.evnt());

         if (accept) {
            numevents++;
            if (!iter2.CopyEvent(iter))
               EOUT("Fail to copy event!!!");
         }
      }

      auto outbuf = iter2.Close();
      if (!outbuf.null() && (outbuf.GetTotalSize() > 0)) {

         fEventRateCnt += numevents;
         fDataRateCnt += outbuf.GetTotalSize();

         Par("FilterData").SetValue(fDataRateCnt/1024./1024.);
         Par("FilterEvents").SetValue(fEventRateCnt);

         SendToAllOutputs(outbuf);
      }
   }

   return (cnt <= 0); // if cnt less than 0, reinject event
}


void hadaq::FilterModule::ProcessTimerEvent(unsigned)
{
   retransmit();
}

int hadaq::FilterModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

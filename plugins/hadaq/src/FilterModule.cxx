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

static const char *MergerName = "Merger";


hadaq::FilterModule::FilterModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   EnsurePorts(1, 1, dabc::xmlWorkPool);

   fMerger = IsName(MergerName);
   fSpliter = !fMerger && !cmd.GetBool("sorter", false) && (NumOutputs() > 2);
   fFilterFunc = cmd.GetPtr("filter");
   fSubFilter = (fFilterFunc != nullptr) && !fSpliter && !fMerger;

   double flushtime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(0.);

   if (flushtime > 0.)
      CreateTimer("FlushTimer", flushtime);

   CreatePar("FilterData").SetRatemeter(false, 3.).SetUnits("MB");
   CreatePar("FilterEvents").SetRatemeter(false, 3.).SetUnits("Ev");

   std::string file_name = Cfg("FilterCode", cmd).AsStr("");

   if (!fFilterFunc && !file_name.empty() && !fMerger) {
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
         EOUT("Fail to compile %s. Abort", file_name.c_str());
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

void hadaq::FilterModule::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

//   DOUT0("Thread assign %s ninp %u nout %u", GetName(), NumInputs(), NumOutputs());

   if (fFilterFunc && fSpliter) {

      //dabc::CmdCreateModule cmd0("hadaq::FilterModule", MergerName);
      //cmd0.SetBool("merger", true);
      //cmd0.SetInt(dabc::xmlNumInputs, NumOutputs());
      //DOUT0("Create module %s", MergerName);
      //dabc::mgr.Execute(cmd0);

      dabc::ModuleRef merger = dabc::mgr.FindModule(MergerName);

      if (merger.null() || merger.NumInputs() != NumOutputs()) {
         EOUT("Did not found Merger module - HALT");
         dabc::mgr.StopApplication();
         return;
      }

      if (merger.NumInputs() != NumOutputs()) {
         EOUT("Merger inputs %u mismatch outputs in splitter %u", merger.NumInputs(), NumOutputs());
         dabc::mgr.StopApplication();
         return;
      }

      for (unsigned n = 0; n < NumOutputs(); n++) {
         std::string mname = dabc::format("Filter%03u", n);
         dabc::CmdCreateModule cmd("hadaq::FilterModule", mname);
         cmd.SetPtr("filter", fFilterFunc);
         cmd.SetBool("sorter", true);

         DOUT0("Create module %s", mname.c_str());

         dabc::mgr.Execute(cmd);

         dabc::ModuleRef m = dabc::mgr.FindModule(mname);

         dabc::mgr.Connect(OutputName(n,true), m.InputName(0));
         DOUT0("Connect %s -> %s connected %s", OutputName(n,true).c_str(), m.InputName(0).c_str(), DBOOL(IsOutputConnected(n)));

         dabc::mgr.Connect(m.OutputName(0), merger.InputName(n));
         DOUT0("Connect %s -> %s", m.OutputName(0).c_str(), merger.InputName(n).c_str());

         m.Start();
      }

      // merger.Start();
   }

}


void hadaq::FilterModule::BeforeModuleStart()
{
   DOUT0("START %s", GetName());
}

void hadaq::FilterModule::AfterModuleStop()
{
   DOUT0("STOP %s", GetName());
}


typedef bool filter_func_t(void *);


bool hadaq::FilterModule::retransmit()
{
   if (fSpliter)
      return distributeBuffers();

   if (fMerger)
      return mergeBuffers();

   return filterBuffers();
}

bool hadaq::FilterModule::distributeBuffers()
{
   int cnt = 200;

   // DOUT0("Distributer %s get called %s %s", GetName(), DBOOL(CanSendToAllOutputs()), DBOOL(CanRecv()));

   while(CanRecv() && CanSendToAllOutputs() && (cnt-- > 0)) {

      auto buf = Recv();

      //if (buf.GetTypeId() == dabc::mbt_EOF)
      //   DOUT0("!!!!!!!!!!!!!! DISTRIBUTER SEES EOF   !!!!!");

      unsigned nport = fSeqId++ % NumOutputs();
      // DOUT0("Distribute buffer %u to port %u", buf.GetTotalSize(), nport);
      Send(nport, buf);
   }

   return true; // cnt <= 0;
}

bool hadaq::FilterModule::mergeBuffers()
{
   int cnt = 20;

   // DOUT0("Merger %s get called seq %u %s %s ", GetName(), fSeqId, DBOOL(CanSendToAllOutputs()), DBOOL(CanRecv(fSeqId % NumInputs())));

   while (CanSendToAllOutputs() && CanRecv(fSeqId % NumInputs()) && (cnt-- > 0)) {
      auto buf = Recv(fSeqId++ % NumInputs());

      // handle dummy buffer
      if (buf.GetTypeId() == dabc::mbt_Generic)
         continue;

      // handle EOF buffer
      // if (buf.GetTypeId() == dabc::mbt_EOF)
      //   DOUT0("!!!!!!!!!!!!!! SEE EOF   !!!!!");

      SendToAllOutputs(buf);
   }

   return cnt <= 0;
}


bool hadaq::FilterModule::filterBuffers()
{
   int cnt = 20;

   filter_func_t *func = (filter_func_t *) fFilterFunc;

   // DOUT0("Filter %s get called %s %s %s isconnected %s", GetName(), DBOOL(CanSendToAllOutputs()), DBOOL(CanRecv()), DBOOL(CanTakeBuffer()), DBOOL(IsInputConnected(0)));

   while (CanSendToAllOutputs() && CanRecv() && CanTakeBuffer() && (cnt-- > 0)) {

      auto buf = Recv();
      if (buf.null()) continue;

      // DOUT0("Filter %s get new buffer %u", GetName(), buf.GetTotalSize());

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
      if (outbuf.null()) continue;

      if (outbuf.GetTotalSize() > 0) {

         fEventRateCnt += numevents;
         fDataRateCnt += outbuf.GetTotalSize();

         Par("FilterData").SetValue(fDataRateCnt/1024./1024.);
         Par("FilterEvents").SetValue(fEventRateCnt);

      } else {
         if (!fSubFilter) continue; // no need to create empty buffer
         outbuf.SetTypeId(dabc::mbt_Generic);
      }

      SendToAllOutputs(outbuf);
   }

   return cnt <= 0; // if cnt less than 0, reinject event
}


void hadaq::FilterModule::ProcessTimerEvent(unsigned)
{
   retransmit();
}

int hadaq::FilterModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void hadaq::FilterModule::ProcessConnectEvent(const std::string &name, bool on)
{
   // DOUT0("Module %s port %s ProcessConnectEvent %s", GetName(), name.c_str(), DBOOL(on));

   (void) name;

   if ((fMerger || fSubFilter || fSpliter) && !on) {
      bool isany = false;
      for (unsigned n = 0; n < NumOutputs(); ++n)
         if (IsOutputConnected(n))
            isany = true;
      if (!isany) {
         DisconnectAllPorts();
         Stop();
      }
   }
}

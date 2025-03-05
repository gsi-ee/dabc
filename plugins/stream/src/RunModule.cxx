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

#include "stream/RunModule.h"

#include "dabc/Configuration.h"
#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Iterator.h"
#include "dabc/Buffer.h"
#include "dabc/Publisher.h"
#include "dabc/Url.h"
#include "dabc/BinaryFile.h"

#include "stream/TdcCalibrationModule.h"

#include "hadaq/Iterator.h"
#include "mbs/Iterator.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/HldProcessor.h"

#include <cstdlib>

#include "base/Buffer.h"
#include "base/StreamProc.h"
#include "stream/DabcProcMgr.h"

// ==================================================================================

stream::RunModule::RunModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fParallel(0),
   fInitFunc(nullptr),
   fStopMode(-1111),
   fProcMgr(nullptr),
   fAsf(),
   fFileUrl(),
   fDidMerge(false),
   fTotalSize(0),
   fTotalEvnts(0),
   fTotalOutEvnts(0)
{
   fParallel = Cfg("parallel", cmd).AsInt(0);

   fDefaultFill = Cfg("fillcolor", cmd).AsInt(3);

   fFastMode = Cfg("fastmode", cmd).AsBool(false);

   // we need one input and no outputs
   EnsurePorts(1, fParallel < 0 ? 0 : fParallel);

   if (fParallel > 0) {
      SetPortSignaling(InputName(0), dabc::Port::SignalEvery);
      for (unsigned n=0;n<NumOutputs();++n)
         SetPortSignaling(OutputName(n), dabc::Port::SignalEvery);
   }

   fInitFunc = cmd.GetPtr("initfunc");

   fFileUrl = cmd.GetStr("fileurl");

   if ((fParallel >= 0) && !fInitFunc) {
      // first generate and load init func

      if (fParallel > 999)
         fParallel = 999;

      // ensure that all histos on all branches present
      hadaq::TdcProcessor::SetAllHistos(true);

      std::string libs_dir = dabc::Configuration::GetLibsDir();
      std::string plugin_dir = dabc::Configuration::GetPluginsDir();
      const char *streamsys = std::getenv("STREAMSYS");

      if (libs_dir.empty()) {
         EOUT("DABC lib dir is empty, check DABCSYS variable or DABC installation");
         dabc::mgr.StopApplication();
         return;
      }

      if (plugin_dir.empty()) {
         EOUT("DABC plugin dir is empty, check DABCSYS or DABC installation");
         dabc::mgr.StopApplication();
         return;
      }

      if (!streamsys) {
         EOUT("STREAMSYS variable not set, cannot run stream framework");
         dabc::mgr.StopApplication();
         return;
      }

      std::string extra_include;
      if (cmd.GetBool("use_autotdc"))
         if (std::system("ls first.C >/dev/null 2>/dev/null") != 0)
            extra_include = dabc::format("-I%s/applications/autotdc", streamsys);

      bool second = std::system("ls second.C >/dev/null 2>/dev/null") == 0;

#if defined(__MACH__) /* Apple OSX section */
      const char *compiler = "clang++";
      const char *ldflags = "";
#else
      const char *compiler = "g++";
      const char *ldflags = "-Wl,--no-as-needed";
#endif

      std::string exec = dabc::format("%s %s/stream/src/stream_engine.cpp -O2 -fPIC -Wall -std=c++11 -I. -I%s/include %s %s"
            " -shared -Wl,-soname,librunstream.so %s -Wl,-rpath,%s -Wl,-rpath,%s/lib  -o librunstream.so",
            compiler, plugin_dir.c_str(), streamsys, extra_include.c_str(),
            (second ? "-D_SECOND_ " : ""), ldflags, libs_dir.c_str(), streamsys);

      std::system("rm -f ./librunstream.so");

      DOUT0("Executing %s", exec.c_str());

      int res = std::system(exec.c_str());

      if (res != 0) {
         EOUT("Fail to compile first.C/second.C scripts. Abort");
         dabc::mgr.StopApplication();
         return;
      }

      if (!dabc::Factory::LoadLibrary("librunstream.so")) {
         EOUT("Fail to load generated librunstream.so library");
         dabc::mgr.StopApplication();
         return;
      }

      fInitFunc = dabc::Factory::FindSymbol("stream_engine");
      if (!fInitFunc) {
         EOUT("Fail to find stream_engine function in librunstream.so library");
         dabc::mgr.StopApplication();
         return;
      }
   }

   if (fParallel >= 0) {
      fAsf = Cfg("asf",cmd).AsStr();
      // do not autosave is specified, module will not stop when data source disappears
      if (fAsf.empty() || (fParallel > 0)) SetAutoStop(false);
      CreatePar("Events").SetRatemeter(false, 3.).SetUnits("Ev");
   } else {
      SetAutoStop(false);
   }

   fWorkerHierarchy.Create("Worker");

   if (fParallel <= 0) {
      CreateTimer("Update", 1.);

      fWorkerHierarchy.CreateHChild("Status").SetField("_hidden", "true");
      fWorkerHierarchy.SetField("_player", "DABC.StreamControl");

      CreatePar("Events").SetRatemeter(false, 3.).SetUnits("Ev");
      CreatePar("DataRate").SetRatemeter(false, 3.).SetUnits("MB");

      dabc::CommandDefinition cmddef = fWorkerHierarchy.CreateHChild("Control/StartRootFile");
      cmddef.SetField(dabc::prop_kind, "DABC.Command");
      // cmddef.SetField(dabc::prop_auth, true); // require authentication
      cmddef.AddArg("fname", "string", true, "file.root");
      cmddef.AddArg("kind", "int", false, "2");
      cmddef.AddArg("maxsize", "int", false, "1900");

      cmddef = fWorkerHierarchy.CreateHChild("Control/StopRootFile");
      cmddef.SetField(dabc::prop_kind, "DABC.Command");
      // cmddef.SetField(dabc::prop_auth, true); // require authentication
   } else {
      CreateTimer("KeepAlive", 0.1);
   }

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));

   double interval = Cfg("AutoSave", cmd).AsDouble(0);
   if (interval > 1) CreateTimer("AutoSave", interval);
}

stream::RunModule::~RunModule()
{
   if (fProcMgr) {
      delete fProcMgr;
      fProcMgr = nullptr;
   }
}

typedef void* entryfunc();


void stream::RunModule::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

   if (fInitFunc && (fParallel <= 0)) {

      entryfunc *func = (entryfunc *) fInitFunc;

      fProcMgr = new DabcProcMgr;
      fProcMgr->SetDefaultFill(fDefaultFill);
      fProcMgr->SetTop(fWorkerHierarchy, fParallel == 0);

      std::string src = "Source: ";
      src += FindPort(InputName(0)).Cfg("url").AsStr();
      fProcMgr->AddRunLog(src.c_str());

      func();

      if (fFileUrl.length() > 0) {
         dabc::Url url(fFileUrl);

         std::string fname = url.GetFullName();

         if (fname.rfind(".root") == fname.length() - 5) {
            fProcMgr->SetTriggeredAnalysis(true);
            int kind = url.GetOptionInt("kind", -1);
            if (kind != -1) fProcMgr->SetStoreKind(kind);
            if (!fProcMgr->CreateStore(fFileUrl.c_str()))
               EOUT("Fail to create store %s - check if libDabcRoot.so plugin in the xml file", fFileUrl.c_str());
         }

         int hlevel = url.GetOptionInt("hlevel", -111);
         if (hlevel != -111) fProcMgr->SetHistFilling(hlevel);

         int hldfilter = url.GetOptionInt("hldfilter", -111);
         if (hldfilter>=0) new hadaq::HldFilter(hldfilter);
      }

      // remove pointer, let other modules to create and use it
      base::ProcMgr::ClearInstancePointer();

      if (fProcMgr->IsStreamAnalysis()) {
         EOUT("Stream analysis kind is not supported in DABC engine");
         dabc::mgr.StopApplication();
      }
   } else if ((fParallel > 0) && fInitFunc)  {
      for (int n=0;n<fParallel;n++) {
         std::string mname = dabc::format("%s%03d", GetName(), n);
         dabc::CmdCreateModule cmd("stream::RunModule", mname);
         cmd.SetPtr("initfunc", fInitFunc);
         cmd.SetInt("parallel", -1);

         DOUT0("Create module %s", mname.c_str());

         dabc::mgr.Execute(cmd);

         dabc::ModuleRef m = dabc::mgr.FindModule(mname);

         DOUT0("Connect %s ->%s", OutputName(n,true).c_str(), m.InputName(0).c_str());
         dabc::Reference r = dabc::mgr.Connect(OutputName(n,true), m.InputName(0));
         DOUT0("Connect output %u connected %s", n, DBOOL(IsOutputConnected(n)));

         m.Start();
      }
   }

   DOUT0("!!!! Assigned to thread %s  !!!!!", thread().GetName());
}

void stream::RunModule::ProduceMergedHierarchy()
{
   if (fDidMerge || (fParallel <= 0)) return;

   fDidMerge = true;

   dabc::PublisherRef publ = GetPublisher();

   dabc::Hierarchy main;
   int nhist = 0;

   DOUT0("Can now merge histograms");
   for (int n=0;n<fParallel;n++) {

      std::string mname = dabc::format("%s%03d", GetName(), n);

      dabc::ModuleRef m = dabc::mgr.FindModule(mname);

      m.Stop();

      dabc::Command cmd("GetHierarchy");
      m.Execute(cmd);

      dabc::Hierarchy h = cmd.GetRef("hierarchy");

      if (main.null()) {
         DOUT0("Adopt histograms from %s", mname.c_str());
         main = h;
         continue;
      }

      dabc::Iterator iter1(main), iter2(h);
      bool miss = false;
      while (iter1.next()) {
         if (!iter2.next()) { miss = true; break; }
         if (strcmp(iter1.name(),iter2.name()) != 0) { miss = true; break; }

         // merge histograms till the end
         dabc::Hierarchy item1 = iter1.ref();
         dabc::Hierarchy item2 = iter2.ref();

         if (item1.HasField("_dabc_hist") && item2.HasField("_dabc_hist") &&
               item1.GetFieldPtr("bins") && item2.GetFieldPtr("bins") &&
               (item1.GetFieldPtr("bins")->GetArraySize() == item2.GetFieldPtr("bins")->GetArraySize())) {
            double* arr1 = item1.GetFieldPtr("bins")->GetDoubleArr();
            double* arr2 = item2.GetFieldPtr("bins")->GetDoubleArr();
            int indx = item1.GetField("_kind").AsStr()=="ROOT.TH1D" ? 2 : 5;
            int len = item1.GetFieldPtr("bins")->GetArraySize();
            if (n == 1) nhist++;

            if (arr1 && arr2)
              while (++indx<len) arr1[indx]+=arr2[indx];
         }
      }

      if (miss) {
         EOUT("!!!!!!!!!!!!!! MISMATCH - CANNOT MERGE HISTOGRAMS !!!!!!!!!!!!");
         dabc::mgr.StopApplication();
         return;
      } else {
         DOUT0("Merged %d histograms from %s", nhist, mname.c_str());
      }
   }

   if (fAsf.length()>0)
      SaveHierarchy(main.SaveToBuffer());
}


int stream::RunModule::ExecuteCommand(dabc::Command cmd)
{
   if (fProcMgr && fProcMgr->ExecuteHCommand(cmd)) {
      if (fProcMgr->IsWorking()) ActivateInput(); // when working set, just ensure that module reads input
      return dabc::cmd_true;
   }

   if (cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {
      std::string cmdpath = cmd.GetStr("Item");
      DOUT0("Execute command %s", cmdpath.c_str());

      if (cmdpath == "Control/StartRootFile") {
         std::string fname = cmd.GetStr("fname","file.root");
         int kind = cmd.GetInt("kind", 2);
         int maxsize = cmd.GetInt("maxsize", 1900);
         fname += dabc::format("?maxsize=%d", maxsize);
         if (fProcMgr)
            if (fProcMgr->CreateStore(fname.c_str())) {
               // only in triggered mode storing is allowed
               if (fProcMgr->IsRawAnalysis()) fProcMgr->SetTriggeredAnalysis(true);
               fProcMgr->SetStoreKind(kind);
               fProcMgr->UserPreLoop(nullptr, true);

            }
         return dabc::cmd_true;

      } else
      if (cmdpath == "Control/StopRootFile") {
         if (fProcMgr) fProcMgr->CloseStore();

         return dabc::cmd_true;
      } else
         return dabc::cmd_false;
   }

   if (cmd.IsName("SlaveFinished")) {
      if (--fStopMode == 0) {
         ProduceMergedHierarchy();
         DOUT0("Stop ourself");
         Stop();
      }
      return dabc::cmd_true;
   } else
   if (cmd.IsName("GetHierarchy")) {
      cmd.SetRef("hierarchy", fWorkerHierarchy);
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void stream::RunModule::BeforeModuleStart()
{
   DOUT0("START STREAM MODULE %s inp %s", GetName(), DBOOL(IsInputConnected(0)));

   if (fProcMgr) fProcMgr->UserPreLoop();
}

void stream::RunModule::SaveHierarchy(dabc::Buffer buf)
{
   if (buf.GetTotalSize() == 0) return;

   DOUT0("store hierarchy size %d in temporary h.bin file", buf.GetTotalSize());
   {
      dabc::BinaryFile f;
      std::system("rm -f h.bin");
      if (f.OpenWriting("h.bin")) {
         if (f.WriteBufHeader(buf.GetTotalSize(), buf.GetTypeId()))
            for (unsigned n=0;n<buf.NumSegments();n++)
               f.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n));
         f.Close();
      }
   }

   std::string args("dabc_root -skip-zero -h h.bin -o ");
   args += fAsf;

   DOUT0("Calling: %s", args.c_str());

   int res = std::system(args.c_str());

   if (res != 0) EOUT("Fail to convert DABC histograms in ROOT file, check h.bin file");
            else std::system("rm -f h.bin");
}

void stream::RunModule::AfterModuleStop()
{
   if (fProcMgr) fProcMgr->UserPostLoop();

   // DOUT0("!!!! thread on start %s  !!!!!", thread().GetName());

   DOUT0("STOP STREAM MODULE %s data %lu evnts %lu outevents %lu  %s", GetName(), fTotalSize, fTotalEvnts, fTotalOutEvnts, (fTotalEvnts == fTotalOutEvnts ? "ok" : "MISSMATCH"));

   if (fParallel > 0) {
      ProduceMergedHierarchy();
   } else if (fAsf.length() > 0) {
      SaveHierarchy(fWorkerHierarchy.SaveToBuffer());
   }

   DestroyPar("Events");
}

bool stream::RunModule::ProcessNextEvent(void* evnt, unsigned evntsize)
{
   if (!fProcMgr)
      return false;

   fTotalEvnts++;

   if (fParallel == 0) Par("Events").SetValue(1);

   // TODO - later we need to use DABC buffer here to allow more complex
   // analysis when many dabc buffers required at the same time to analyze data

   base::Buffer bbuf;

   bbuf.makereferenceof(evnt, evntsize);

   bbuf().kind = base::proc_TRBEvent;
   bbuf().boardid = 0;
   bbuf().format = 0;

   fProcMgr->ProvideRawData(bbuf, fFastMode);

   if (fFastMode)
      return true;

   base::Event *outevent = nullptr;

   // scan new data
   bool new_event = fProcMgr->AnalyzeNewData(outevent);

   while (new_event) {
      fTotalOutEvnts++;

      fProcMgr->ProcessEvent(outevent);

      new_event = fProcMgr->ProduceNextEvent(outevent);
   }

   delete outevent;

   return true;
}


bool stream::RunModule::ProcessNextBuffer()
{
   if (fProcMgr && !fProcMgr->IsWorking()) return false;

   dabc::Buffer buf = Recv();

   if (fParallel == 0) Par("DataRate").SetValue(buf.GetTotalSize()/1024./1024.);

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      if (fParallel < 0) {
         std::string main = GetName();
         main.resize(main.length()-3);
         dabc::mgr.FindModule(main).Submit(dabc::Command("SlaveFinished"));
      }
      return true;
   }

   fTotalSize += buf.GetTotalSize();

   if (buf.GetTypeId() == mbs::mbt_MbsEvents) {
      mbs::ReadIterator iter(buf);
      while (iter.NextEvent()) {
         if (iter.NextSubEvent())
            ProcessNextEvent(iter.rawdata(), iter.rawdatasize());
      }
   } else {
      hadaq::ReadIterator iter(buf);
      while (iter.NextEvent()) {
         ProcessNextEvent(iter.evnt(), iter.evntsize());
      }
   }

   return true;
}

void stream::RunModule::GenerateEOF(dabc::Buffer buf)
{
   if ((fStopMode != -1111) || (fParallel <= 0)) return;

   DOUT0("Inject EOF to finish parallel jobs");

   SendToAllOutputs(buf);

   fStopMode = fParallel;
   SendToAllOutputs(buf);
}

bool stream::RunModule::RedistributeBuffers()
{
   while (CanRecv()) {

      unsigned indx = 0, max = 0, min = 10;
      for (unsigned n = 0; n < NumOutputs(); n++) {
         unsigned cansend = NumCanSend(n);
         if (cansend > max) { max = cansend; indx = n; }
         if (cansend < min) min = cansend;
      }

      // one need at least one output to be able send something
      if (max == 0) return false;

      // in case of EOF one need that all outputs can accept at least one buffer
      if ((RecvQueueItem().GetTypeId() == dabc::mbt_EOF) && (min == 0))
         return false;

      dabc::Buffer buf = Recv();

      if (buf.GetTypeId() == dabc::mbt_EOF) {
         GenerateEOF(buf);
         return false;
      }

      fTotalSize += buf.GetTotalSize();

      int cnt = 0;

      if (buf.GetTypeId() == mbs::mbt_MbsEvents) {
         mbs::ReadIterator iter(buf);
         while (iter.NextEvent()) cnt++;
      } else {
         hadaq::ReadIterator iter(buf);
         while (iter.NextEvent()) cnt++;
      }

      fTotalEvnts+=cnt;

      Par("Events").SetValue(cnt);

      //   DOUT0("Send buffer to output %d\n", indx);

      Send(indx, buf);
   }

   // all possible buffers are processed, no reason to invoke method once again
   return false;
}

bool stream::RunModule::ProcessRecv(unsigned)
{
   if (fParallel <= 0)
      return ProcessNextBuffer();

   return RedistributeBuffers();
}

void stream::RunModule::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) == "AutoSave") {
      if (fProcMgr) fProcMgr->SaveAllHistograms();
      return;
   }

   if (TimerName(timer) == "KeepAlive") {
      // std::string s = dabc::format("numcanrecv %u isconnected %s cansend", NumCanRecv(), DBOOL(IsPortConnected(InputName())));
      // for (unsigned n=0;n<NumOutputs();n++)
      //    s.append(dabc::format(" %u:%u", n, NumCanSend(n)));
      // DOUT0("keep alive %s", s.c_str());

      RedistributeBuffers();

      if ((fTotalEvnts > 0) && !IsPortConnected(InputName())) {
         dabc::Buffer buf = TakeBuffer();
         buf.SetTypeId(dabc::mbt_EOF);
         GenerateEOF(buf);
      }

      return;
   }


   hadaq::HldProcessor *hld = dynamic_cast<hadaq::HldProcessor*> (fProcMgr->FindProc("HLD"));
   if (!hld) return;

   dabc::Hierarchy folder = fWorkerHierarchy.FindChild("Status");

   folder.SetField("EventsRate", Par("Events").GetField("value").AsDouble());
   folder.SetField("EventsCount", (int64_t) fTotalEvnts);
   folder.SetField("StoreInfo", fProcMgr->GetStoreInfo());

   for (unsigned n = 0; n < hld->NumberOfTRB(); n++) {
      hadaq::TrbProcessor *trb = hld->GetTRB(n);
      if (!trb) continue;

      dabc::Hierarchy item = folder.CreateHChild(trb->GetName());

      dabc::Hierarchy logitem;

      TdcCalibrationModule::SetTRBStatus(item, logitem, trb);
   }

}


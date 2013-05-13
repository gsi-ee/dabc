#include "roc/SplitterModule.h"

#include <stdlib.h>

#include "dabc/Buffer.h"
#include "dabc/Pointer.h"
#include "dabc/Parameter.h"
#include "dabc/Manager.h"
#include "dabc/Command.h"

#include "roc/Board.h"
#include "roc/Iterator.h"
#include "roc/Commands.h"


roc::SplitterModule::SplitterModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   EnsurePorts(1, 2);

   fFlushTime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(0.3);
   bool sdebug = cmd.GetBool("SplitterDebug", false);

   CreatePar("InpData").SetRatemeter(false, 3.).SetDebugOutput(sdebug);
   CreatePar("OutData").SetRatemeter(false, 3.).SetDebugOutput(sdebug);

   SetPortRatemeter(InputName(), Par("InpData"));

   for(unsigned n=0; n<NumOutputs(); n++)
      SetPortRatemeter(OutputName(n), Par("OutData"));

   CreateTimer("FlushTimer", (fFlushTime>0 && fFlushTime<0.1) ? fFlushTime : 0.1);

   DOUT0("Splitter module created: name:%s numout:%u flush:%4.2fs", GetName(), NumOutputs(), fFlushTime);
}

roc::SplitterModule::~SplitterModule()
{
}

void roc::SplitterModule::BeforeModuleStart()
{
}

bool roc::SplitterModule::ProcessSend(unsigned port)
{
   return FlushNextBuffer();
}

bool roc::SplitterModule::ProcessRecv(unsigned port)
{
   return FlushNextBuffer();
}

void roc::SplitterModule::ProcessTimerEvent(unsigned timer)
{
//   bool isany = false;
//   while (FlushNextBuffer()) isany = true;
//   if (!isany) CheckBuffersFlush();

   CheckBuffersFlush();
}

int roc::SplitterModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("AddROC")) {

      int rocid = cmd.GetInt("ROCID", -1);

      if ((rocid<0) || (fMap.size()>=NumOutputs())) return dabc::cmd_false;

      OutputRec rec;
      rec.nout = fMap.size();

      DOUT0("Splitter output %u assigned for ROC%d", rec.nout, rocid);

      fMap[rocid] = rec;
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

bool roc::SplitterModule::FlushNextBuffer()
{
   if (!CanSendToAllOutputs()) return false;

   if (!CanRecv()) return false;

   dabc::Buffer buf = Recv();

   if (buf.null()) return false;

   if ((buf.GetTypeId() < roc::rbt_RawRocData) ||
         (buf.GetTypeId() > roc::rbt_RawRocData + roc::formatNormal)) {
      buf.Release(); // not necessary, but keep it
      EOUT("Something wrong with buffer type");
      return false;
   }

   int fmt = (buf.GetTypeId() - roc::rbt_RawRocData);

   if (fmt!=roc::formatOptic2) {
      EOUT("Non-optic format not supported!");
      buf.Release(); // not necessary, but keep it
      return false;
   }

   OutputMap::iterator miter;
   for (miter=fMap.begin(); miter != fMap.end(); miter++) {
      miter->second.wassent = false;
   }
   bool isanysent = false;

   // FIXME: use Pointer class to access data from the buffer
   uint16_t* src = (uint16_t*) buf.SegmentPtr();
   unsigned srclen = buf.GetTotalSize() / 8;

//   DOUT0("Receive buffer nummsg:%u", srclen);

   while (srclen>0) {
      miter = fMap.find(*src);
      if (miter == fMap.end()) {
         EOUT("Non existing board number %u", *src);
         src+=4;
         srclen--;
         continue;
      }

      if (miter->second.buf.null()) {
         dabc::Buffer mbuf = TakeBuffer();
         if (mbuf.null()) {
            EOUT("No buffer for data from %u - skip data", *src);
            src+=4;
            srclen--;
            continue;
         }
         mbuf.SetTypeId(buf.GetTypeId());

         miter->second.ptr = mbuf;
         miter->second.buf << mbuf;
         miter->second.len = 0;
      }

      miter->second.ptr.copyfrom(src, 8);
      miter->second.len+=8;
      miter->second.ptr.shift(8);

      if (miter->second.len > miter->second.buf.GetTotalSize() - 8) {
         miter->second.buf.SetTotalSize(miter->second.len);
         Send(miter->second.nout, miter->second.buf);
         //         fOutRate->AccountValue(miter->second.len / 1024./1024.);
         //         DOUT0("Send to Output%u buffer len %u", miter->second.nout, miter->second.len);
         miter->second.buf.Release();
         miter->second.len = 0;
         miter->second.ptr.reset();
         miter->second.lasttm = dabc::Now();
         miter->second.wassent = true;
         isanysent = true;
      }

      src+=4; // !!!!! we are using uint16_t, therefore +4 !!!!!!!!
      srclen--;
   }


   buf.Release(); // not necessary, but keep it

   CheckBuffersFlush(isanysent);

   return true;
}

void roc::SplitterModule::CheckBuffersFlush(bool forceunsent)
{
   OutputMap::iterator miter;

   dabc::TimeStamp now = dabc::Now();

   for (miter=fMap.begin(); miter != fMap.end(); miter++) {

      // variable indicate that we should send buffer while some other data was flushed
      bool sendanyway = forceunsent && !miter->second.wassent;

      miter->second.wassent = false;

      // if no data was filled keep buffer for next data
      if ((miter->second.len == 0) || miter->second.buf.null()) continue;

      // if there is too few data or flush timeout not yet happen keep buffers
      if (!sendanyway && (miter->second.len <= miter->second.buf.GetTotalSize() - 8) &&
         ((fFlushTime<=0.) || ((now - miter->second.lasttm) < fFlushTime))) continue;

      // if output queue is full, do not try
      if (!CanSend(miter->second.nout)) continue;

      miter->second.buf.SetTotalSize(miter->second.len);
      Send(miter->second.nout, miter->second.buf);
//      fOutRate->AccountValue(miter->second.len / 1024./1024.);
//      DOUT0("Send to Output%u buffer len %u", miter->second.nout, miter->second.len);
      miter->second.buf.Release(); // no need but keep it
      miter->second.len = 0;
      miter->second.ptr.reset();
      miter->second.lasttm = now;
   }
}


void roc::SplitterModule::AfterModuleStop()
{
   OutputMap::iterator miter;

   for (miter=fMap.begin(); miter != fMap.end(); miter++) {
      miter->second.buf.Release();
      miter->second.len = 0;
   }
}

extern "C" void InitSplitter()
{
   if (dabc::mgr.null()) {
      EOUT("Manager is not created");
      exit(1);
   }

   dabc::mgr.CreateMemoryPool(roc::xmlRocPool);

   dabc::CmdCreateDevice cmd1(roc::typeAbbDevice, "AbbDev");
   cmd1.SetStr("Thread", "AbbThrd");
   cmd1.SetStr(roc::xmlBoardAddr, "abb");
   cmd1.SetStr(roc::xmlRole, base::roleToString(base::roleDAQ));

   if (!dabc::mgr.Execute(cmd1)) {
      EOUT("Cannot create ABB device");
      exit(1);
   }

   dabc::CmdCreateModule cmd2("roc::SplitterModule", "Splitter");
   cmd2.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
   cmd2.SetInt(dabc::xmlNumOutputs, 2);
   cmd2.SetBool("SplitterDebug", true);
   if (!dabc::mgr.Execute(cmd2)) {
      EOUT("Cannot create Splitter module");
      exit(1);
   }
   dabc::ModuleRef m = dabc::mgr.FindModule("Splitter");
   if (m.null()) {
      EOUT("No Splitter module");
      exit(1);
   }

   dabc::CmdCreateTransport cmd3("Splitter/Input", "AbbDev", "AbbThrd");
   cmd3.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
   cmd3.SetStr(roc::xmlBoardAddr, "abb");

   if (!dabc::mgr.Execute(cmd3)) {
      EOUT("Cannot connect splitter module to ABB device");
      exit(1);
   }

   dabc::Command cmd4("AddROC");
   cmd4.SetInt("ROCID", 0);
   if (!m.Execute(cmd4)) {
      EOUT("Cannot ADD ROCID 0 to splitter");
      exit(1);
   }

   dabc::Command cmd5("AddROC");
   cmd5.SetInt("ROCID", 1);
   if (!m.Execute(cmd5)) {
      EOUT("Cannot ADD ROCID 1 to splitter");
      exit(1);
   }
}




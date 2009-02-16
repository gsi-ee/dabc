/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "mbs/CombinerModule.h"

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Manager.h"

mbs::CombinerModule::CombinerModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0),
   fBufferSize(0),
   fInp(),
   fOut(0),
   fOutBuf(0),
   fTmCnt(0),
   fFileOutput(false),
   fServOutput(false)
{

   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16384, cmd);

   fPool = CreatePoolHandle(dabc::xmlWorkPool, fBufferSize, 10);

   int numinp = GetCfgInt(dabc::xmlNumInputs, 2, cmd);

   fFileOutput = GetCfgBool("DoFile", false, cmd);
   fServOutput = GetCfgBool("DoServer", false, cmd);

   double flashtmout = GetCfgDouble(dabc::xmlFlashTimeout, 1., cmd);

   for (int n=0;n<numinp;n++) {
      CreateInput(FORMAT(("Input%d", n)), fPool, 5);
      fInp.push_back(ReadIterator(0));
   }

   CreateOutput("Output", fPool, 5);
   if (fFileOutput) CreateOutput("FileOutput", fPool, 5);
   if (fServOutput) CreateOutput("ServerOutput", fPool, 5);

   if (flashtmout>0.) CreateTimer("Flash", flashtmout, false);
}

mbs::CombinerModule::~CombinerModule()
{
   if (fOutBuf!=0) {
      fOut.Close();
      dabc::Buffer::Release(fOutBuf);
      fOutBuf = 0;
   }
}

void mbs::CombinerModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if (fTmCnt > 0) fTmCnt--;
   if (fTmCnt == 0) FlushBuffer();
}

void mbs::CombinerModule::ProcessInputEvent(dabc::Port* port)
{
   while (BuildEvent());
}

void mbs::CombinerModule::ProcessOutputEvent(dabc::Port* port)
{
   while (BuildEvent());
}

bool mbs::CombinerModule::FlushBuffer()
{
   if (fOutBuf==0) return false;

   if (fOut.IsEmpty()) return false;

   if (!CanSendToAllOutputs()) return false;

   fOut.Close();

   SendToAllOutputs(fOutBuf);

   fTmCnt = 2; // set 2 means that two timeout events should happen before flush will be triggered

   fOutBuf = 0;

   return true;
}

bool mbs::CombinerModule::BuildEvent()
{
   mbs::EventNumType mineventid = 0;
   mbs::EventNumType maxeventid = 0;

   for (unsigned ninp=0; ninp<NumInputs(); ninp++) {

      if (!fInp[ninp].IsBuffer()) {

         dabc::Port* port = Input(ninp);

         while (port->CanRecv()) {
            if (fInp[ninp].Reset(port->FirstInputBuffer()))
               if (fInp[ninp].NextEvent()) break;

            fInp[ninp].Reset(0);
            port->SkipInputBuffers(1);
         }

         // if no buffer is possible to assign, break
         if (!fInp[ninp].IsBuffer()) return false;
      }

      mbs::EventNumType evid = fInp[ninp].evnt()->EventNumber();

      if (ninp==0) {
         mineventid = evid;
         maxeventid = evid;
      } else {
         if (mineventid < evid) mineventid = evid; else
         if (maxeventid > evid) maxeventid = evid;
      }
   }

   mbs::EventNumType buildevid = mineventid;
   // treat correctly situation of event id overflow
   if (maxeventid!=mineventid)
      if (maxeventid - mineventid > 0x10000000) buildevid = mineventid;

   uint32_t subeventssize = 0;

   for (unsigned ninp=0; ninp<NumInputs(); ninp++)
      if (fInp[ninp].evnt()->EventNumber() == buildevid)
         subeventssize += fInp[ninp].evnt()->SubEventsSize();

   if ((fOutBuf!=0) && !fOut.IsPlaceForEvent(subeventssize))
      if (!FlushBuffer()) return false;

   if (fOutBuf==0) {
      fOutBuf = fPool->TakeBufferReq(fBufferSize);
      if (fOutBuf == 0) return false;

      if (!fOut.Reset(fOutBuf)) {
         EOUT(("Cannot use buffer for output - hard error!!!!"));
         dabc::Buffer::Release(fOutBuf);
         return false;
      }
   }

   if (!fOut.IsPlaceForEvent(subeventssize))
      EOUT(("Event size %u too big for buffer %u, skip event %u", subeventssize+ sizeof(mbs::EventHeader), fBufferSize, buildevid));
   else {

      DOUT0(("Build event %u", buildevid));

      fOut.NewEvent(buildevid);
      dabc::Pointer ptr;
      bool isfirst = true;
      for (unsigned ninp=0; ninp<NumInputs(); ninp++)
         if (fInp[ninp].evnt()->EventNumber() == buildevid) {
            if (isfirst) {
               fOut.evnt()->CopyHeader(fInp[ninp].evnt());
               isfirst = false;
            }

            fInp[ninp].AssignEventPointer(ptr);
            ptr.shift(sizeof(mbs::EventHeader));
            fOut.AddSubevent(ptr);
         }
      fOut.FinishEvent();

      // if output buffer filled already, flush it immediately
      if (!fOut.IsPlaceForEvent(0)) FlushBuffer();
   }

   for (unsigned ninp=0; ninp<NumInputs(); ninp++)
      if (fInp[ninp].evnt()->EventNumber() == buildevid)
         if (!fInp[ninp].NextEvent()) {
            fInp[ninp].Reset(0); // forgot about buffer
            Input(ninp)->SkipInputBuffers(1);
         }

   return true;
}


// _________________________________________________________________________


extern "C" void StartMbsCombiner()
{
    if (dabc::mgr()==0) {
       EOUT(("Manager is not created"));
       exit(1);
    }

    DOUT0(("Start MBS combiner module"));

    mbs::CombinerModule* m = new mbs::CombinerModule("Combiner");
    dabc::mgr()->MakeThreadForModule(m);

    for (unsigned n=0;n<m->NumInputs();n++)
       if (!dabc::mgr()->CreateTransport(FORMAT(("Combiner/Input%u", n)), mbs::typeClientTransport, "MbsTransport")) {
          EOUT(("Cannot create MBS client transport"));
          exit(1);
       }

    if (m->IsServOutput()) {
       if (!dabc::mgr()->CreateTransport("Combiner/ServerOutput", mbs::typeServerTransport, "MbsOutTransport")) {
          EOUT(("Cannot create MBS server"));
          exit(1);
       }
    }

    if (m->IsFileOutput())
       if (!dabc::mgr()->CreateTransport("Combiner/FileOutput", mbs::typeLmdOutput, "MbsOutTransport")) {
          EOUT(("Cannot create MBS file output"));
          exit(1);
       }

    m->Start();

    DOUT0(("Start MBS combiner module done"));
}


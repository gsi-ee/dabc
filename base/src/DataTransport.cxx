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

#include "dabc/DataTransport.h"

#include "dabc/Buffer.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"


dabc::InputTransport::InputTransport(dabc::Command cmd, const PortRef& inpport, DataInput* inp, bool owner, WorkerAddon* addon) :
   dabc::Transport(cmd, inpport, 0),
   fInput(inp),
   fInputOwner(owner),
   fInpState(inpInit),
   fCurrentBuf(),
   fNextDataSize(0),
   fPoolChangeCounter(0),
   fPoolRef(),
   fExtraBufs(0)
{
   AssignAddon(addon);
   CreateTimer("SysTimer");
//   DOUT0("Create InputTransport %s", GetName());
}

dabc::InputTransport::~InputTransport()
{
   DOUT2("Destroy InputTransport %s", GetName());
   CloseInput();
}

void dabc::InputTransport::RequestPoolMonitoring()
{
   fPoolRef = dabc::mgr.FindPool(PoolName());
   if (fPoolRef.null())
      EOUT("Cannot find memory pool, associated with handle %s, monitoring will not work", PoolName().c_str());
}


bool dabc::InputTransport::StartTransport()
{
   Transport::StartTransport();

   DOUT2("============================= Start InputTransport %s isrunning %s", ItemName().c_str(), DBOOL(IsRunning()));

   // if we are doing input transport,
   // generate artificial event for the port to start transport
   // TODO: should it be default for the module itself???
   if (fInput==0) {
      EOUT("Input object is not assigned");

      return false;
   }


   fInpState = inpInit;
   fNextDataSize = 0;
   ProduceOutputEvent();

   return true;
}

bool dabc::InputTransport::StopTransport()
{
//   DOUT0("Stopping InputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   bool res = Transport::StopTransport();

   DOUT2("Stopping InputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   return res;
}

bool dabc::InputTransport::ProcessBuffer(unsigned pool)
{
   // we only need buffer when we explicitly request it

   if (fInpState != inpWaitBuffer) return false;

   // only when buffer is awaited, react on buffer event and continue
   fCurrentBuf = TakeBuffer(pool);

//   DOUT0("@@@@@@@@@@ Process buffer null %s size %u", DBOOL(fCurrentBuf.null()), fCurrentBuf.GetTotalSize());

   fInpState = inpCheckBuffer;
   ProcessOutputEvent();

   // we are interesting for next buffer event if we really waiting for the buffer
   return fInpState == inpWaitBuffer;
}

void dabc::InputTransport::CloseInput()
{
   if ((fInput!=0) && fInputOwner) {
      // printf("keep input\n");
      delete fInput;
   }
   fInput = 0;
   fInputOwner = false;
}

void dabc::InputTransport::ObjectCleanup()
{

   if (fInpState != inpClosed) {
      CloseInput();
      fInpState = inpClosed;
   }

   fCurrentBuf.Release();

   fPoolRef.Release();

   dabc::Transport::ObjectCleanup();
}


void dabc::InputTransport::ProcessTimerEvent(unsigned timer)
{
   if (fInpState == inpInitTimeout)
      fInpState = inpInit;

   if (fInpState == inpComplitTimeout)
      fInpState = inpCompliting;

   ProcessOutputEvent();
}

void dabc::InputTransport::Read_CallBack(unsigned sz)
{
   // this inform that we are
   if (sz == dabc::di_QueueBufReady) {
      // in case when transport waiting for next buffer, switch state to receiving of next ready buffer
      if ((fInpState == inpWaitBuffer) && (fExtraBufs>0))
         fInpState = inpCallBack;
   }

   switch (fInpState) {

      case inpInit:
         if (fExtraBufs==0) {
            EOUT("Call back at init state not with extra mode");
            exit(333);
         }
         fInpState = inpCompliting;
         break;

      case inpSizeCallBack:
         fInpState = inpCheckSize;
         // DOUT0("dabc::InputTransport::Get request for buffer %u", sz);
         fNextDataSize = sz;
         break;

      case inpCallBack:
         fInpState = inpCompliting;
         break;

      default:
         EOUT("Get callback at wrong state %d", fInpState);
         fInpState = inpError;
   }

   ProcessOutputEvent();
}

bool dabc::InputTransport::ProcessSend(unsigned port)
{
   // DOUT0("dabc::InputTransport  %s  ProcessSend state %d", ItemName().c_str(), fInpState);

   // if transport was already closed, one should ignore any other events
   if (fInpState == inpClosed) return false;

   if (NumPools()==0) {
      EOUT("InputTransport %s - no memory pool!!!!", GetName());
      CloseTransport(true);
      return false;
   }

   if (fInput==0) {
      EOUT("InputTransport %s - no input object!!!!", GetName());
      CloseTransport(true);
      return false;
   }

   if ((fInpState == inpSizeCallBack) || (fInpState == inpInitTimeout) || (fInpState == inpComplitTimeout)) {
      // at these states one should get event from other source first
      return false;
   }

//   EOUT("Tr:%08x Process event", this);



   // first step - request size of the buffer

   if (fInpState == inpInit) {

      // if internal queue already acquire as many buffers, wait
      if (fExtraBufs && (NumCanSend(port) <= fExtraBufs)) {
//            DOUT0("There are too many buffers in the transport queue - start wait for the call-back buf %u", fCurrentBuf.GetTotalSize());
         fInpState = inpCallBack;
         return false;
      }

      fNextDataSize = fInput->Read_Size();

      // this is case when input want to repeat operation
      // when we return true, we say that we want to continue processing

      switch (fNextDataSize) {
         case di_Repeat:
            return true;
         case di_RepeatTimeOut:
            fInpState = inpInitTimeout;
            ShootTimer("SysTimer", fInput->Read_Timeout());
            return false;
         case di_CallBack:
            fInpState = inpSizeCallBack;
            fNextDataSize = 0;
            return false;
      }

      fInpState = inpCheckSize;
   }

   if (fInpState == inpCheckSize) {

//      DOUT0("InputTransport process fInpState == inpCheckSize sz %u", fNextDataSize);

      switch (fNextDataSize) {

         case di_CallBack:
            EOUT("Wrong place for callback");
            fInpState = inpError;
            break;

         case di_EndOfStream:
            fInpState = inpEnd;
            break;

         case di_DfltBufSize:
            fInpState = inpNeedBuffer;
            fNextDataSize = 0;
            break;

         default:
            if (fNextDataSize <= di_ValidSize)
               fInpState = inpNeedBuffer;
            else {
               EOUT("Reading error");
               fInpState = inpError;
            }
      }
   }

   // here we request buffer

   if (fInpState == inpNeedBuffer) {

     if (!fPoolRef.null() && fPoolRef()->CheckChangeCounter(fPoolChangeCounter))
         ProcessPoolChanged(fPoolRef());

      fCurrentBuf = TakeBuffer();

//      DOUT0("input transport taking buffer null %s size %u autopool %s connected %s", DBOOL(fCurrentBuf.null()), fCurrentBuf.GetTotalSize(), DBOOL(IsAutoPool()), DBOOL(IsPoolConnected()));

      if (!fCurrentBuf.null()) {
         fInpState = inpCheckBuffer;
      } else
      if (IsAutoPool()) {
         fInpState = inpWaitBuffer;
         return false;
      } else {
         EOUT("Did not get buffer and pool queue is not configured - use minimal timeout");
         ShootTimer("SysTimer", 0.001);
         return false;
      }
   }

   if (fInpState == inpCheckBuffer) {

//      DOUT0("Check buffer null %s size %u", DBOOL(fCurrentBuf.null()), fCurrentBuf.GetTotalSize());
      // if buffer was provided, use it
      if (fCurrentBuf.GetTotalSize() < fNextDataSize) {
         EOUT("Requested buffer smaller than actual data size");
         fInpState = inpError;
      } else {
         if (fNextDataSize>0) fCurrentBuf.SetTotalSize(fNextDataSize);
         fInpState = inpHasBuffer;
      }
   }

   if (fInpState == inpHasBuffer) {
//      DOUT0("Read_Start buf %u", fCurrentBuf.GetTotalSize());
      unsigned start_res = fInput->Read_Start(fCurrentBuf);
//      DOUT0("Read_Start res = %x buf %u", start_res, fCurrentBuf.GetTotalSize());
      switch (start_res) {
         case di_Ok:
            // this will allows to call Read_Complete method in next iteration
            fInpState = inpCompliting;
            break;
         case di_CallBack:
            // if we starts callback, just not fire event
            fInpState = inpCallBack;
            return false;

         case di_NeedMoreBuf:
         case di_HasEnoughBuf:
            // this is case when transport internally uses queue and need more buffers
            fExtraBufs++;

            if (start_res == di_NeedMoreBuf) {
               fInpState = inpInit;
               return true;
            } else {
               fInpState = inpCallBack;
               return false;
            }

         default:
            fInpState = inpError;
      }
   }


   if (fInpState == inpCallBack) {
      // this is state when transport fills buffer and should invoke CallBack
      return false;
   }

   if (fInpState == inpCompliting) {

      if (fExtraBufs && !fCurrentBuf.null()) {
         EOUT("Internal error - currbuf not null when compliting");
         return false;
      }

      if (!fExtraBufs && fCurrentBuf.null()) {
         EOUT("Internal error - currbuf null when completing");
         return false;
      }

      unsigned res = fInput->Read_Complete(fCurrentBuf);

      if (fExtraBufs) {
         if (fCurrentBuf.null()) EOUT("Transport does not return buffer!!!");
         fExtraBufs--;
      }

      switch (res) {
         case di_Ok:
            //
            fInpState = inpReady;
            break;
         case di_MoreBufReady:
            // we send immediately buffer and will try to take more buffers out of transport
            Send(fCurrentBuf);
            return true;
         case di_SkipBuffer:
            fCurrentBuf.Release();
            DOUT4("Skip input buffer");
            fInpState = inpInit;
            break;
         case di_EndOfStream:
            fCurrentBuf.Release();
            DOUT4("End of stream");
            fInpState = inpEnd;
            break;
         case di_Repeat:
            return true;
         case di_RepeatTimeOut:
            fInpState = inpComplitTimeout;
            ShootTimer("SysTimer", fInput->Read_Timeout());
            return false;
         default:
            EOUT("Error when do buffer reading res = %d", res);
            fInpState = inpError;
      }
   }

   if (fInpState == inpReady) {
      // DOUT0("Input transport sends buf %u", (unsigned) fCurrentBuf.SegmentId(0));

      Send(fCurrentBuf);
      fCurrentBuf.Release();
      fInpState = inpInit;
   }

   if ((fInpState == inpError) || (fInpState == inpEnd)) {

      DOUT2("InputTransport:: Generate EOF packet");
      CloseInput();
      fNextDataSize = 0;

      fCurrentBuf.MakeEmpty();

      if (fCurrentBuf.null()) {
         EOUT("Fatal error - cannot get empty buffer, try after 1 sec");
         ShootTimer("SysTimer", 1.);
         return false;
      } else {
         fCurrentBuf.SetTypeId(dabc::mbt_EOF);
         Send(fCurrentBuf);
         fInpState = inpClosed;
      }
   }

   if (fInpState == inpClosed) {
      CloseTransport(false);
      return false;
   }

   return true;
}

// ====================================================================================

dabc::OutputTransport::OutputTransport(dabc::Command cmd, const PortRef& outport, DataOutput* out, bool owner, WorkerAddon* addon) :
   dabc::Transport(cmd, 0, outport),
   fOutput(out),
   fOutputOwner(owner),
   fState(outInit),
   fCurrentBuf()
{
   AssignAddon(addon);
   CreateTimer("SysTimer");

   if (!fTransportInfoName.empty() && fOutput)
      fOutput->SetInfoParName(fTransportInfoName);
}

dabc::OutputTransport::~OutputTransport()
{
   CloseOutput();
}

void dabc::OutputTransport::CloseOutput()
{
   if ((fOutput!=0) && fOutputOwner)
      delete fOutput;

   fOutput = 0;
   fOutputOwner = false;

   fCurrentBuf.Release();
}


bool dabc::OutputTransport::StartTransport()
{
   DOUT2("Starting OutputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   Transport::StartTransport();

   if (fOutput==0) {
      EOUT("Output was not specified!!!");
      return false;
   }

   return true;
}

bool dabc::OutputTransport::StopTransport()
{
   DOUT2("Stopping OutputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   return Transport::StopTransport();
}


void dabc::OutputTransport::ObjectCleanup()
{
   CloseOutput();

   dabc::Transport::ObjectCleanup();
}

void dabc::OutputTransport::ProcessEvent(const EventId& evnt)
{
 //  DOUT0("%s dabc::OutputTransport::ProcessEvent %u  state %u", GetName(), (unsigned) evnt.GetCode(), fState);

   if (evnt.GetCode() == evCallBack) {

      if (evnt.GetArg() != do_Ok) {
         EOUT("Callback with error argument");
         fState = outClosing;
         CloseOutput();
         CloseTransport(true);
         return;
      }

      if (fState == outWaitCallback) {
         fState = outInit;
         ProcessInputEvent();
         return;
      }

      if (fState == outWaitFinishCallback) {
         fState = outFinishWriting;

         // we need to call ProcessRecv directly at least once before entering into normal loop
         if (ProcessRecv(0))
            ProcessInputEvent();

         return;
      }

      EOUT("Call-back at wrong state!!");
   }

   dabc::Transport::ProcessEvent(evnt);
}

bool dabc::OutputTransport::ProcessRecv(unsigned port)
{
//   if (IsName("_OnlineServer_Output0_Transport_Slave0_Transport"))
//      DOUT0("dabc::OutputTransport::ProcessRecv  %s state %u", GetName(), fState);

   if (fOutput==0) {
      EOUT("Output object not specified");
      fState = outError;
   }

   if (fState == outInit) {

      unsigned ret(do_Ok);

      unsigned buftyp = RecvQueueItem(port,0).GetTypeId();

      if (buftyp == dabc::mbt_EOF) {
         DOUT0("EOF - close output transport");
         Recv(port).Release();
         ret = do_Close;
      } else
      if (buftyp == dabc::mbt_EOL) {
         fOutput->Write_Flush();
         Recv(port).Release();
         return true;
      } else {
         ret = fOutput->Write_Check();
      }

      switch (ret) {
         case do_Ok:
            fState = outStartWriting;
            break;
         case do_Repeat:
            return true;
         case do_RepeatTimeOut:
            fState = outInitTimeout;
            ShootTimer("SysTimer", fOutput->Write_Timeout());
            return false;
         case do_CallBack:
            fState = outWaitCallback;
            return false;
         case do_Skip:
            Recv(port).Release();
            return true;
         case do_Close:
            fState = outClosing;
            break;
         case do_Error:
            fState = outError;
            break;
         default:
            EOUT("Wrong return value %u for the Write_Check", ret);
            fState = outError;
      }
   }

   if (fState == outInitTimeout) {
      return false;
   }

   if (fState == outWaitCallback) {
      return false;
   }

   if (fState == outStartWriting) {

      fCurrentBuf = Recv(port);

      unsigned ret = fOutput->Write_Buffer(fCurrentBuf);

      switch (ret) {
         case do_Ok:
            fState = outFinishWriting;
            break;
         case do_CallBack:
            fState = outWaitFinishCallback;
            return false;
         case do_Skip:
            fState = outInit;
            return true;
         case do_Close:
            fState = outClosing;
            break;
         case do_Error:
            EOUT("Error when writing buffer");
            fState = outError;
            break;
         default:
            EOUT("Wrong return value %u for the Write_Buffer", ret);
            fState = outError;
      }
   }

   if (fState == outWaitFinishCallback) {
      // if we wait for call back, ignore all possible events
      return false;
   }

   if (fState == outFinishWriting) {

      fCurrentBuf.Release();

      unsigned ret = fOutput->Write_Complete();

      switch (ret) {
         case do_Ok:
            fState = outInit;
            break;
         case do_Close:
            fState = outClosing;
            break;
         case do_Error:
            fState = outError;
            break;
         default:
            EOUT("Wrong return value %u for the Write_Complete", ret);
            fState = outError;
      }
   }

   if (fState == outClosing) {
      fState = outClosed;
      CloseOutput();
      CloseTransport(false);
      return false;
   }

   if (fState == outError) {
      fState = outClosed;
      CloseOutput();
      CloseTransport(true);
      return false;
   }

   if (fOutput && InfoExpected()) {
      std::string info = fOutput->ProvideInfo();
      ProvideInfo(0, info);
   }

   return true;
}

void dabc::OutputTransport::ProcessTimerEvent(unsigned timer)
{
   if (fState == outInitTimeout)
      fState = outInit;

   ProcessInputEvent();
}



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


dabc::InputTransport::InputTransport(dabc::Command cmd, const PortRef& inpport, DataInput* inp, bool owner) :
   dabc::Transport(cmd, inpport, 0),
   fInput(0),
   fInputOwner(false),
   fInpState(inpInit),
   fCurrentBuf(),
   fNextDataSize(0),
   fPoolChangeCounter(0),
   fPoolRef(),
   fExtraBufs(0),
   fReconnect(),
   fStopRequested(false)
{
   if (inp!=0) SetDataInput(inp, owner);

   CreateTimer("SysTimer");
//   DOUT5("Create InputTransport %s", GetName());
}

dabc::InputTransport::~InputTransport()
{
}

void dabc::InputTransport::SetDataInput(DataInput* inp, bool owner)
{
   CloseInput();

   if (inp==0) return;

   fInput = inp;
   fInputOwner = false;
   WorkerAddon* addon = inp->Read_GetAddon();

   if (addon==0) {
      fInputOwner = owner;
   } else
   if (owner)
      AssignAddon(addon);
   else
      EOUT("Cannot assigned addon while owner flag is not specified");

}

void dabc::InputTransport::EnableReconnect(const std::string& reconn)
{
   fReconnect = reconn;
}


void dabc::InputTransport::RequestPoolMonitoring()
{
   fPoolRef = dabc::mgr.FindPool(PoolName());
   if (fPoolRef.null())
      EOUT("Cannot find memory pool, associated with handle %s, monitoring will not work", PoolName().c_str());
}


bool dabc::InputTransport::StartTransport()
{
   bool res = Transport::StartTransport();

   DOUT2("============================= Start InputTransport %s isrunning %s", ItemName().c_str(), DBOOL(IsRunning()));

   // if we are doing input transport,
   // generate artificial event for the port to start transport
   // TODO: should it be default for the module itself???
   if (fInput==0) {
      EOUT("Input object is not assigned");

      return false;
   }

   // clear any existing previous request
   fStopRequested = false;

   if (!SuitableStateForStartStop()) {
      EOUT("Start transport %s at not optimal state %u", GetName(), (unsigned) fInpState);
   }

   // fNextDataSize = 0;
   ProduceOutputEvent();

   return res;
}

bool dabc::InputTransport::StopTransport()
{
//   DOUT0("Stopping InputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   DOUT2("Stopping InputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));
   if (SuitableStateForStartStop()) {
      fStopRequested = false;
      return Transport::StopTransport();
   }

   if (!fStopRequested) {
      DOUT2("%s Try to wait until suitable state is achieved, now state = %u", GetName(), (unsigned) fInpState);
      fStopRequested = true;
      fAddon.Notify("TransportWantToStop");
   }
   return true;
}

bool dabc::InputTransport::ProcessBuffer(unsigned pool)
{
   // we only need buffer when we explicitly request it

   if (fInpState != inpWaitBuffer) return false;

   // only when buffer is awaited, react on buffer event and continue
   fCurrentBuf = TakeBuffer(pool);

//   DOUT0("@@@@@@@@@@ Process buffer null %s size %u", DBOOL(fCurrentBuf.null()), fCurrentBuf.GetTotalSize());

   ChangeState(inpCheckBuffer);
   ProcessOutputEvent(0);

   // we are interesting for next buffer event if we really waiting for the buffer
   return fInpState == inpWaitBuffer;
}

void dabc::InputTransport::CloseInput()
{
   if ((fInput!=0) && fInputOwner) {
      delete fInput;
   }
   fInput = 0;
   fInputOwner = false;

   AssignAddon(0);
}

void dabc::InputTransport::TransportCleanup()
{
   if (fInpState != inpClosed) {
      CloseInput();
      fInpState = inpClosed;
   }

   fCurrentBuf.Release();

   fPoolRef.Release();

   dabc::Transport::TransportCleanup();
}


void dabc::InputTransport::ProcessTimerEvent(unsigned timer)
{
   if (fInpState == inpInitTimeout)
      ChangeState(inpInit);

   if (fInpState == inpComplitTimeout)
      ChangeState(inpCompleting);

   ProcessOutputEvent(0);
}

int dabc::InputTransport::ExecuteCommand(Command cmd)
{
   bool isfailure = cmd.IsName(CmdDataInputFailed::CmdName());

   if (isfailure || cmd.IsName(CmdDataInputClosed::CmdName())) {

      if (fInpState != inpClosed) {
         CloseInput();
         ChangeState(inpClosed);
      }

      if (fReconnect.empty()) {
         CloseTransport(isfailure);
      } else {
         ChangeState(inpReconnect);
         ShootTimer("SysTimer", 1.);
      }

      return cmd_true;
   }

   if (cmd.IsName("GetTransportStatistic")) {
      // take statistic from output element
      if (fInput) fInput->Read_Stat(cmd);
      return cmd_true;
   }

   return dabc::Transport::ExecuteCommand(cmd);
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
         fInpState = inpCompleting;
         break;

      case inpSizeCallBack:
         fInpState = inpCheckSize;
         // DOUT0("dabc::InputTransport::Get request for buffer %u", sz);
         fNextDataSize = sz;
         break;

      case inpCallBack:
         fInpState = inpCompleting;
         break;

      default:
         EOUT("Get callback at wrong state %d", fInpState);
         fInpState = inpError;
   }

   ProcessOutputEvent(0);
}


void dabc::InputTransport::ChangeState(EInputStates state)
{
   fInpState = state;

   if (fStopRequested && SuitableStateForStartStop()) {
      DOUT2("%s Stop transport at suitable state", GetName());
      fStopRequested = false;
      Transport::StopTransport();
   }
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

   // if transport was created via device and device is destroyed - close transport as well
   if (fTransportDevice.DeviceDestroyed()) {
      if (fInpState != inpClosed) {
         CloseInput();
         ChangeState(inpClosed);
      }
      CloseTransport(false);
      return false;
   }

   if (fInpState == inpReconnect) {

      if (fInput!=0) {
         EOUT("Reconnect when input non 0 - ABORT");
         CloseTransport(true);
         return false;
      }

      if (fReconnect.empty()) {
         EOUT("Reconnect not specified - ABORT");
         CloseTransport(true);
         return false;
      }

      DataInput* inp = dabc::mgr.CreateDataInput(fReconnect);

      if (inp==0) {
         ShootTimer("SysTimer", 1.);
         return false;
      }

      SetDataInput(inp, true);

      ChangeState(inpInit);
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

      // if transport not running, do not start acquire new buffer
      if (!isTransportRunning()) return false;

      // if internal queue already acquire as many buffers, wait
      if (fExtraBufs && (NumCanSend(port) <= fExtraBufs)) {
//            DOUT0("There are too many buffers in the transport queue - start wait for the call-back buf %u", fCurrentBuf.GetTotalSize());
         ChangeState(inpCallBack);
         return false;
      }

      fNextDataSize = fInput->Read_Size();

      // this is case when input want to repeat operation
      // when we return true, we say that we want to continue processing

      switch (fNextDataSize) {
         case di_Repeat:
            return true;
         case di_RepeatTimeOut:
            ChangeState(inpInitTimeout);
            ShootTimer("SysTimer", fInput->Read_Timeout());
            return false;
         case di_CallBack:
            ChangeState(inpSizeCallBack);
            fNextDataSize = 0;
            return false;
      }

      ChangeState(inpCheckSize);
   }

   if (fInpState == inpCheckSize) {

//      DOUT0("InputTransport process fInpState == inpCheckSize sz %u", fNextDataSize);

      switch (fNextDataSize) {

         case di_CallBack:
            EOUT("Wrong place for callback");
            ChangeState(inpError);
            break;

         case di_EndOfStream:
            ChangeState(inpEnd);
            break;

         case di_DfltBufSize:
            ChangeState(inpNeedBuffer);
            fNextDataSize = 0;
            break;

         default:
            if (fNextDataSize <= di_ValidSize) {
               ChangeState(inpNeedBuffer);
            } else {
               DOUT0("Tr:%s Reading error", GetName());
               ChangeState(inpError);
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
         ChangeState(inpCheckBuffer);
      } else
      if (IsAutoPool()) {
         ChangeState(inpWaitBuffer);
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
         ChangeState(inpError);
      } else {
         if (fNextDataSize>0) fCurrentBuf.SetTotalSize(fNextDataSize);
         ChangeState(inpHasBuffer);
      }
   }

   if (fInpState == inpHasBuffer) {
//      DOUT0("Read_Start buf %u", fCurrentBuf.GetTotalSize());
      unsigned start_res = fInput->Read_Start(fCurrentBuf);
//      DOUT0("Read_Start res = %x buf %u", start_res, fCurrentBuf.GetTotalSize());
      switch (start_res) {
         case di_Ok:
            // this will allows to call Read_Complete method in next iteration
            ChangeState(inpCompleting);
            break;
         case di_CallBack:
            // if we starts callback, just not fire event
            ChangeState(inpCallBack);
            return false;

         case di_NeedMoreBuf:
         case di_HasEnoughBuf:
            // this is case when transport internally uses queue and need more buffers
            fExtraBufs++;

            if (start_res == di_NeedMoreBuf) {
               ChangeState(inpInit);
               return true;
            } else {
               ChangeState(inpCallBack);
               return false;
            }

         default:
            ChangeState(inpError);
      }
   }


   if (fInpState == inpCallBack) {
      // this is state when transport fills buffer and should invoke CallBack
      return false;
   }

   if (fInpState == inpCompleting) {

      if (fExtraBufs && !fCurrentBuf.null()) {
         EOUT("Internal error - currbuf not null when completing");
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
            ChangeState(inpReady);
            break;
         case di_MoreBufReady:
            // we send immediately buffer and will try to take more buffers out of transport
            Send(fCurrentBuf);
            return true;
         case di_SkipBuffer:
            fCurrentBuf.Release();
            DOUT4("Skip input buffer");
            ChangeState(inpInit);
            break;
         case di_EndOfStream:
            fCurrentBuf.Release();
            DOUT4("End of stream");
            ChangeState(inpEnd);
            break;
         case di_Repeat:
            return true;
         case di_RepeatTimeOut:
            ChangeState(inpComplitTimeout);
            ShootTimer("SysTimer", fInput->Read_Timeout());
            return false;
         default:
            EOUT("Error when do buffer reading res = %d", res);
            ChangeState(inpError);
      }
   }

   if (fInpState == inpReady) {
      // DOUT0("Input transport sends buf %u", (unsigned) fCurrentBuf.SegmentId(0));

      Send(fCurrentBuf);
      fCurrentBuf.Release();
      ChangeState(inpInit);
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
         ChangeState(inpClosed);
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
   fOutState(outReady),
   fCurrentBuf(),
   fStopRequested(false),
   fRetryPeriod(-1.)
{
   AssignAddon(addon);
   CreateTimer("SysTimer");

   fRetryPeriod = outport.Cfg("retry", cmd).AsDouble(-1);

   if (!fTransportInfoName.empty() && fOutput)
      fOutput->SetInfoParName(fTransportInfoName);

   DOUT2("Create out transport %s  %s", GetName(), ItemName().c_str());
}

dabc::OutputTransport::~OutputTransport()
{
   // DOUT0("DESTROY OUTPUT TRANSPORT %s", GetName());
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

void dabc::OutputTransport::ChangeState(EOutputStates state)
{
   fOutState = state;

   if (fStopRequested && SuitableStateForStartStop()) {
      StopTransport();
   }
}


bool dabc::OutputTransport::StartTransport()
{
   DOUT2("Starting OutputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   bool res = Transport::StartTransport();

   fStopRequested = false;

   if (fOutput==0) {
      EOUT("Output was not specified!!!");
      return false;
   }

   if (!SuitableStateForStartStop()) {
      EOUT("Start transport %s at not optimal state %u", GetName(), (unsigned) fOutState);
   }

   return res;
}

bool dabc::OutputTransport::StopTransport()
{
   DOUT2("Stopping OutputTransport %s isrunning %s", GetName(), DBOOL(IsRunning()));

   if (SuitableStateForStartStop()) {
      fStopRequested = false;
      return Transport::StopTransport();
   }

   if (!fStopRequested) {
      fStopRequested = true;
      DOUT2("%s Try to wait until suitable state is achieved now %u", GetName(), (unsigned) fOutState);
   }

   return true;
}


void dabc::OutputTransport::TransportCleanup()
{
   // DOUT0("CLEANUP OUTPUT TRANSPORT %s", GetName());

   CloseOutput();

   dabc::Transport::TransportCleanup();
}

void dabc::OutputTransport::CloseOnError()
{

   if ((fRetryPeriod < 0.) || (fOutput==0) || !fOutput->Write_Retry()) {
      ChangeState(outClosed);
      CloseOutput();
      CloseTransport(true);
   }

   ChangeState(outRetry);
   ShootTimer("SysTimer", fRetryPeriod);
}


void dabc::OutputTransport::ProcessEvent(const EventId& evnt)
{
 //  DOUT0("%s dabc::OutputTransport::ProcessEvent %u  state %u", GetName(), (unsigned) evnt.GetCode(), fOutState);

   if (evnt.GetCode() == evCallBack) {

      if (evnt.GetArg() != do_Ok) {
         if (evnt.GetArg() == do_Error) EOUT("Callback with error argument");
         CloseOnError();
         return;
      }

      if (fOutState == outWaitCallback) {
         ChangeState(outReady);
         ProcessInputEvent(0);
         return;
      }

      if (fOutState == outWaitFinishCallback) {
         ChangeState(outFinishWriting);

         // we need to call ProcessRecv directly at least once before entering into normal loop
         if (ProcessRecv(0))
            ProcessInputEvent(0);

         return;
      }

      EOUT("Call-back at wrong state!!");
   }

   dabc::Transport::ProcessEvent(evnt);
}

bool dabc::OutputTransport::ProcessRecv(unsigned port)
{
//   if (IsName("_OnlineServer_Output0_Transport_Slave0_Transport"))
//      DOUT0("dabc::OutputTransport::ProcessRecv  %s state %u", GetName(), fOutState);

   if (fOutput==0) {
      EOUT("Output object not specified");
      ChangeState(outError);
   }

   if (fTransportDevice.DeviceDestroyed()) {
      ChangeState(outClosed);
      CloseOutput();
      CloseTransport(false);
      return false;
   }

   if (fOutState == outReady) {

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
// TODO: should we stop output when transport is closed?????
//      } else
//      if (!isTransportRunning()) {
//         // when transport not running, ignore all input buffers anyway
//         return false;
      } else {
         ret = fOutput->Write_Check();
      }

      switch (ret) {
         case do_Ok:
            ChangeState(outStartWriting);
            break;
         case do_Repeat:
            return true;
         case do_RepeatTimeOut:
            ChangeState(outInitTimeout);
            ShootTimer("SysTimer", fOutput->Write_Timeout());
            return false;
         case do_CallBack:
            ChangeState(outWaitCallback);
            return false;
         case do_Skip:
            Recv(port).Release();
            return true;
         case do_Close:
            ChangeState(outClosing);
            break;
         case do_Error:
            ChangeState(outError);
            break;
         default:
            EOUT("Wrong return value %u for the Write_Check", ret);
            ChangeState(outError);
      }
   }

   if (fOutState == outInitTimeout) {
      return false;
   }

   if (fOutState == outWaitCallback) {
      return false;
   }

   if (fOutState == outStartWriting) {

      fCurrentBuf = Recv(port);

      unsigned ret = fOutput->Write_Buffer(fCurrentBuf);

      switch (ret) {
         case do_Ok:
            ChangeState(outFinishWriting);
            break;
         case do_CallBack:
            ChangeState(outWaitFinishCallback);
            return false;
         case do_Skip:
            ChangeState(outReady);
            return true;
         case do_Close:
            ChangeState(outClosing);
            break;
         case do_Error:
            DOUT0("Error when writing buffer in transport %s", GetName());
            ChangeState(outError);
            break;
         default:
            EOUT("Wrong return value %u for the Write_Buffer", ret);
            ChangeState(outError);
      }
   }

   if (fOutState == outWaitFinishCallback) {
      // if we wait for call back, ignore all possible events
      return false;
   }

   if (fOutState == outFinishWriting) {

      fCurrentBuf.Release();

      unsigned ret = fOutput->Write_Complete();

      switch (ret) {
         case do_Ok:
            ChangeState(outReady);
            break;
         case do_Close:
            ChangeState(outClosing);
            break;
         case do_Error:
            ChangeState(outError);
            break;
         default:
            EOUT("%s Wrong return value %u for the Write_Complete", GetName(), ret);
            ChangeState(outError);
      }
   }

   if (fOutState == outClosing) {
      ChangeState(outClosed);
      CloseOutput();
      CloseTransport(false);
      return false;
   }

   if (fOutState == outError) {
      CloseOnError();
      return false;
   }

   if (fOutput && InfoExpected()) {
      std::string info = fOutput->ProvideInfo();
      ProvideInfo(0, info);
   }

   return true;
}

void dabc::OutputTransport::ProcessTimerEvent(unsigned)
{
   if (fOutState == outInitTimeout)
      ChangeState(outReady);

   if (fOutState == outRetry) {
      if (fOutput && fOutput->Write_Init())
         ChangeState(outReady);
      else {
         ShootTimer("SysTimer", fRetryPeriod);
         return;
      }
   }

   ProcessInputEvent(0);
}

int dabc::OutputTransport::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetTransportStatistic")) {
      // take statistic from output element
      cmd.SetStr("OutputState",StateAsStr());
      if (fOutput) fOutput->Write_Stat(cmd);
      return cmd_true;
   }

   return dabc::Transport::ExecuteCommand(cmd);
}

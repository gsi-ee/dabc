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
#include "dabc/Port.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"

dabc::DataTransport::DataTransport(Reference port, bool doinput, bool dooutput) :
   Worker(0, "DataTransport", true),
   Transport(port),
   MemoryPoolRequester(),
   fMutex(),
   fInpQueue(doinput ? GetPort()->InputQueueCapacity() : 0),
   fOutQueue(dooutput ? GetPort()->OutputQueueCapacity() : 0),
   fActive(false),
   fDoInput(doinput),
   fDoOutput(dooutput),
   fInpState(inpReady),
   fInpLoopActive(false),
   fNextDataSize(0),
   fCurrentBuf(),
   fComplRes(di_None),
   fPoolChangeCounter(0)
{
   DOUT5(("Create DataTransport %p for port %s", this, port.ItemName().c_str()));

   RegisterTransportDependencies(this);
}

dabc::DataTransport::~DataTransport()
{
   DOUT5(("Destroy DataTransport %u %p", fWorkerId, this));

   fInpQueue.Cleanup();
   fOutQueue.Cleanup();

   DOUT5(("Destroy DataTransport %u %p done", fWorkerId, this));
}

int dabc::DataTransport::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("TestPoolChange")) {
      if (GetPool() && GetPool()->CheckChangeCounter(fPoolChangeCounter))
         ProcessPoolChanged(GetPool());
      return cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

void dabc::DataTransport::PortAssigned()
{
   Execute("TestPoolChange");
}

void dabc::DataTransport::StartTransport()
{
   DOUT3(("DataTransport::StartTransport() %u", fWorkerId));
   unsigned nout = 0;
   bool fireinpevent = false;

   {
      LockGuard guard(fMutex);
      fActive = true;
      if (ProvidesOutput())
         nout = fOutQueue.Size();

      if (ProvidesInput() && !fInpLoopActive) {
         fInpLoopActive = true;
         fireinpevent = true;
      }
   }

   // start reading of the input in the beginning
   if (fireinpevent) FireEvent(evDataInput);

   // if one had buffers in output queue, activate all of them
   while (nout-- > 0) FireEvent(evDataOutput);
}

void dabc::DataTransport::StopTransport()
{
   DOUT3(("DataTransport::StopTransport() %u", fWorkerId));

   {
      LockGuard guard(fMutex);
      fActive = false;
   }
}

void dabc::DataTransport::CleanupFromTransport(Object* obj)
{
   if (obj == GetPool()) {
      fInpQueue.Cleanup(&fMutex);
      fOutQueue.Cleanup(&fMutex);

      fCurrentBuf.Release(&fMutex);

      LockGuard guard(fMutex);

      // FIXME: that should be done when buffer reading is started and than buffer is disappearing????
      // if (fInpState == inpPrepare) Complete_Buffer_Reading!!!!

      fInpState = inpReady;
   }



   dabc::Transport::CleanupFromTransport(obj);
}


void dabc::DataTransport::CleanupTransport()
{
   if (ProvidesInput()) {
      ProcessInputEvent(false);

      // cancel request to the memory pool
      if (GetPool()) GetPool()->RemoveRequester(this);
   }

   {
      LockGuard guard(fMutex);
      fInpState = inpReady;
   }

   fCurrentBuf.Release(&fMutex);

   fInpQueue.Cleanup(&fMutex);
   fOutQueue.Cleanup(&fMutex);

   CloseInput();
   CloseOutput();

   dabc::Transport::CleanupTransport();
}


bool dabc::DataTransport::Recv(Buffer &buf)
{
   bool fireinpevent = false;

   {
      LockGuard lock(fMutex);

      if (fInpQueue.Size()<=0) {
          // EOUT(("Get buffer from DataTransport when it is empty"));
          return false;
      }

      buf << fInpQueue.Pop();

      if (!buf.ispool()) EOUT(("Buffer without pool - should not happen!!!"));

      if (!fInpLoopActive) {
         fireinpevent = true;
         fInpLoopActive = true;
      }
   }

   if (fireinpevent) FireEvent(evDataInput);
   
   return buf.ispool();
}

unsigned dabc::DataTransport::RecvQueueSize() const
{
   LockGuard guard(fMutex);

   return fInpQueue.Size();
}

dabc::Buffer& dabc::DataTransport::RecvBuffer(unsigned indx) const
{
   LockGuard lock(fMutex);

   return fInpQueue.ItemRef(indx);
}

bool dabc::DataTransport::Send(const Buffer& buf)
{
//   DOUT1(("dabc::DataTransport::Send %p", buf));

   bool res = false;

   if (!buf.null())
      res = fOutQueue.Push(buf, &fMutex);

   if (!res) {
      EOUT(("AAAAAAAAAAA !!!!!!!!!!! %d %d ", fOutQueue.Size(), fOutQueue.Capacity()));
   } else
      FireEvent(evDataOutput);

   return res;
}

unsigned dabc::DataTransport::SendQueueSize()
{
   LockGuard guard(fMutex);

   return fOutQueue.Size();
}

void dabc::DataTransport::ProcessEvent(const EventId& evnt)
{
//   DOUT1(("DataTransport::ProcessEvent id:%u arg:%llx ev:%u", fWorkerId, arg, evid));

   switch (evnt.GetCode()) {
      case evDataInput: {
         double tmout = ProcessInputEvent(true);
         if (tmout>=0) ActivateTimeout(tmout);
         break;
      }
      case evDataOutput:
         ProcessOutputEvent();
         break;

      default:
         Worker::ProcessEvent(evnt);
   }
}

double dabc::DataTransport::ProcessInputEvent(bool norm_call)
{
   // do recv next buffer
   // returns required timeout, -1 - no timeout
   // norm_call indicates that processing is called by event or by timeout

   if (GetPool()==0) {
      DOUT2(("DataTransport %p - no memory pool!!!!", this));
      return -1.;
   }

//   EOUT(("Tr:%08x Process event", this));

   // do not try to read from input if module is blocked or data channel is blocked
   // next time when module starts, we again get new event (via ProcessStart)
   // and come back to this point again
   if (!ProvidesInput()) return -1.;

   bool doreadbegin = true;
   bool dofireevent = false;
   double ret_tmout = -1.;
   EInputStates state = inpReady;

   dabc::Buffer currbuf, qbuf, eqbuf;

   unsigned qsize = 0;

   {
      LockGuard lock(fMutex);

      // just an indicator, that somebody else activate event processing
      if (fInpState == inpWorking) return -1.;

      // we indicating that our loop now active and we do not want any external events
      fInpLoopActive = true;

      if (!fActive || !norm_call)
         doreadbegin = false;

      currbuf << fCurrentBuf;

      state = fInpState;
      fInpState = inpWorking;

      qsize = fInpQueue.Capacity() - fInpQueue.Size();
   }

   if (state == inpPrepare) {

      if (!currbuf.ispool()) {
         EOUT(("Internal error - currbuf==0 when state is prepared"));
         state = inpError;
      } else {

         if (fComplRes == di_None)
            fComplRes = Read_Complete(currbuf);

         switch (fComplRes) {
            case di_Ok:
               state = inpReady;
               break;
            case di_SkipBuffer:
               currbuf.Release();
               DOUT4(("Skip input buffer"));
               state = inpReady;
               break;
            case di_EndOfStream:
               currbuf.Release();
               DOUT4(("End of stream"));
               state = inpError;
               break;
            case di_Repeat:
               fComplRes = di_None;
               dofireevent = true;
               break;
            case di_RepeatTimeOut:
               fComplRes = di_None;
               ret_tmout = Read_Timeout();
               if (ret_tmout>0)
                  dofireevent = false; //do not fire event, function will be caused by timeout
               else {
                  dofireevent = true;
                  ret_tmout = -1.;
               }
               break;
            default:
               EOUT(("Error when do buffer reading res = %d", fComplRes));
               state = inpError;
         }
      }
   }

   if (state == inpReady) {

      qbuf << currbuf;
      if (qbuf.ispool()) qsize--;

      if (qsize == 0) {
         doreadbegin = false;
         dofireevent = false;
      }

      if (doreadbegin) {
         fNextDataSize = Read_Size();

         if (fNextDataSize <= di_ValidSize)
            state = inpNeedBuffer;
         else
         if (fNextDataSize == di_Repeat) {
            // repeat process input as soon as possible
            dofireevent = true;
         } else
         if (fNextDataSize == di_RepeatTimeOut) {
            // repeat process after timeout
            ret_tmout = Read_Timeout();
            if (ret_tmout>0)
               dofireevent = false; //do not fire event, function will be caused by timeout
            else {
               dofireevent = true;
               ret_tmout = -1.;
            }

         } else
         if (fNextDataSize != di_EndOfStream) {
            EOUT(("Reading error"));
            state = inpError;
         }
      }
   }

   if (state == inpNeedBuffer) {

      if (GetPool()->CheckChangeCounter(fPoolChangeCounter))
         ProcessPoolChanged(GetPool());

      currbuf = GetPool()->TakeBufferReq(this, fNextDataSize);

      if (currbuf.ispool()) {
         if (currbuf.GetTotalSize() < fNextDataSize) {
            EOUT(("Requested buffer smaller than actual data size"));
            state = inpError;
         } else {
            currbuf.SetTotalSize(fNextDataSize);
            state = inpPrepare;
            fComplRes = di_Ok;
            dofireevent = true;
         }
      }
   }

   if (state == inpError) {

      DOUT4(("Generate EOF packet"));
      CloseInput();
      fNextDataSize = 0;

      dofireevent = false;

      currbuf.Release();

      eqbuf = GetPool()->TakeEmpty();
      if (!eqbuf.ispool()) {
         EOUT(("Fatal error - cannot get empty buffer, try after 1 sec"));
         ret_tmout = 1.;
      } else {
         eqbuf.SetTypeId(dabc::mbt_EOF);
         state = inpClosed;
      }
   }


   // Call it here, where everything is ready to "leave" event loop
   // This is important for callback functionality

   bool iscallback = false;

   if (state == inpPrepare)
       switch (Read_Start(currbuf)) {
          case di_Ok:
             // this will allows to call Read_Complete method in next iteration
             fComplRes = di_None;
             break;
          case di_CallBack:
             // if we starts callback, just not fire event
             dofireevent = false;
             iscallback = true;
             break;
          default: 
             state = inpError;
       }


   int firecnt(0);

   {
      LockGuard lock(fMutex);
      if (qbuf.ispool()) { fInpQueue.Push(qbuf); firecnt++; }
      if (eqbuf.ispool()) { fInpQueue.Push(eqbuf); firecnt++; }
      // if queue become empty while single input event processing, restart loop
      if (!dofireevent && !iscallback && (fInpQueue.Size()==0) &&
           (state!=inpClosed) && (ret_tmout<=0.)) dofireevent = true;
      fInpLoopActive = dofireevent || (ret_tmout>0.);
      fCurrentBuf << currbuf;
      fInpState = state;

   }

   if ((fInpState == inpPrepare) && !fCurrentBuf.ispool())
      EOUT(("Empty buffer in prepare state !!!"));

   // no need to use port mutex - we are inside thread
   while (firecnt-->0) FirePortInput();
   if (dofireevent) FireEvent(evDataInput);

   return ret_tmout;
}

void dabc::DataTransport::Read_CallBack(unsigned compl_res)
{
   bool fireinpevent = false;

   {
      LockGuard lock(fMutex);

      if (fInpState == inpPrepare) {
         fireinpevent = true;
         fComplRes = compl_res;
      } else
         EOUT(("Call back at wrong place !!!"));
   }

   if (fireinpevent) FireEvent(evDataInput);

//   ProcessInputEvent(true, compl_res);
}

void dabc::DataTransport::ProcessOutputEvent()
{
   // do send next buffer

   dabc::Buffer buf;

   {
      LockGuard guard(fMutex);
      if (!fActive || (fOutQueue.Size()==0)) return;
      buf << fOutQueue.Pop();
   }

   if (buf.null()) return;

   bool dofire = false;

   if (!IsTransportErrorFlag()) {
      if (buf.GetTypeId() == dabc::mbt_EOF) {
         // we know that this is very last packet
         // we can close output
         CloseTransport();
         DOUT1(("Close output while EOF"));
      } else
      if (buf.GetTypeId() == dabc::mbt_EOL) {
         DOUT1(("Skip EOL buffer and flush output"));
         FlushOutput();
         dofire = true;
      } else {
         // we can do it outside lock while access to file itself is only possible
         // from the FileThread only
         if (fDoOutput){
           if (WriteBuffer(buf))
              dofire = true;
           else {
              EOUT(("Error when writing buffer to output - close it"));
              CloseTransport(true);
           }
         }
      }
   }

   // release buffer in any case
   buf.Release();

   // no need port mutex while we are inside the thread
   if (dofire) FirePortOutput();
}

double dabc::DataTransport::ProcessTimeout(double)
{
   // timeout is only need for input processing, directly do it

   return ProcessInputEvent(true);
}

bool dabc::DataTransport::ProcessPoolRequest()
{
   // this call done from other thread, therefore fire event to activate loop

   bool fireinpevent = false;

   {
      LockGuard lock(fMutex);

      if (!fInpLoopActive) {
         fireinpevent = true;
         fInpLoopActive = true;
      }
   }

   if (fireinpevent) FireEvent(evDataInput);
   return true;
}

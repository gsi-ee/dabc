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
#include "dabc/DataTransport.h"

#include "dabc/Buffer.h"
#include "dabc/Port.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"

dabc::DataTransport::DataTransport(Device* dev, Port* port, bool doinput, bool dooutput) :
   Transport(dev),
   WorkingProcessor(),
   MemoryPoolRequester(),
   fMutex(),
   fInpQueue(doinput ? port->InputQueueCapacity() : 0),
   fOutQueue(dooutput ? port->OutputQueueCapacity() : 0),
   fActive(false),
   fPool(0),
   fDoInput(doinput),
   fDoOutput(dooutput),
   fInpState(inpReady),
   fInpLoopActive(false),
   fNextDataSize(0),
   fCurrentBuf(0),
   fComplRes(di_None),
   fPoolChangeCounter(0)
{
   fPool = port->GetMemoryPool();

   DOUT5(("Data transport %p pool %p port->pool %p", this, fPool, port->GetPoolHandle()));
   DOUT5(("Create DataTransport %p for port %s", this, port->GetFullName().c_str()));
}

dabc::DataTransport::~DataTransport()
{
   DOUT5(("Destroy DataTransport %u %p", fProcessorId, this));

   CloseInput();
   CloseOutput();

   fInpQueue.Cleanup();
   fOutQueue.Cleanup();

   DOUT5(("Destroy DataTransport %u %p done", fProcessorId, this));
}

int dabc::DataTransport::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("CloseOperation")) {
      // we just complete operation, if it was not completed before
      ProcessInputEvent(false);

      // cancel request to the memory pool
      if (fPool) fPool->RemoveRequester(this);

      // fCurrentBuf pointer will be also cleared
      dabc::Buffer::Release(fCurrentBuf, &fMutex);

      return cmd_true;
   } else
   if (cmd->IsName("TestPoolChange")) {
      if (fPool->CheckChangeCounter(fPoolChangeCounter))
         ProcessPoolChanged(fPool);
      return cmd_true;
   }

   return dabc::WorkingProcessor::ExecuteCommand(cmd);
}

void dabc::DataTransport::PortChanged()
{
   if (IsPortAssigned()) {
      Execute("TestPoolChange");
//      Submit(new dabc::Command("TestPoolChange"));
   } else {

      if (ProvidesInput())
         Execute("CloseOperation", 1.);

      // release buffers as soon as possible, using mutex
      fInpQueue.Cleanup(&fMutex);
      fOutQueue.Cleanup(&fMutex);
   }
}

void dabc::DataTransport::StartTransport()
{
   DOUT3(("DataTransport::StartTransport() %u", fProcessorId));
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
   DOUT3(("DataTransport::StopTransport() %u", fProcessorId));

   {
      LockGuard guard(fMutex);
      fActive = false;
   }
}

bool dabc::DataTransport::Recv(Buffer* &buf)
{
   buf = 0;

   bool fireinpevent = false;

   {
      LockGuard lock(fMutex);

      if (fInpQueue.Size()<=0) return false;

      buf = fInpQueue.Pop();

      if (!fInpLoopActive) {
         fireinpevent = true;
         fInpLoopActive = true;
      }
   }

   if (fireinpevent) FireEvent(evDataInput);

   return buf!=0;
}

unsigned dabc::DataTransport::RecvQueueSize() const
{
   LockGuard guard(fMutex);

   return fInpQueue.Size();
}

dabc::Buffer* dabc::DataTransport::RecvBuffer(unsigned indx) const
{
   LockGuard lock(fMutex);

   return fInpQueue.Item(indx);
}

bool dabc::DataTransport::Send(Buffer* buf)
{
//   DOUT1(("dabc::DataTransport::Send %p", buf));

   bool res = false;

   {
      LockGuard guard(fMutex);

      res = fOutQueue.MakePlaceForNext();
      if (res) fOutQueue.Push(buf);
   }

   if (!res) {
      dabc::Buffer::Release(buf);
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

void dabc::DataTransport::ProcessEvent(EventId evnt)
{
//   DOUT1(("DataTransport::ProcessEvent id:%u arg:%llx ev:%u", fProcessorId, arg, evid));

   switch (GetEventCode(evnt)) {
      case evDataInput: {
         double tmout = ProcessInputEvent(true);
         if (tmout>=0) ActivateTimeout(tmout);
         break;
      }
      case evDataOutput:
         ProcessOutputEvent();
         break;

      default:
         WorkingProcessor::ProcessEvent(evnt);
   }
}

double dabc::DataTransport::ProcessInputEvent(bool norm_call)
{
   // do recv next buffer
   // returns required timeout, -1 - no timeout
   // norm_call indicates that processing is called by event or by timeout

   if (fPool==0) EOUT(("DataTransport %p AAAAAAAAAAAAAAAA", this));

//   EOUT(("Tr:%08x Process event", this));

   // do not try to read from input if module is blocked or data channel is blocked
   // next time when module starts, we again get new event (via ProcessStart)
   // and come back to this point again
   if (!ProvidesInput()) return -1.;

   bool doreadbegin = true;
   bool dofireevent = false;
   double ret_tmout = -1.;
   EInputStates state = inpReady;

   dabc::Buffer *currbuf(0), *qbuf(0), *eqbuf(0);

   unsigned qsize = 0;

   {
      LockGuard lock(fMutex);

      // just an indicator, that somebody else activate event processing
      if (fInpState == inpWorking) return -1.;

      // we indicating that our loop now active and we do not want any external events
      fInpLoopActive = true;

      if (!fActive || !norm_call)
         doreadbegin = false;

      currbuf = fCurrentBuf;
      fCurrentBuf = 0;

      state = fInpState;
      fInpState = inpWorking;

      qsize = fInpQueue.Capacity() - fInpQueue.Size();
   }

   if (state == inpPrepare) {

      if (currbuf==0) {
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
               dabc::Buffer::Release(currbuf);
               currbuf = 0;
               DOUT1(("Skip input buffer"));
               state = inpReady;
               break;
            case di_EndOfStream:
               dabc::Buffer::Release(currbuf);
               currbuf = 0;
               DOUT1(("End of stream"));
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

      qbuf = currbuf;
      currbuf = 0;
      if (qbuf) qsize--;

      if (qsize == 0) {
         doreadbegin = false;
         dofireevent = false;
      }

      if (doreadbegin) {
         fNextDataSize = Read_Size();

         if (fNextDataSize > 0)
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

      if (fPool->CheckChangeCounter(fPoolChangeCounter))
         ProcessPoolChanged(fPool);

      currbuf = fPool->TakeBufferReq(this, fNextDataSize);

      if (currbuf!=0) {
         if (currbuf->GetDataSize() < fNextDataSize) {
            EOUT(("Requested buffer smaller than actual data size"));
            state = inpError;
         } else {
            currbuf->SetDataSize(fNextDataSize);
            state = inpPrepare;
            fComplRes = di_Ok;
            dofireevent = true;
         }
      }
   }

   if (state == inpError) {

      DOUT1(("Generate EOF packet"));

      CloseInput();
      fNextDataSize = 0;

      dofireevent = false;

      if (currbuf!=0) {
         dabc::Buffer::Release(currbuf);
         currbuf = 0;
      }

      eqbuf = fPool->TakeEmptyBuffer();
      if (eqbuf==0) {
         EOUT(("Fatal error - cannot get empty buffer, try after 1 sec"));
         ret_tmout = 1.;
      } else {
         eqbuf->SetTypeId(dabc::mbt_EOF);
         state = inpClosed;
      }
   }

   {
      LockGuard lock(fMutex);
      if (qbuf) fInpQueue.Push(qbuf);
      if (eqbuf) fInpQueue.Push(eqbuf);
      // if queue become empty while single input event processing, restart loop
      if (!dofireevent && (fInpQueue.Size()==0) &&
           (state!=inpClosed) && (ret_tmout<=0.)) dofireevent = true;
      fInpLoopActive = dofireevent || (ret_tmout>0.);
      fCurrentBuf = currbuf;
      fInpState = state;
   }

   // Call it here, where everything is ready to "leave" event loop
   // This is important for callback functionality

   if (state == inpPrepare)
       switch (Read_Start(currbuf)) {
          case di_Ok:
             // this will allows to call Read_Complete method in next iteration
             fComplRes = di_None;
             break;
          case di_CallBack:
             // if we starts callback, just not fire event
             dofireevent = false;
             break;
          default: {
             LockGuard lock(fMutex);
             fInpState = inpError;
          }
       }

   if (qbuf) FireInput();
   if (eqbuf) FireInput();
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

   dabc::Buffer* buf = 0;

   {
      LockGuard guard(fMutex);
      if (!fActive || (fOutQueue.Size()==0)) return;
      buf = fOutQueue.Pop();
   }

   if (buf==0) return;

   bool dofire = false;

   if (!IsErrorState()){
      if (buf->GetTypeId() == dabc::mbt_EOF) {
         // we know that this is very last packet
         // we can close output
         CloseTransport();
         DOUT1(("Close output while EOF"));
      } else
      if (buf->GetTypeId() == dabc::mbt_EOL) {
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
              ErrorCloseTransport();
           }
           }
      }
      }

   // release buffer in any case
   dabc::Buffer::Release(buf);

   if (dofire) FireOutput();
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

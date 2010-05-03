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
#include "dabc/ModuleSync.h"

#include "dabc/logging.h"
#include "dabc/ModuleItem.h"
#include "dabc/PoolHandle.h"

dabc::ModuleSync::ModuleSync(const char* name, Command* cmd) :
   Module(name, cmd),
   fTmoutExcept(false),
   fDisconnectExcept(false),
   fSyncCommands(false),
   fNewCommands(),
   fWaitItem(0),
   fWaitId(0),
   fWaitRes(false),
   fLoopStatus(stInit)
{
}

dabc::ModuleSync::~ModuleSync()
{
   fRunState = msHalted;

   // if module assigned to thread, make sure that mainloop of the module is leaved
   ExitMainLoop();

   if (fLoopStatus != stInit) {
      EOUT(("Problem in destructor - cannot leave normally main loop, must crash :-O"));
   }
}

bool dabc::ModuleSync::DoHalt()
{
   fRunState = msHalted;

   ExitMainLoop();

   return dabc::Module::DoHalt();
}


bool dabc::ModuleSync::WaitConnect(Port* port, double timeout)
   throw (PortException, StopException, TimeoutException)
{
   if (port==0) return false;

   uint16_t evid(evntNone);

   do {
      if (port->IsConnected()) return true;

      if (evid == evntPortConnect) {
         EOUT(("Internal error. Connect event, but no transport"));
         throw PortException(port, "Internal");
      }

   } while (WaitItemEvent(timeout, port, &evid));

   return false;

}

bool dabc::ModuleSync::Send(Port* port, Buffer* buf, double timeout)
   throw (PortOutputException, StopException, TimeoutException)
{
   BufferGuard guard(buf);

   return Send(port, guard, timeout);
}

bool dabc::ModuleSync::Send(Port* port, BufferGuard &buf, double timeout)
   throw (PortOutputException, StopException, TimeoutException)
{
   if ((port==0) || (buf()==0)) return false;

   uint16_t evid(evntNone);

   // one need Keeper to release buffer in case of exceptions
   do {

      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw PortOutputException(port, "Disconnect");

      if (port->CanSend())
         return port->Send(buf.Take());

   } while (WaitItemEvent(timeout, port, &evid));

   return false;
}


dabc::Buffer* dabc::ModuleSync::Recv(Port* port, double timeout)
   throw (PortInputException, StopException, TimeoutException)
{
   if (port==0) return 0;

   uint16_t evid(evntNone);

   do {
      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw PortInputException(port, "Disconnect");

      if (port->CanRecv())
         return port->Recv();

   } while (WaitItemEvent(timeout, port, &evid));

   return 0;
}

dabc::Buffer* dabc::ModuleSync::RecvFromAny(Port** port, double timeout)
   throw (PortInputException, StopException, TimeoutException)
{
   uint16_t evid(evntNone);
   ModuleItem* resitem(0);
   unsigned shift(0);

   do {
      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw PortInputException((Port*) resitem, "Disconnect");

      if (NumInputs() == 0) return 0;

      if (evid == evntInput) shift = InputNumber( (Port*) resitem );
                        else shift = 0;

      for (unsigned n=0; n < NumInputs(); n++) {
         Port* p = Input( (n+shift) % NumInputs());
         if (p->CanRecv()) {
            if (port) *port = p;
            return p->Recv();
         }
      }
   } while (WaitItemEvent(timeout, 0, &evid, &resitem));

   return 0;
}


bool dabc::ModuleSync::WaitInput(Port* port, unsigned minqueuesize, double timeout)
  throw (PortInputException, StopException, TimeoutException)
{
   if (port==0) return false;

   uint16_t evid(evntNone);

   do {
      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw PortInputException(port, "Disconnect");

      if (port->InputPending() >= minqueuesize) return true;
   } while (WaitItemEvent(timeout, port, &evid));

   return false;
}

dabc::Buffer* dabc::ModuleSync::TakeBuffer(PoolHandle* pool, BufferSize_t size, BufferSize_t hdrsize, double timeout)
   throw (StopException, TimeoutException)
{
   if (pool==0) return 0;

   dabc::Buffer* buf = pool->TakeRequestedBuffer();
   if (buf!=0) {
      EOUT(("There is requested buffer of size %d", buf->GetTotalSize()));
      dabc::Buffer::Release(buf);
      buf = 0;
   }

   if (timeout==0.)
      return pool->TakeBuffer(size, hdrsize);

   buf = pool->TakeBufferReq(size, hdrsize);
   if (buf!=0) return buf;

   do {
      buf = pool->TakeRequestedBuffer();
      if (buf!=0) return buf;
   } while (WaitItemEvent(timeout, pool));

   return 0;
}

bool dabc::ModuleSync::ModuleWorking(double timeout)
   throw (StopException, TimeoutException)
{
   // process exact one event without any timeout
   SingleLoop(0.);

   AsyncProcessCommands();

   return !IsHalted();
}

uint16_t dabc::ModuleSync::WaitEvent(double timeout)
   throw (StopException, TimeoutException)
{
   uint16_t evid = 0;

   if (!WaitItemEvent(timeout, 0, &evid)) evid = 0;

   return evid;
}

int dabc::ModuleSync::PreviewCommand(Command* cmd)
{
   int cmd_res = cmd_ignore;

   if (cmd->IsName("StartModule")) {
      cmd_res = cmd_true;
      switch (fLoopStatus) {
         case stInit:
            // this call should lead to the initiation of main loop
            // there as first step, status must be changed
            cmd_res = cmd_bool(ActivateMainLoop());
            break;
         case stRun:
            // main loop running, do nothing
            break;
         case stSuspend:
            DoStart();
            fLoopStatus = stRun;
            break;
      }
   } else
   if (cmd->IsName("StopModule")) {
      cmd_res = cmd_true;
      switch (fLoopStatus) {
         case stInit:
            // call do stop if somebody call stop at this place
            // actually, nothing should happen
            DoStop();
            break;
         case stRun:
            DoStop();
            fLoopStatus = stSuspend;
            // main loop running, do nothing
            break;
         case stSuspend:
            // here nothing to do
            break;
      }
   } else {
      cmd_res = Module::PreviewCommand(cmd);

      if (!fSyncCommands && (cmd_res==cmd_ignore)) {
         fNewCommands.Push(cmd);
         cmd_res = cmd_postponed;
      }
   }

   return cmd_res;
}

void dabc::ModuleSync::StopUntilRestart()
{
   Stop();

   DOUT1(("Stop module %s until restart", GetName()));

   double tmout = -1.;

   WaitItemEvent(tmout);

   DOUT1(("Finish StopUntilRestart for module %s", GetName()));
}

void dabc::ModuleSync::AsyncProcessCommands()
{
   while (fNewCommands.Size()>0) {
      dabc::Command* cmd = fNewCommands.Pop();
      int cmd_res = ExecuteCommand(cmd);
      if (cmd_res>=0)
         dabc::Command::Reply(cmd, cmd_res);
   }
}

void dabc::ModuleSync::ProcessUserEvent(ModuleItem* item, uint16_t evid)
{
   // no need to store any consequent events
   if (fWaitRes) return;

   if (item==0) { EOUT(("Zero item !!!!!!!!!!!!!")); }

   if ((fWaitItem==item) || (fWaitItem==0)) {
      fWaitRes = true;
      fWaitId = evid;
      fWaitItem = item;
   }
}

bool dabc::ModuleSync::WaitItemEvent(double& tmout, ModuleItem* item, uint16_t *resevid, ModuleItem** resitem)
  throw (StopException, TimeoutException)

{
   fWaitItem = item;
   fWaitId = 0;
   fWaitRes = false;

   TimeStamp_t last_tm = NullTimeStamp;

   while (!fWaitRes || (fLoopStatus==stSuspend)) {

      if ((ProcessorThread()==0) || IsHalted())
         throw StopException();

      // account timeout only in running state
      if ((tmout>=0) && (fLoopStatus==stRun)) {
         TimeStamp_t tm = ThrdTimeStamp();

         if (!IsNullTime(last_tm)) {
            tmout -= TimeDistance(last_tm, tm);
            if (tmout<0){
               if (IsTmoutExcept())
                  throw TimeoutException();
               else
                  return false;
            }
         }

         last_tm = tm;
      } else
         last_tm = NullTimeStamp;

      SingleLoop(tmout);
   }

   if (resevid!=0) *resevid = fWaitId;
   if (resitem!=0) *resitem = fWaitItem;

   return true; // it is normal exit
}

void dabc::ModuleSync::DoProcessorMainLoop()
{
   DoStart();

   fLoopStatus = stRun;

   MainLoop();

}

void dabc::ModuleSync::DoProcessorAfterMainLoop()
{
   if (fLoopStatus == stRun)
      DoStop();

   fLoopStatus = stInit;
}

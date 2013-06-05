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

#include "dabc/ModuleSync.h"

#include "dabc/logging.h"
#include "dabc/CommandsQueue.h"

dabc::ModuleSync::ModuleSync(const std::string& name, Command cmd) :
   Module(name, cmd),
   fTmoutExcept(false),
   fDisconnectExcept(false),
   fSyncCommands(false),
   fNewCommands(0),
   fWaitItem(0),
   fWaitId(0),
   fWaitRes(false),
   fInsideMainLoop(false)
{
}

dabc::ModuleSync::~ModuleSync()
{
   // if module was not yet halted, make sure that mainloop of the module is leaved

   if (IsRunning()) {
      EOUT("Problem in sync module %s destructor - cannot leave normally main loop, must crash :-O", GetName());
   }

   if (fNewCommands!=0) {
      EOUT("Some commands remain event in module %s destructor - BAD", GetName());
      delete fNewCommands;
      fNewCommands = 0;
   }
}

bool dabc::ModuleSync::WaitConnect(const std::string& name, double timeout) throw (Exception)
{
   PortRef port = FindPort(name);

   if (port.null()) return false;

   uint16_t evid(evntModuleNone);

   do {
      // we are using direct pointer, while method can only be used from inside thread
      if (port()->IsConnected()) return true;

      if (evid == evntPortConnect)
         throw Exception(ex_Connect, "Get connection event when port is not connected", port.ItemName());

   } while (WaitItemEvent(timeout, port(), &evid));

   return false;

}

bool dabc::ModuleSync::Send(unsigned indx, Buffer &buf, double timeout) throw (dabc::Exception)
{
   OutputPort* port = Output(indx);

   if ((port==0) || buf.null()) return false;

   uint16_t evid(evntModuleNone);

   // one need Keeper to release buffer in case of exceptions
   do {

      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw Exception(ex_Disconnect, "Port disconnected when sending buffer", port->ItemName());

      if (port->CanSend())
         return port->Send(buf);

   } while (WaitItemEvent(timeout, port, &evid));

   return false;
}


dabc::Buffer dabc::ModuleSync::Recv(unsigned indx, double timeout) throw (dabc::Exception)
{
   InputPort* port = Input(indx);

   if (port==0) return Buffer();

   uint16_t evid(evntModuleNone);

   do {
      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw Exception(ex_Disconnect, "Port disconnected when receiving buffer", port->ItemName());

      if (port->CanRecv())
         return port->Recv();

   } while (WaitItemEvent(timeout, port, &evid));

   return Buffer();
}

dabc::Buffer dabc::ModuleSync::TakeBuffer(unsigned poolindx, double timeout) throw (dabc::Exception)
{
   PoolHandle* handle = Pool(poolindx);
   if (handle==0) return (poolindx==0) ? TakeDfltBuffer() : Buffer();

   do {
      if (handle->CanTakeBuffer())
         return handle->TakeBuffer();
   } while (WaitItemEvent(timeout, handle));

   return Buffer();
}



dabc::Buffer dabc::ModuleSync::RecvFromAny(unsigned* indx, double timeout) throw (dabc::Exception)
{
   uint16_t evid(evntModuleNone);
   ModuleItem* resitem(0);
   unsigned shift(0);

   do {
      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw Exception(ex_Disconnect, "Port disconnected when receiving buffer", resitem ? resitem->ItemName() : "");

      if (NumInputs() == 0) return Buffer();

      if (resitem && (evid == evntInput))
         shift = resitem->fSubId;
      else
         shift = 0;

      for (unsigned n=0; n < NumInputs(); n++) {
         InputPort* p = fInputs[(n+shift) % NumInputs()];
         if (p->CanRecv()) {
            if (indx) *indx = p->fSubId;
            return p->Recv();
         }
      }
   } while (WaitItemEvent(timeout, 0, &evid, &resitem));

   return Buffer();
}


bool dabc::ModuleSync::WaitInput(unsigned indx, unsigned minqueuesize, double timeout) throw (Exception)
{
   InputPort* port = Input(indx);

   if (port==0) return false;

   uint16_t evid(evntModuleNone);

   do {
      if (evid == evntPortDisconnect)
         if (IsDisconnectExcept())
            throw Exception(ex_Disconnect, "Port disconnected when waiting for input buffers", port->ItemName());

      if (port->NumCanRecv() >= minqueuesize) return true;
   } while (WaitItemEvent(timeout, port, &evid));

   return false;
}

bool dabc::ModuleSync::ModuleWorking(double timeout) throw (Exception)
{
   AsyncProcessCommands();

   if (!SingleLoop(timeout))
      throw dabc::Exception(ex_Stop, "Module stopped", ItemName());

   return true;
}

uint16_t dabc::ModuleSync::WaitEvent(double timeout) throw (Exception)
{
   uint16_t evid = 0;

   if (!WaitItemEvent(timeout, 0, &evid)) evid = 0;

   return evid;
}

int dabc::ModuleSync::PreviewCommand(Command cmd)
{
   if (cmd.IsName("StartModule")) {
      // module already running
      if (IsRunning()) return cmd_true;

      if (fInsideMainLoop) return cmd_bool(DoStart());

      return cmd_bool(ActivateMainLoop());
   } else
   if (cmd.IsName("StopModule")) {
      if (!IsRunning()) return cmd_true;

      if (!fInsideMainLoop) {
         EOUT("Something wrong, module %s runs without main loop ????", GetName());
         return cmd_false;
      }

      AsyncProcessCommands();

      return cmd_bool(DoStop());
   }

   int cmd_res = Module::PreviewCommand(cmd);

   // asynchronous execution possible only in running mode,
   // when module is stopped, commands will be executed immediately
   if (!fSyncCommands && (cmd_res==cmd_ignore) && IsRunning()) {

      if (fNewCommands==0)
         fNewCommands = new CommandsQueue(CommandsQueue::kindSubmit);

      fNewCommands->Push(cmd);
      cmd_res = cmd_postponed;
   }

   return cmd_res;
}

void dabc::ModuleSync::StopUntilRestart()
{
   Stop();

   DOUT1("Stop module %s until restart", GetName());

   double tmout = -1.;

   WaitItemEvent(tmout);

   DOUT1("Finish StopUntilRestart for module %s", GetName());
}


void dabc::ModuleSync::ObjectCleanup()
{
   if (fNewCommands!=0) {
      EOUT("Some commands remain event when module %s is cleaned up - BAD", GetName());
      AsyncProcessCommands();
   }

   DOUT4("ModuleSync::ObjectCleanup %s", GetName());

   dabc::Module::ObjectCleanup();
}

void dabc::ModuleSync::AsyncProcessCommands()
{
   if (fNewCommands==0) return;

   while (fNewCommands->Size()>0) {
      Command cmd = fNewCommands->Pop();
      int cmd_res = ExecuteCommand(cmd);
      if (cmd_res>=0) cmd.Reply(cmd_res);
   }

   delete fNewCommands;
   fNewCommands = 0;
}

void dabc::ModuleSync::ProcessItemEvent(ModuleItem* item, uint16_t evid)
{
   if ((evid==evntInput) || (evid==evntOutput))
     ((Port*) item)->ConfirmEvent();

   // no need to store any consequent events
   if (fWaitRes) return;

   if (item==0) { EOUT("Zero item !!!!!!!!!!!!!"); }

   if ((fWaitItem==item) || (fWaitItem==0)) {
      fWaitRes = true;
      fWaitId = evid;
      fWaitItem = item;
   }
}

bool dabc::ModuleSync::WaitItemEvent(double& tmout, ModuleItem* item, uint16_t *resevid, ModuleItem** resitem) throw (Exception)
{
   if (tmout<0) return false;

   fWaitItem = item;
   fWaitId = 0;
   fWaitRes = false;

   TimeStamp last_tm, tm;

   // if module not in running state, wait item event will block main loop completely
   while (!fWaitRes || !IsRunning()) {

      // account timeout only in running state
      if ((tmout>=0) && IsRunning()) {
         tm.GetNow();

         if (!last_tm.null()) {
            tmout -= (tm - last_tm);
            if (tmout<0) {
               if (IsTmoutExcept())
                  throw Exception(ex_Timeout, "Operation timeout for item", ItemName());
               else
                  return false;
            }
         }

         last_tm = tm;
      } else
         last_tm.Reset();

      // SingleLoop return false only when Worker should be halted,
      // we use this to stop module and break recursion

      if (!SingleLoop(tmout)) throw Exception(ex_Stop, "Module stopped when waiting for", ItemName());
   }

   if (resevid!=0) *resevid = fWaitId;
   if (resitem!=0) *resitem = fWaitItem;

   return true; // it is normal exit
}

void dabc::ModuleSync::DoWorkerMainLoop()
{
   DoStart();

   try {
      fInsideMainLoop = true;

      MainLoop();

      fInsideMainLoop = false;
   } catch (...) {
      fInsideMainLoop = false;
      throw;
   }
}

void dabc::ModuleSync::DoWorkerAfterMainLoop()
{
   fInsideMainLoop = false;

   if (IsRunning()) {
      AsyncProcessCommands();
      DoStop();
   }

   DOUT3("Stop sync module %s", GetName());
}

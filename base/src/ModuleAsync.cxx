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

#include "dabc/ModuleAsync.h"

#include "dabc/logging.h"
#include "dabc/Buffer.h"

dabc::ModuleAsync::~ModuleAsync()
{
}


bool dabc::ModuleAsync::RecvQueueFull(unsigned indx)
{
   InputPort* inp = Input(indx);
   return inp ? inp->QueueFull() : false;
}

void dabc::ModuleAsync::SignalRecvWhenFull(unsigned indx)
{
   InputPort* inp = Input(indx);
   if (inp) inp->SignalWhenFull();
}

dabc::Buffer dabc::ModuleAsync::RecvQueueItem(unsigned indx, unsigned nbuf)
{
   InputPort* inp = Input(indx);
   if (inp) return inp->Item(nbuf);
   return dabc::Buffer();
}

void dabc::ModuleAsync::ProcessItemEvent(ModuleItem* item, uint16_t evid)
{
    switch (evid) {
       case evntInput:
          ((Port*) item)->ConfirmEvent();
       case evntInputReinj:
          if (item->GetType() == mitPool)
             ProcessPoolEvent(item->fSubId);
          else
             ProcessInputEvent(item->fSubId);
          break;
       case evntOutput:
          ((Port*) item)->ConfirmEvent();
       case evntOutputReinj:
          ProcessOutputEvent(item->fSubId);
          break;
       case evntTimeout:
          ProcessTimerEvent(item->fSubId);
          break;
       case evntPortConnect:
          ProcessConnectEvent(item->GetName(), true);
          break;
       case evntPortDisconnect:
          ProcessConnectEvent(item->GetName(), false);
          break;
       case evntUser:
          ProcessUserEvent(item->fSubId);
          break;
       default:
          break;
    }
}

bool dabc::ModuleAsync::DoStart()
{
   if (!dabc::Module::DoStart()) return false;

   // TODO: in case of every event generate appropriate number of events
   for (unsigned n=0;n<NumInputs();n++)
      ProduceInputEvent(n, fInputs[n]->NumStartEvents());

   for (unsigned n=0;n<NumOutputs();n++)
      ProduceOutputEvent(n, fOutputs[n]->NumStartEvents());

   for (unsigned n=0;n<NumPools();n++)
      ProducePoolEvent(n, fPools[n]->NumStartEvents());

   return true;
}

void dabc::ModuleAsync::ActivateInput(unsigned port)
{
   InputPort* inp = Input(port);

   if ((inp!=0) && IsRunning() && inp->CanRecv())
      FireEvent(evntInputReinj, inp->ItemId());
}

void dabc::ModuleAsync::ProcessInputEvent(unsigned port)
{
   InputPort* inp = Input(port);

   int cnt = inp->GetMaxLoopLength();

   while (IsRunning() && inp->CanRecv()) {
      if (!ProcessRecv(port)) return;
      if (cnt<0) return;
      if (cnt-- == 0) {
         DOUT3("Port %s performed too many receive operations - break the loop", inp->ItemName().c_str());
         FireEvent(evntInputReinj, inp->ItemId());
         return;
      }
   }
}

void dabc::ModuleAsync::ActivateOutput(unsigned port)
{
   OutputPort* out = Output(port);

   if ((out!=0) && IsRunning() && out->CanSend())
      FireEvent(evntOutputReinj, out->ItemId());
}

void dabc::ModuleAsync::ProcessOutputEvent(unsigned port)
{
   OutputPort* out = Output(port);

   int cnt = out->GetMaxLoopLength();

   while (IsRunning() && out->CanSend()) {
      if (!ProcessSend(port)) return;
      if (cnt<0) return;
      if (cnt-- == 0) {
         DOUT3("Port %s performed too many send operations - break the loop", out->ItemName().c_str());
         FireEvent(evntOutputReinj, out->ItemId());
         return;
      }
   }
}

void dabc::ModuleAsync::ActivatePool(unsigned poolindex)
{
   PoolHandle* pool = Pool(poolindex);

   if ((pool!=0) && IsRunning() && pool->CanTakeBuffer())
      FireEvent(evntOutputReinj, pool->ItemId());
}


void dabc::ModuleAsync::ProcessPoolEvent(unsigned indx)
{
   // DOUT0("Module %s process pool event %u", GetName(), pool->NumRequestedBuffer());

   PoolHandle* pool = Pool(indx);

   int cnt = pool->GetMaxLoopLength();

   while (IsRunning() && pool->CanTakeBuffer()) {
      if (!ProcessBuffer(indx)) return;
      if (cnt<0) return;
      if (cnt-- == 0) {
         DOUT3("Pool %s performed too many send operations - break the loop", pool->GetName());
         FireEvent(evntInputReinj, pool->ItemId());
         return;
      }
   }
}



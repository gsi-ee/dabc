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
#include "dabc/ModuleItem.h"
#include "dabc/PoolHandle.h"
#include "dabc/Port.h"
#include "dabc/Buffer.h"

dabc::ModuleAsync::~ModuleAsync()
{
}


void dabc::ModuleAsync::ProcessItemEvent(ModuleItem* item, uint16_t evid)
{
    switch (evid) {
       case evntInput:
          ProcessInputEvent((Port*) item);
          break;
       case evntOutput:
          ProcessOutputEvent((Port*) item);
          break;
       case evntPool:
          ProcessPoolEvent((PoolHandle*) item);
          break;
       case evntTimeout:
          ProcessTimerEvent((Timer*)item);
          break;
       case evntPortConnect:
          ProcessConnectEvent((Port*) item);
          break;
       case evntPortDisconnect:
          ProcessDisconnectEvent((Port*) item);
          break;
       case evntUser:
          ProcessUserEvent(item, evid);
          break;
       default:
          if (evid>evntUser) ProcessUserEvent(item, evid);
          break;
    }
}

void dabc::ModuleAsync::ProcessPoolEvent(PoolHandle* pool)
{
   // this is default implementation

   pool->TakeRequestedBuffer().Release();
}

void dabc::ModuleAsync::OnThreadAssigned()
{
   dabc::Module::OnThreadAssigned();

   // produce initial output events,
   // that user can fill output queue via processing of output event
   for (unsigned n=0;n<NumOutputs();n++) {
      Port* port = Output(n);
      unsigned len = port->OutputQueueCapacity() - port->OutputPending();
      while (len--)
         GetUserEvent(port, evntOutput);
   }
}


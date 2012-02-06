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

#include "dabc/Transport.h"

#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Port.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"

const unsigned dabc::AcknoledgeQueueLength = 2;

long dabc::Transport::gNumTransports = 0;

dabc::Transport::Transport(Reference port) :
   fPortMutex(),
   fPort(port),
   fPool(),
   fTransportState(stNormal)
{
   gNumTransports++;

   if (GetPort()) fPool = Reference(GetPort()->GetMemoryPool());
}

dabc::Transport::~Transport()
{
   gNumTransports--;

   DOUT5(("Transport %p destroyed", this));
}

void dabc::Transport::RegisterTransportDependencies(Object* this_transport)
{
   // dependency between transport and port is bidirectional
   dabc::mgr()->RegisterDependency(this_transport, fPort(), true);

   // here only transport keeps reference on memory pool
   dabc::mgr()->RegisterDependency(this_transport, fPool());
}

void dabc::Transport::CleanupTransport()
{
   // method normally called inside worker cleanup routine
   // it means that we are already on long way to destructor
   Reference port, pool;
   {
      LockGuard guard(fPortMutex);
      port << fPort;
      pool << fPool;

   }
   port.Release();
   pool.Release();

   if (fTransportState==stNormal) fTransportState = stDeleting;
}


void dabc::Transport::CleanupFromTransport(Object* obj)
{
   if ((fPort()==obj) || (fPool()==obj)) {

      bool del = (fTransportState==stNormal);

      // we should release all data from transport
      CleanupTransport();

      if (del) DestroyTransport();
   }
}

void dabc::Transport::CloseTransport(bool witherr)
{
   bool del = (fTransportState==stNormal);

   if (witherr)
      fTransportState = stError;
   else
      if (fTransportState==stNormal) fTransportState = stDeleting;

   // object can be deleted ourself
   if (del) DestroyTransport();
}

bool dabc::Transport::SetPort(Port* port)
{
   // method called from the port thread, therefore it is just notification
   // it is not allowed to changed port reference here, while it is used from
   // transport thread without any mutexes
   //

   if (port==0) {

      // TODO: do we need any confirmation here
      return true;
   }

   {
      LockGuard guard(fPortMutex);
      if (fPort() != port) {
         EOUT(("Something completely wrong - port pointers are differ %p %p", port, fPort()));
         return false;
      }
   }

   PortAssigned();

   return true;
}

bool dabc::Transport::FirePortInput(bool withmutex)
{
   LockGuard guard(withmutex ? &fPortMutex : 0);

   return GetPort() ? GetPort()->FireInput() : false;
}

bool dabc::Transport::FirePortOutput(bool withmutex)
{
   LockGuard guard(withmutex ? &fPortMutex : 0);

   return GetPort() ? GetPort()->FireOutput() : false;
}

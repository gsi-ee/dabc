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
#include "dabc/Transport.h"

#include <stdlib.h>

#include "dabc/Port.h"
#include "dabc/logging.h"
#include "dabc/Device.h"

const unsigned dabc::AcknoledgeQueueLength = 2;

long dabc::Transport::gNumTransports = 0;

dabc::Transport::Transport(Device* dev) :
   fPortMutex(),
   fPort(0),
   fDevice(dev),
   fTransportState(stCreated),
   fTransportErrorFlag(false)
{
   gNumTransports++;

   if (GetDevice()) GetDevice()->TransportCreated(this);
}

dabc::Transport::~Transport()
{
   if (fTransportState!=stDidCleanup)
      if (GetDevice()) GetDevice()->TransportDestroyed(this);

   gNumTransports--;

   DOUT5(("Transport %p destroyed", this));
}

void dabc::Transport::HaltTransport()
{
}

bool dabc::Transport::CanCleanupTransport(bool force)
{
   LockGuard guard(fPortMutex);

   if ((fTransportState==stDidCleanup) || (fTransportState==stNeedCleanup)) return true;

   if (!force) return false;

   if (force && (fPort!=0)) {
      EOUT(("!!! We try to force cleanup at the moment when port pointer is assigned !!!"));
   }

   if (fTransportState!=stDoingCleanup) fTransportState = stNeedCleanup;

   return true;
}

void dabc::Transport::MakeTranportCleanup()
{
   {
      LockGuard guard(fPortMutex);
      if (fTransportState==stDidCleanup) return;

      if (fTransportState==stDoingCleanup) {
         EOUT(("We try to call cleanup twice !!!"));
         return;
      }

      fTransportState = stDoingCleanup;
   }

   CleanupTransport();

   LockGuard guard(fPortMutex);
   fTransportState = stDidCleanup;
}


void dabc::Transport::ErrorCloseTransport()
{
   // method must be called, when error is detected and one wants to close(destroy) transport

   bool old = fTransportErrorFlag;
   fTransportErrorFlag = true;

   if (!old) {
      DOUT3(("Detach port due to error condition"));
      DettachPort();
   }
}


bool dabc::Transport::IsDoingCleanup()
{
   LockGuard guard(fPortMutex);

   return fTransportState == stDoingCleanup;
}

bool dabc::Transport::AttachPort(Port* port, bool sync)
{
   if (port==0) {
      EOUT(("One should specify existing port"));
      return false;
   }

   {
      LockGuard guard(fPortMutex);

      if (fTransportState != stCreated) {
         EOUT(("One only can attach port after transport creation"));
         return false;
      }

      fTransportState = stDoingAttach;
   }

   if (port->AssignTransport(this, sync)) return true;

   {
      LockGuard guard(fPortMutex);
      fTransportState = stNeedCleanup;
   }

   if (fDevice) fDevice->InitiateCleanup();

   return false;
}


void dabc::Transport::DettachPort()
{
   // one cannot set fPort to 0 directly, while it means that
   // transport is inactive and can be cleaned or even destroyed
   // Port must confirm cleanup and only than one can start real object deletion

   {

      LockGuard guard(fPortMutex);

      if (fTransportState == stDoingAttach) {
         // this is situation, when command is submitted to port and we should wait until it comes back
         // we are not allowed to cleanup or destroy transport

         fTransportState = stDoingDettach;
         return;

      }

      // if called several times, ignore such calls
      if (fTransportState == stDoingDettach) return;

      if (fTransportState == stAssigned) {
         if (fPort==0) { EOUT(("Internal error")); exit(147); }
         if (fPort->AssignTransport(0)) return;
      }
   }

   DOUT5(("DettachPort forced %p", this));

   // if fPort is not set or not like to disconnect, force it
   SetPort(0, false);
}

bool dabc::Transport::SetPort(Port* port, bool called_by_port)
{
   // method normally should be called from the port (module) thread
   // it is normally activated via Port::AssignTransport() call
   //

   DOUT4(("Transport %p ::SetPort 1 new = %p old = %p locked = %s", this, port, fPort, DBOOL(fPortMutex.IsLocked())));

   bool cleanup = false;
   bool initiate = false;
   bool res = true;
   bool changed = false;

   {
      LockGuard guard(fPortMutex);

      switch (fTransportState) {

         case stCreated:
            if (port!=0) {
               EOUT(("Wrong argument in SetPort"));
               return false;
            } else {
               fPort = 0;
               cleanup = true;
            }

         case stDoingAttach:
            if (port==0) {
               if (called_by_port) EOUT(("Cannot be, port has no transport pointer yet"));
               cleanup = true;
            } else {
               fPort = port;
               fTransportState = stAssigned;
               changed = true;
            }

            break;

         case stAssigned:
            if (port==fPort) return true;

            if (port==0) {
               fPort = 0;
               cleanup = true;
               changed = true;
            } else {
               EOUT(("Cannot assign new port at such state"));
               return false;
            }

            break;

         case stDoingDettach:
            if (port!=0) {
               EOUT(("Port want to be assigned when transport going to dettach, reject"));
               res = false;
            }
            changed = (fPort!=0);
            fPort = 0;
            cleanup = true;
            break;

         case stNeedCleanup:
            cleanup = true;
            break;

         default:
            if (port!=0) {
               EOUT(("Set port in wrong state, cannot do anything useful"));
               return false;
            }
      }

      initiate = cleanup;

      if (cleanup) {
         if (called_by_port)
            fTransportState = stDoingCleanup;
         else {
            cleanup = false; // we do not call cleanup, just inform device about it
            fTransportState = stNeedCleanup;
         }
      }

   }

   if (changed) PortChanged();

   if (cleanup) {
      CleanupTransport();

      LockGuard guard(fPortMutex);
      fTransportState = stDidCleanup;
   }

   if (initiate && fDevice)
      fDevice->InitiateCleanup();


   DOUT4(("Transport %p ::SetPort new = %p res = %s", this, fPort, DBOOL(res)));

   return res;
}



bool dabc::Transport::IsPortAssigned() const
{
   LockGuard guard(fPortMutex);
   return fPort!=0;
}

dabc::MemoryPool* dabc::Transport::GetPortPool()
{
   LockGuard guard(fPortMutex);
   return fPort ? fPort->GetMemoryPool() : 0;
}

void dabc::Transport::FireInput()
{
   LockGuard guard(fPortMutex);
   if (fPort) fPort->FireInput();
}

void dabc::Transport::FireOutput()
{
   LockGuard guard(fPortMutex);
   if (fPort) fPort->FireOutput();
}

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

#ifndef DABC_LocalTransport
#define DABC_LocalTransport

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_BuffersQueue
#include "dabc/BuffersQueue.h"
#endif


namespace dabc {

   class Port;
   class LocalTransportRef;

   /** \brief %Transport between two ports on the same node
    *
    * \ingroup dabc_all_classes
    *
    */


   class LocalTransport : public Object {

      friend class Port;
      friend class LocalTransportRef;

      protected:

         BuffersQueue fQueue;
         bool fWithMutex;

         WorkerRef fOut;
         unsigned fOutId;
         int fOutSignKind;
         unsigned fSignalOut;

         WorkerRef fInp;
         unsigned fInpId;
         int fInpSignKind;
         unsigned fSignalInp;

         unsigned fConnected;

         enum { MaskInp = 0x1, MaskOut = 0x2, MaskConn = 0x3 };

         LocalTransport(unsigned capacity, bool withmutex);

         virtual ~LocalTransport();

         void SetConnected(bool isinp) { LockGuard lock(QueueMutex()); fConnected |= isinp ? MaskInp : MaskOut; }

         void ConfirmEvent(bool isoutput);

         void PortActivated(int itemkind, bool on);

         void CleanupQueue();

         bool IsConnected() const { LockGuard lock(QueueMutex()); return (fConnected == MaskConn); }

         unsigned NumCanSend() const { LockGuard lock(QueueMutex()); return (fConnected == MaskConn) ? (fQueue.Capacity() - fQueue.Size()) : 1; }

         bool CanSend() const { LockGuard lock(QueueMutex()); return (fConnected == MaskConn) ? !fQueue.Full() : true; }

         bool Send(Buffer& buf);

         bool CanRecv() const { LockGuard lock(QueueMutex()); return !fQueue.Empty(); }

         bool Recv(Buffer& buf);

         void SignalWhenFull();

         inline Mutex* QueueMutex() const { return fWithMutex ? ObjectMutex() : 0; }

         // no need for mutex - capacity is not changed until destructor call
         unsigned Capacity() const { LockGuard lock(QueueMutex()); return fQueue.Capacity(); }

         unsigned Size() const { LockGuard lock(QueueMutex()); return fQueue.Size(); }

         unsigned Full() const { LockGuard lock(QueueMutex()); return fQueue.Full(); }

         Buffer Item(unsigned indx) const { LockGuard lock(QueueMutex()); return fQueue.Item(indx); }

         void Disconnect(bool isinp);

      public:

         static int ConnectPorts(Reference port1ref, Reference port2ref);
   };

   // ________________________________________________________________________

   /** \brief %Reference on the \ref dabc::LocalTransport
    *
    * \ingroup dabc_all_classes
    *
    */

   class LocalTransportRef : public Reference {
      DABC_REFERENCE(LocalTransportRef, Reference, LocalTransport)

      bool IsConnected() const { return GetObject() ? GetObject()->IsConnected() : false; }

      unsigned NumCanSend() const { return GetObject() ? GetObject()->NumCanSend() : 1; }

      bool CanSend() const { return GetObject() ? GetObject()->CanSend() : true; }

      bool Send(Buffer& buf) { if (GetObject()) return GetObject()->Send(buf); buf.Release(); return true; }

      bool CanRecv() const { return GetObject() ? GetObject()->CanRecv() : false; }

      bool Recv(Buffer& buf) { return GetObject() ? GetObject()->Recv(buf) : false; }

      void SignalWhenFull() { if (GetObject()) GetObject()->SignalWhenFull(); }

      void Disconnect(bool isinp) { if (GetObject()) GetObject()->Disconnect(isinp); Release(); }

      void PortActivated(int itemkind, bool on) { if (GetObject()) GetObject()->PortActivated(itemkind, on); }

      void ConfirmEvent(bool isoutput) { if (GetObject()) GetObject()->ConfirmEvent(isoutput); }

      unsigned Size() const { return GetObject() ? GetObject()->Size() : 0; }

      Buffer Item(unsigned indx) const { return GetObject() ? GetObject()->Item(indx) : Buffer(); }

      bool Full() const { return GetObject() ? GetObject()->Full() : false; }
   };



}

#endif

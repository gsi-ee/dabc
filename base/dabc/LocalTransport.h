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
         bool fWithMutex{false};

         WorkerRef fOut;
         unsigned fOutId{0};
         int fOutSignKind{0};
         unsigned fSignalOut{0};

         WorkerRef fInp;
         unsigned fInpId{0};
         int fInpSignKind{0};
         unsigned fSignalInp{0};

         unsigned fConnected{0};

         bool fBlockWhenUnconnected{false}; ///< should queue block when input port not connected, default false
         bool fBlockWhenConnected{false};   ///< should queue block when input port connected, default true

         enum { MaskInp = 0x1, MaskOut = 0x2, MaskConn = 0x3 };

         LocalTransport(unsigned capacity, bool withmutex);

         virtual ~LocalTransport();

         void SetConnected(bool isinp) { LockGuard lock(QueueMutex()); fConnected |= isinp ? MaskInp : MaskOut; }

         void ConfirmEvent(bool isoutput);

         void PortActivated(int itemkind, bool on);

         void CleanupQueue();

         bool IsConnected() const
         {
            LockGuard lock(QueueMutex());
            return (fConnected == MaskConn);
         }

         /** How many buffers can be add to the queue */
         unsigned NumCanSend() const
         {
            LockGuard lock(QueueMutex());
            if (!fQueue.Full()) return fQueue.Capacity() - fQueue.Size();
            // when queue is full and transport in non-blocking state, one buffer can be add (oldest will be lost)
            return ((fConnected == MaskConn) ? fBlockWhenConnected : fBlockWhenUnconnected) ? 0 : 1;
         }

         /** Returns true when send operation will add buffer into the queue
          * When queue is not connected, buffer can always be add to the queue - this
          * will left in the queue newest buffers */
         bool CanSend() const { return NumCanSend() > 0; }

         bool Send(Buffer& buf);

         bool CanRecv() const { LockGuard lock(QueueMutex()); return !fQueue.Empty(); }

         bool Recv(Buffer& buf);

         void SignalWhenFull();

         inline Mutex* QueueMutex() const { return fWithMutex ? ObjectMutex() : nullptr; }

         inline void EnableMutex() { fWithMutex = true; }

         // no need for mutex - capacity is not changed until destructor call
         unsigned Capacity() const { LockGuard lock(QueueMutex()); return fQueue.Capacity(); }

         unsigned Size() const { LockGuard lock(QueueMutex()); return fQueue.Size(); }

         BufferSize_t TotalBuffersSize() const { LockGuard lock(QueueMutex()); return fQueue.TotalBuffersSize(); }

         unsigned Full() const { LockGuard lock(QueueMutex()); return fQueue.Full(); }

         Buffer Item(unsigned indx) const { LockGuard lock(QueueMutex()); return fQueue.Item(indx); }

         void Disconnect(bool isinp, bool witherr = false);

      public:

         static int ConnectPorts(Reference port1ref, Reference port2ref, Command cmd = nullptr);
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

      void Disconnect(bool isinp, bool witherr = false) { if (GetObject()) GetObject()->Disconnect(isinp, witherr); Release(); }

      void PortActivated(int itemkind, bool on) { if (GetObject()) GetObject()->PortActivated(itemkind, on); }

      void ConfirmEvent(bool isoutput) { if (GetObject()) GetObject()->ConfirmEvent(isoutput); }

      unsigned Size() const { return GetObject() ? GetObject()->Size() : 0; }

      BufferSize_t TotalBuffersSize() const { return GetObject() ? GetObject()->TotalBuffersSize() : 0; }

      Buffer Item(unsigned indx) const { return GetObject() ? GetObject()->Item(indx) : Buffer(); }

      bool Full() const { return GetObject() ? GetObject()->Full() : false; }

      bool SubmitCommandTo(bool to_input, Command cmd)
      {
         if (!GetObject()) return false;
         return to_input ? GetObject()->fInp.Submit(cmd) : GetObject()->fOut.Submit(cmd);
      }
   };


}

#endif

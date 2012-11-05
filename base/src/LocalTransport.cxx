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

#include "dabc/LocalTransport.h"

#include "dabc/logging.h"

#include "dabc/Port.h"
#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/Module.h"
#include "dabc/Manager.h"

dabc::LocalTransport::LocalTransport(Reference port, Mutex* mutex, bool owner, bool memcopy, bool doinp, bool doout) :
   Worker(0, "LocalTransport", true),
   Transport(port),
   fOther(0),
   fQueue(GetPort()->InputQueueCapacity()),
   fMutex(mutex),
   fMutexOwner(owner),
   fMemCopy(memcopy),
   fDoInput(doinp),
   fDoOutput(doout),
   fRunning(true)
{
   RegisterTransportDependencies(this);
   
   DOUT0(("Create local transport with queue capacity %u", fQueue.Capacity()));
}

dabc::LocalTransport::~LocalTransport()
{
   DOUT2((" dabc::LocalTransport::~LocalTransport %p other %p queue = %d", this, fOther(), fQueue.Size()));

   if (fMutex && fMutexOwner) {

      DOUT2(("~LocalTransport %p destroy mutex %p", this, fMutex));
      delete fMutex;
      fMutex = 0;
      fMutexOwner = false;
   }

   if (fOther()!=0) {
      EOUT(("Other pointer is still there"));
      fOther.Release();
   }

   if (fQueue.Size()>0) EOUT(("Queue size is not zero!!!"));
   fQueue.Cleanup();
}

void dabc::LocalTransport::CleanupFromTransport(Object* obj)
{
   DOUT4(("LocalTransport::CleanupFromTransport %p", obj));

   if (obj==fOther()) {
      DOUT3(("LocalTransport %p remove other %p", this, obj));
      fOther.Release();
      if (!fMutexOwner) {
         DOUT3(("LocalTransport %p !!!!!!!! FORGET MUTEX !!!!!!!!!!!", this));
         fMutex = 0;
      }
      CloseTransport();
   } else
   if (obj == GetPool()) {
      // pool reference will be cleaned up in parent class

      DOUT4(("LocalTransport::CleanupFromTransport %p memory pool mutex %p locked %s size %u", obj, fMutex, DBOOL((fMutex ? fMutex->IsLocked() : false)), fQueue.Size()));

      fQueue.Cleanup(fMutex);
   }

   DOUT4(("LocalTransport::CleanupFromTransport %p done", obj));

   dabc::Transport::CleanupFromTransport(obj);
}

void dabc::LocalTransport::CleanupTransport()
{
   DOUT5(("LocalTransport::CleanupTransport %p other %p mutex %p refcounter %u", this, fOther(), fMutex, fObjectRefCnt));

   {
      // first of all indicate that we are not longer running and will not accept new buffers
      LockGuard lock(fMutex);
      fRunning = false;
   }

   DOUT5(("LocalTransport::CleanupTransport size %u before", fQueue.Size()));

   fQueue.Cleanup(fMutex);

   DOUT5(("LocalTransport::CleanupTransport size %u after", fQueue.Size()));

   // forgot about mutex
   if (!fMutexOwner) fMutex = 0;

   fOther.Release();

   dabc::Transport::CleanupTransport();

   DOUT3(("LocalTransport::CleanupTransport %p done", this));
}


bool dabc::LocalTransport::Recv(Buffer &buf)
{
   {
      LockGuard lock(fMutex);

      if (fQueue.Size()<=0) return false;

      fQueue.PopBuffer(buf);
   }

   // use port mutex only when other works in different thread
   if (fOther())
     ((LocalTransport*) fOther())->FirePortOutput(fMutex!=0);

   return !buf.null();
}

unsigned dabc::LocalTransport::RecvQueueSize() const
{
   LockGuard lock(fMutex);

   return fQueue.Size();
}

dabc::Buffer& dabc::LocalTransport::RecvBuffer(unsigned indx) const
{
   LockGuard lock(fMutex);

   return fQueue.ItemRef(indx);
}

bool dabc::LocalTransport::Send(const Buffer& buf)
{
   LocalTransport* other = (LocalTransport*) fOther();

   if (buf.null() || (other==0)) return false;

   dabc::Buffer sendbuf;

   if (fMemCopy && !buf.null()) {
      MemoryPool* pool = other->GetPool();
      if (pool)
         sendbuf = pool->CopyBuffer(buf);
   }

   if (sendbuf.null()) sendbuf = buf;

   bool res = false;

   if (other->fRunning)
      res = other->fQueue.Push(sendbuf, fMutex);

   if (res) {
      // here one need port mutex when other transport working in another thread
      other->FirePortInput(fMutex!=0);
   }

   return res;
}

unsigned dabc::LocalTransport::SendQueueSize()
{
   LockGuard lock(fMutex);

   return fOther() ? ((LocalTransport*) fOther())->fQueue.Size() : 0;
}


int dabc::LocalTransport::ConnectPorts(Reference port1ref, Reference port2ref)
{
   Port* port1 = (Port*) port1ref();
   Port* port2 = (Port*) port2ref();

   if ((port1==0) || (port2==0)) return cmd_false;

   Module* m1 = port1->GetModule();
   Module* m2 = port2->GetModule();

   bool memcopy = false;
   Mutex* mutex = 0;

   if ((m1!=0) && (m2!=0))
      if (m1->thread()() != m2->thread()())
         mutex = new Mutex;

   if (port1->GetPoolHandle() && port2->GetPoolHandle()) {
      memcopy = ! port1->GetPoolHandle()->IsName(port2->GetPoolHandle()->GetName());
      if (memcopy) {
         EOUT(("Transport between ports %s %s will be with memcpy",
                 port1->ItemName().c_str(), port2->ItemName().c_str()));
      }
   }

   if (port1->InputQueueCapacity() < port2->OutputQueueCapacity())
      port1->ChangeInputQueueCapacity(port2->OutputQueueCapacity());
   bool doinp1 = port1->InputQueueCapacity() > 0;

   if (port2->InputQueueCapacity() < port1->OutputQueueCapacity())
      port2->ChangeInputQueueCapacity(port1->OutputQueueCapacity());
   bool doinp2 = port2->InputQueueCapacity() > 0;

   if (port1->InlineDataSize() < port2->InlineDataSize())
      port1->ChangeInlineDataSize(port2->InlineDataSize());
   else
   if (port1->InlineDataSize() > port2->InlineDataSize())
      port2->ChangeInlineDataSize(port1->InlineDataSize());

   LocalTransport* tr1 = new LocalTransport(port1ref, mutex, true, memcopy, doinp1, doinp2);
   LocalTransport* tr2 = new LocalTransport(port2ref, mutex, false, memcopy, doinp2, doinp1);

   Reference ref1(tr1);
   Reference ref2(tr2);

   if (!tr1->AssignToThread(m1->thread()) || !tr2->AssignToThread(m2->thread())) {
      ref1.Destroy();
      ref2.Destroy();
      return cmd_false;
   }

   tr1->fOther = Reference(tr2);
   tr2->fOther = Reference(tr1);

   // register many dependencies to be able clear them at right moment
   dabc::mgr()->RegisterDependency(tr1, tr2, true);

   if (port1->AssignTransport(ref1, tr1) && port2->AssignTransport(ref2, tr2)) return cmd_true;

   return cmd_false;
}

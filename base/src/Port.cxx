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

#include "dabc/Port.h"

#include <stdlib.h>

#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"


dabc::Port::Port(int kind, Reference parent, const std::string& name, unsigned queuesize) :
   ModuleItem(kind, parent, name),
   fQueueCapacity(queuesize),
   fRate(),
   fSignal(SignalConfirm),
   fQueue(),
   fBindName(),
   fRateName(),
   fMaxLoopLength(0),
   fReconnectPeriod(-1),
   fReconnectLimit(-1),
   fDoingReconnect(false),
   fOnError()
{
}

dabc::Port::~Port()
{
   DOUT3("PORT %s: destructor %p queue %p", GetName(), this, fQueue());
}


void dabc::Port::ReadPortConfiguration()
{
   fQueueCapacity = Cfg(xmlQueueAttr).AsInt(fQueueCapacity);
   fMaxLoopLength = Cfg(xmlLoopAttr).AsInt(fMaxLoopLength);
   std::string signal = Cfg(xmlSignalAttr).AsStr();
   if (signal == "none") fSignal = SignalNone; else
   if ((signal == "confirm") || (signal == "normal")) fSignal = SignalConfirm; else
   if (signal == "oper")  fSignal = SignalConfirm; else
   if (signal == "every") fSignal = SignalEvery;
   fBindName = Cfg(xmlBindAttr).AsStr(fBindName);
   fRateName = Cfg(xmlRateAttr).AsStr(fRateName);

   ConfigureOnError(Cfg("onerror").AsStr());

   ConfigureReconnect(Cfg(xmlReconnectAttr).AsDouble(-1.), Cfg(xmlNumReconnAttr).AsInt(-1));
}


bool dabc::Port::SetSignaling(EventsProducing kind)
{
   if (IsConnected()) {
      EOUT("Cannot change signaling kind with connected port!!!");
      return false;
   }

   fSignal = kind;
   return true;
}

dabc::ConnectionRequest dabc::Port::GetConnReq(bool force)
{
   dabc::ConnectionRequest req = Par(dabc::ConnectionObject::ObjectName());

   if (!req.null() || !force) return req;

   // at the moment when first connection request will be created,
   // connection manager should be already there

   DOUT0("Create CONNREQ %s ", ItemName().c_str());

   req = new dabc::ConnectionObject(this, dabc::mgr.ComposeAddress("",ItemName()));

   ConfigIO io(dabc::mgr()->cfg());

   io.ReadRecordField(this, dabc::ConnectionObject::ObjectName(), 0, &(req()->Fields()));

   req()->FireParEvent(parCreated);

   return req;
}


void dabc::Port::RemoveConnReq()
{
   DestroyPar(dabc::ConnectionObject::ObjectName());
}

unsigned dabc::Port::QueueCapacity() const
{
   dabc::LockGuard lock(ObjectMutex());
   return fQueueCapacity;
}

void dabc::Port::SetBindName(const std::string& name)
{
   dabc::LockGuard lock(ObjectMutex());
   fBindName = name;
}

std::string dabc::Port::GetBindName() const
{
   dabc::LockGuard lock(ObjectMutex());
   return fBindName;
}


void dabc::Port::SetQueue(Reference& ref)
{
   fQueue << ref;
   if (fQueue.null()) return;

   fQueue()->SetConnected(IsInput());

   // change capacity field under mutex, while it can be accessed outside the thread
   dabc::LockGuard lock(ObjectMutex());
   fQueueCapacity = fQueue()->Capacity();
}


void dabc::Port::DoStart()
{
   fQueue.PortActivated(GetType(), true);
}

void dabc::Port::DoStop()
{
   fQueue.PortActivated(GetType(), false);
}

void dabc::Port::DoCleanup()
{
   Disconnect();
   fRate.Release();
}

void dabc::Port::ObjectCleanup()
{
   DOUT3("Port %s cleanup inp:%s out:%s", ItemName().c_str(), DBOOL(IsInput()), DBOOL(IsOutput()));

   // remove queue
   Disconnect();

   fRate.Release();

   dabc::ModuleItem::ObjectCleanup();
}


void dabc::Port::SetRateMeter(const Parameter& ref)
{
   fRate = ref;

   if (fRate.GetUnits().empty())
      fRate.SetUnits("MB");

   // TODO: do we need dependency on the rate parameter or it should remain until we release it
   // dabc::mgr()->RegisterDependency(this, fInpRate());
}

int dabc::Port::GetMaxLoopLength()
{
   switch (SignalingKind()) {
      case SignalNone:
         return 0;
      case SignalConfirm:
      case SignalOperation:
         return fMaxLoopLength ? fMaxLoopLength : QueueCapacity();
      case Port::SignalEvery:
         return -1;
   }
   return -1;
}

bool dabc::Port::SubmitCommandToTransport(Command cmd)
{
   if (IsInput()) return fQueue.SubmitCommandTo(false, cmd);
   if (IsOutput()) return fQueue.SubmitCommandTo(true, cmd);
   return false;
}

bool dabc::Port::TryNextReconnect(bool caused_by_error)
{
   if (!caused_by_error) {
      return false;
   }

   if ((fReconnectPeriod>0) && (--fReconnectLimit>=0)) return true;

   SetDoingReconnect(false);

   if (fOnError == "none") {
      // do nothing
   } else
   if (fOnError == "stop") {
      DOUT0("Stop module %s due to error on port %s", DNAME(GetParent()), ItemName().c_str());
      StopModule();
   } else
   if (fOnError == "exit") {
      DOUT0("Exit application due to error on port %s", ItemName().c_str());
      dabc::mgr.StopApplication();
   } else
   if (fOnError == "abort") {
      DOUT0("Abort application due to error on port %s", ItemName().c_str());
      abort();
   } else {
      Disconnect();
   }

   return false;
}


// ================================================================

int dabc::PortRef::GetSignalingKind()
{
   if (GetObject()==0) return Port::SignalNone;
   dabc::Command cmd("GetSignalingKind");
   cmd.SetStr("Port", GetObject()->GetName());
   if (GetModule().Execute(cmd) == cmd_true)
      return cmd.GetInt("Kind");
   return Port::SignalNone;
}

bool dabc::PortRef::Disconnect(bool witherr)
{
   if (GetObject()==0) return false;
   dabc::Command cmd("DisconnectPort");
   cmd.SetStr("Port", GetObject()->GetName());
   cmd.SetBool("WithErr", witherr);
   return GetModule().Execute(cmd) == cmd_true;
}

dabc::PortRef dabc::PortRef::GetBindPort()
{
   std::string name;
   if (GetObject()) name = GetObject()->GetBindName();
   if (name.empty()) return 0;
   return GetModule().FindChild(name.c_str());
}

bool dabc::PortRef::IsConnected()
{
   if (GetObject()==0) return false;
   dabc::Command cmd("IsPortConnected");
   cmd.SetStr("Port", GetObject()->GetName());
   return GetModule().Execute(cmd) == cmd_true;
}


dabc::ConnectionRequest dabc::PortRef::MakeConnReq(const std::string& url, bool isserver)
{
   dabc::ConnectionRequest req;

   if (null() || GetModule().null()) return req;

   dabc::Command cmd("MakeConnReq");
   cmd.SetStr("Port", GetObject()->GetName());
   cmd.SetStr("Url", url);
   cmd.SetBool("IsServer", isserver);

   if (GetModule().Execute(cmd) == cmd_true)
      req = cmd.GetRef("ConnReq");

   return req;
}

// ====================================================================================


dabc::InputPort::InputPort(Reference parent,
                           const std::string& name,
                           unsigned queuesize) :
   Port(dabc::mitInpPort, parent, name, queuesize)
{
   // only can do it here, while in Port Cfg() cannot correctly locate InputPort as class

   ReadPortConfiguration();
}

dabc::InputPort::~InputPort()
{
   DOUT4("PORT: destructor %p", this);
}

bool dabc::InputPort::SkipBuffers(unsigned cnt)
{
   while (cnt>0) {
      if (!CanRecv()) return false;
      dabc::Buffer buf = Recv();
      buf.Release();
      cnt--;
   }

   return true;
}

unsigned dabc::InputPort::NumStartEvents()
{
   switch (SignalingKind()) {
      case SignalNone:
         return 0;
      case SignalConfirm:
      case SignalOperation:
         return CanRecv() ? 1 : 0;
      case Port::SignalEvery:
         return NumCanRecv();
   }
   return 0;
}

dabc::Buffer dabc::InputPort::Recv()
{
   Buffer buf;
   fQueue.Recv(buf);
   fRate.SetValue(buf.GetTotalSize()/1024./1024.);
   return buf;
}


// ====================================================================================

dabc::OutputPort::OutputPort(Reference parent,
                             const std::string& name,
                             unsigned queuesize) :
   Port(dabc::mitOutPort, parent, name, queuesize),
   fSendallFlag(false)
{
   // only can do it here, while in Port Cfg() cannot correctly locate OutputPort as class
   ReadPortConfiguration();
}

dabc::OutputPort::~OutputPort()
{
   DOUT4("PORT: destructor %p", this);
}


bool dabc::OutputPort::Send(dabc::Buffer& buf)
{
   fRate.SetValue(buf.GetTotalSize()/1024./1024.);

   bool res = fQueue.Send(buf);

   if (!buf.null() && !fQueue.null() && res)
      EOUT("Should not happen queue %p  buf %p", fQueue.GetObject(), buf.GetObject());

   if (!res) {
      EOUT("PORT %s fail to send buffer %u", ItemName().c_str(), buf.GetTotalSize());
      buf.Release();
   }

   return res;
}


unsigned dabc::OutputPort::NumStartEvents()
{
   switch (SignalingKind()) {
      case SignalNone:
         return 0;
      case SignalConfirm:
      case SignalOperation:
         return CanSend() ? 1 : 0;
      case Port::SignalEvery:
         return NumCanSend();
   }
   return 0;
}


// ====================================================================================


dabc::PoolHandle::PoolHandle(Reference parent,
                             Reference pool,
                             const std::string& name,
                             unsigned queuesize) :
   Port(dabc::mitPool, parent, name, queuesize),
   fPool(pool)
{
   // only can do it here, while in Port Cfg() cannot correctly locate PoolHandle as class
   ReadPortConfiguration();
}


dabc::PoolHandle::~PoolHandle()
{
   DOUT4("PoolHandle: destructor %p", this);
}


unsigned dabc::PoolHandle::NumStartEvents()
{
   switch (SignalingKind()) {
      case SignalNone:
         return 0;
      case SignalConfirm:
      case SignalOperation:
         return (NumRequestedBuffer() > 0) ? 1 : 0;
      case Port::SignalEvery:
         return NumRequestedBuffer();
   }
   return 0;
}


dabc::Buffer dabc::PoolHandle::TakeBuffer(BufferSize_t size)
{
   if (QueueCapacity()==0) return ((MemoryPool*)fPool())->TakeBuffer(size);

   return TakeRequestedBuffer();
}


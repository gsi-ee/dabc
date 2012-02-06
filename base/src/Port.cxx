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

#include "dabc/logging.h"
#include "dabc/Module.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"

dabc::PortException::PortException(Port* port, const char* info) throw() :
   dabc::ModuleItemException(port, info)
{
}

dabc::Port* dabc::PortException::GetPort() const throw()
{
   return dynamic_cast<Port*>(GetItem());
}

// ________________________________________________________________

dabc::Port::Port(Reference parent,
                 const char* name,
                 PoolHandle* pool,
                 unsigned recvqueue,
                 unsigned sendqueue) :
   ModuleItem(dabc::mitPort, parent, name),
   fPoolHandle(),
   fPool(),
   fInputQueueCapacity(recvqueue),
   fInputPending(0),
   fOutputQueueCapacity(sendqueue),
   fOutputPending(0),
   fInlineDataSize(0),
   fInpRate(),
   fOutRate(),
   fTrHandle(),
   fTransport(0)
{
   if (recvqueue > 0)
      fInputQueueCapacity = Cfg(xmlInputQueueSize).AsInt(recvqueue);

   if (sendqueue > 0)
      fOutputQueueCapacity = Cfg(xmlOutputQueueSize).AsInt(sendqueue);

   fInlineDataSize = Cfg(xmlInlineDataSize).AsInt(fInlineDataSize);

   if (pool!=0) {
      fPoolHandle = Reference(pool);
      dabc::mgr()->RegisterDependency(this, fPoolHandle());
      fPool = pool->GetPoolRef();
      dabc::mgr()->RegisterDependency(this, fPool());
   }

   if (GetMemoryPool()==0) {
      EOUT(("No memory pool for port %s", GetName()));
      exit(055);
   }

   DOUT4(("Create Port %s with inp %u out %u hdr %u", GetName(), fInputQueueCapacity, fOutputQueueCapacity, fInlineDataSize));
}

dabc::Port::~Port()
{
   if (fTransport != 0)
      EOUT(("Port is not cleaned up - why????"));

   DOUT4(("PORT: destructor %p", this));
}


dabc::ConnectionRequest dabc::Port::GetConnReq(bool force)
{
   dabc::ConnectionRequest req = Par(dabc::ConnectionObject::ObjectName());

   if (!req.null() || !force) return req;

   // at the moment when first connection request will be created,
   // connection manager is activated
   dabc::mgr.CreateConnectionManager();

   req = new dabc::ConnectionObject(this, dabc::mgr.ComposeUrl(this));

   ConfigIO io(dabc::mgr()->cfg());

   io.ReadRecord(this, dabc::ConnectionObject::ObjectName(), req());

   req()->FireEvent(parCreated);

   return req;
}


dabc::ConnectionRequest dabc::Port::MakeConnReq(const std::string& url, bool isserver)
{
   dabc::ConnectionRequest req = GetConnReq(true);

   req.SetInitState();

   req.SetRemoteUrl(url);
   req.SetServerSide(isserver);

   return req;
}

void dabc::Port::RemoveConnReq()
{
   DestroyPar(dabc::ConnectionObject::ObjectName());
}


void dabc::Port::ProcessEvent(const EventId& evid)
{
   switch (evid.GetCode()) {
      case evntInput:
         if (fTransport!=0) fInputPending++;
         ProduceUserEvent(evntInput);
         break;

      case evntOutput:
         if (fTransport!=0) fOutputPending--;
         ProduceUserEvent(evntOutput);
         break;

      default:
         ModuleItem::ProcessEvent(evid);
         break;
   }
}

dabc::PoolHandle* dabc::Port::GetPoolHandle() const
{
   return (dabc::PoolHandle*) fPoolHandle();
}

dabc::MemoryPool* dabc::Port::GetMemoryPool()
{
   return (dabc::MemoryPool*) fPool();
}

unsigned dabc::Port::NumOutputBuffersRequired() const
{
   // Here we calculate number of buffers, which are required for data output

   unsigned sz = OutputQueueCapacity();

   // this is additional buffers for sending acknowledge packets (when will be required)
   if (InputQueueCapacity() > 0) sz++;

   return sz;
}

unsigned dabc::Port::NumInputBuffersRequired() const
{
   // here we calculate number of buffers, which are required for data input

   unsigned sz = InputQueueCapacity();

   // this is additional buffers for receiving acknowledge packets (when will be required)
   if ((OutputQueueCapacity() > 0) && (sz<AcknoledgeQueueLength)) sz = AcknoledgeQueueLength;

   return sz;
}

bool dabc::Port::AssignTransport(Reference handle, Transport* tr, bool sync)
{
   Command cmd("AssignTransport");
   cmd.SetRef("TrHandle", handle);
   cmd.SetPtr("#Transport", tr);
   if (sync) return Execute(cmd);
   return Submit(cmd);
}

void dabc::Port::SetInpRateMeter(const Parameter& ref)
{
   fInpRate = ref.Ref();

   if (fInpRate.GetUnits().empty())
      fInpRate.SetUnits("MB");

   // TODO: do we need dependency on the rate parameter or it should remain until we release it
   // dabc::mgr()->RegisterDependency(this, fInpRate());
}

void dabc::Port::SetOutRateMeter(const Parameter& ref)
{
   fOutRate = ref.Ref();

   if (fOutRate.GetUnits().empty())
      fOutRate.SetUnits("MB");

   // TODO: do we need dependency on the rate parameter or it should remain until we release it
   // dabc::mgr()->RegisterDependency(this, fOutRate());
}

int dabc::Port::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("AssignTransport")) {

      Transport* oldtr = fTransport;
      if (oldtr) oldtr->SetPort(0);
      fTrHandle.Destroy();

      fTransport = (Transport*) cmd.GetPtr("#Transport");
      fTrHandle = cmd.GetRef("TrHandle");

      DOUT4(("%s Get AssignTransport command old %p new %p", ItemName().c_str(), oldtr, fTransport));

      if (fTransport!=0)
         if (!fTransport->SetPort(this)) {
            fTransport = 0;
            fTrHandle.Destroy();
         }

      fInputPending = 0;
      fOutputPending = 0;

      if ((oldtr!=0) && (fTransport==0)) {
         ProduceUserEvent(evntPortDisconnect);
         GetConnReq(true).ChangeState(ConnectionObject::sDisconnected, true);
      } else
      if ((oldtr==0) && (fTransport!=0)) {
         ProduceUserEvent(evntPortConnect);
         GetConnReq(true).ChangeState(ConnectionObject::sConnected, true);
      }

      if (GetModule()->IsRunning() && fTransport)
         fTransport->StartTransport();

      DOUT5(("%s processed AssignTransport command cansend %s", ItemName().c_str(), DBOOL(CanSend())));

      return cmd_true;
   }

   return ModuleItem::ExecuteCommand(cmd);
}

void dabc::Port::ObjectCleanup()
{
   DOUT4(("PORT:%s ObjectCleanup obj=%p", ItemName().c_str(), this));

   // FIXME: should we cleanup transport queue this way? Can transport do itself this job?
   // while (SkipInputBuffers(1));

   // remove transport
   Disconnect();

   // clear pool handle reference
   fPoolHandle.Release();

   // clear pool reference
   fPool.Release();

   fInpRate.Release();
   fOutRate.Release();

   dabc::ModuleItem::ObjectCleanup();

   DOUT4(("PORT:%s ObjectCleanup done numref: %u", ItemName().c_str(), NumReferences()));
}

void dabc::Port::ObjectDestroyed(Object* obj)
{
   if (fTrHandle()==obj) {
      DOUT3(("PORT:%s Transport destroyed", ItemName().c_str()));
      Disconnect();
      ProduceUserEvent(evntPortDisconnect);
      GetConnReq(true).ChangeState(ConnectionObject::sBroken, true);
      DOUT2(("Inform that state changed port %s", ItemName().c_str()));
   } else
   if (fPool()==obj) {
      // FIXME: should we delete ourself here or produce disconnect message ?
      fPool.Release();
   } else
   if (fPoolHandle()==obj) {
      fPoolHandle.Release();
   }

   dabc::ModuleItem::ObjectDestroyed(obj);
}

void dabc::Port::Disconnect()
{
   if (fTransport!=0) {
      fTransport->SetPort(0);
      fTransport = 0;
   }

   DOUT3(("Port %s destroy transport %p", ItemName().c_str(), fTrHandle()));

   fTrHandle.Destroy();
   fInputPending = 0;
   fOutputPending = 0;
}

void dabc::Port::DoStart()
{
   if (fTransport!=0)
      fTransport->StartTransport();
}

void dabc::Port::DoStop()
{
   DOUT5(("PORT:%s DoStop=%p transport=%p", ItemName().c_str(), this, fTransport));

   if (fTransport!=0)
      fTransport->StopTransport();
}

bool dabc::Port::Send(const Buffer& buf) throw (PortOutputException)
{
   // Sends buffer to the Port
   // If return true, operation is successful
   // If return false, error is occurs
   // In all cases buffer is no longer available to user and will be released by the core

   if (buf.null()) throw PortOutputException(this, "No buffer specified");

   double size_mb = buf.GetTotalSize()/1024./1024.;

   if (fTransport==0) {
      fOutRate.SetDouble(size_mb);
      (const_cast<Buffer*>(&buf))->Release();
      return false;
   }

   if (!CanSend()) {
      (const_cast<Buffer*>(&buf))->Release();
      throw PortOutputException(this, "Output blocked - queue is full");
   }

   if (!fTransport->Send(buf)) {
      (const_cast<Buffer*>(&buf))->Release();
      return false;
   }

   fOutRate.SetDouble(size_mb);

   fOutputPending++;
   return true;
}

dabc::Buffer dabc::Port::Recv() throw (PortInputException)
{
   if (fTransport==0) throw PortInputException(this, "No transport");

   if (fInputPending==0)
      EOUT(("No input pending - recv should fail!!!"));
   else
      fInputPending--;

   dabc::Buffer buf;

   if (fTransport->Recv(buf))
      fInpRate.SetDouble(buf.GetTotalSize()/1024./1024.);
//   else 
//     EOUT(("Didnot get buffer from transport pending %d", fInputPending));

   return buf;
}

bool dabc::Port::SkipInputBuffers(unsigned num)
{
   while (num-->0) {
      if (!CanRecv()) return false;
      dabc::Buffer buf = Recv();
      if (buf.null()) return false;
      buf.Release();
   }
   return true;
}

int dabc::Port::GetTransportParameter(const char* name)
{
   return fTransport ? fTransport->GetParameter(name) : 0;
}

dabc::Buffer& dabc::Port::InputBuffer(unsigned indx) const
{
   if ((fTransport==0) || (indx>=InputPending()))
      throw dabc::Exception("Wrong argument in Port::InputBuffer call");

   return fTransport->RecvBuffer(indx);
}


// ====================================================================

bool dabc::PortRef::IsConnected() const
{
   return GetObject() ? GetObject()->IsConnected() : false;
}

bool dabc::PortRef::Disconnect()
{
   if (!IsConnected()) return true;

   if (CanSubmitCommand())
      return GetObject()->AssignTransport(Reference(), 0, true);

   GetObject()->Disconnect();

   return true;
}

dabc::Reference dabc::PortRef::GetPool()
{
   if (GetObject()) return GetObject()->fPool.Ref();

   return dabc::Reference();
}


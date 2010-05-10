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
#include "dabc/Port.h"

#include "dabc/logging.h"
#include "dabc/Module.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Parameter.h"

dabc::PortException::PortException(Port* port, const char* info) throw() :
   dabc::ModuleItemException(port, info)
{
}

dabc::Port* dabc::PortException::GetPort() const throw()
{
   return dynamic_cast<Port*>(GetItem());
}

// ________________________________________________________________

dabc::Port::Port(Basic* parent,
                 const char* name,
                 PoolHandle* pool,
                 unsigned recvqueue,
                 unsigned sendqueue,
                 BufferSize_t usrheadersize) :
   ModuleItem(dabc::mitPort, parent, name),
   fPool(pool),
   fInputQueueCapacity(recvqueue),
   fInputPending(0),
   fOutputQueueCapacity(sendqueue),
   fOutputPending(0),
   fUsrHeaderSize(usrheadersize),
   fTransport(0),
   fInpRate(0),
   fOutRate(0)
{
   if (recvqueue > 0)
      fInputQueueCapacity = GetCfgInt(xmlInputQueueSize, recvqueue);

   if (sendqueue > 0)
      fOutputQueueCapacity = GetCfgInt(xmlOutputQueueSize, sendqueue);

   if (usrheadersize > 0)
      fUsrHeaderSize = GetCfgInt(xmlHeaderSize, usrheadersize);

   DOUT5(("Create Port %s with inp %u out %u hdr %u", GetName(), fInputQueueCapacity, fOutputQueueCapacity, fUsrHeaderSize));
}

dabc::Port::~Port()
{
   DOUT5(( "Delete port %s %s tr = %p", GetName(), GetFullName().c_str(), fTransport));

   Disconnect();

   DOUT5(( "Delete port %s  done", GetName()));
}

void dabc::Port::ProcessEvent(EventId evid)
{
   switch (GetEventCode(evid)) {
      case evntInput:
         fInputPending++;
         ProduceUserEvent(evntInput);
         break;

      case evntOutput:
         fOutputPending--;
         ProduceUserEvent(evntOutput);
         break;

      default:
         ModuleItem::ProcessEvent(evid);
   }
}


dabc::MemoryPool* dabc::Port::GetMemoryPool() const
{
   return fPool ? fPool->getPool() : 0;
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

bool dabc::Port::AssignTransport(Transport* tr, bool sync)
{
   Command* cmd = new Command("AssignTransport");
   cmd->SetPtr("#Transport", tr);
   if (sync) return Execute(cmd)==cmd_true;
   return Submit(cmd);
}

int dabc::Port::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("AssignTransport")) {
      Transport* oldtr = fTransport;
      fTransport = (Transport*) cmd->GetPtr("#Transport");

      DOUT5(("%s Get AssignTransport command old %p new %p", GetFullName().c_str(), oldtr, fTransport));

      if (oldtr!=0) oldtr->AssignPort(0);

      if (fTransport!=0) fTransport->AssignPort(this);

      fInputPending = 0;
      fOutputPending = 0;

      if ((oldtr!=0) && (fTransport==0)) ProduceUserEvent(evntPortDisconnect); else
      if ((oldtr==0) && (fTransport!=0)) ProduceUserEvent(evntPortConnect);

      if (GetModule()->IsRunning() && fTransport)
         fTransport->StartTransport();

      DOUT5(("%s processed AssignTransport command cansend %s", GetFullName().c_str(), DBOOL(CanSend())));
   } else
      return cmd_false;

   return cmd_true;
}

void dabc::Port::Disconnect()
{
   if (fTransport!=0) fTransport->AssignPort(0);
   fTransport = 0;
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
   if (fTransport!=0)
      fTransport->StopTransport();
}

void dabc::Port::DoHalt()
{
   DOUT3(("Halt port %s Transport %p", GetFullName().c_str(), fTransport));

   while (SkipInputBuffers(1));

   Disconnect();
}

bool dabc::Port::Send(Buffer* buf) throw (PortOutputException)
{
   // Sends buffer to the Port
   // If return true, operation is successful
   // If return false, error is occurs
   // In all cases buffer is no longer available to user and will be released by the core

   if (buf==0) throw PortOutputException(this, "No buffer specified");

   if (fTransport==0) {
      dabc::Buffer::Release(buf);
      return false;
   }

   if (!CanSend()) {
      dabc::Buffer::Release(buf);
      throw PortOutputException(this, "Output blocked - queue is full");
   }

   double size_mb = (buf->GetTotalSize() + buf->GetHeaderSize())/1024./1024.;

   if (!fTransport->Send(buf)) {
      dabc::Buffer::Release(buf);
      return false;
   }

   if (fOutRate)
      fOutRate->AccountValue(size_mb);

   fOutputPending++;
   return true;
}

dabc::Buffer* dabc::Port::Recv() throw (PortInputException)
{
   if (fTransport==0) throw PortInputException(this, "No transport");

   dabc::Buffer* buf(0);

   if (!fTransport->Recv(buf)) return 0;

   if (fInpRate && buf)
      fInpRate->AccountValue((buf->GetTotalSize() + buf->GetHeaderSize())/1024./1024.);

   fInputPending--;
   return buf;
}

bool dabc::Port::SkipInputBuffers(unsigned num)
{
   while (num-->0) {
      if (!CanRecv()) return false;
      dabc::Buffer::Release(Recv());
   }
   return true;
}

int dabc::Port::GetTransportParameter(const char* name)
{
   return fTransport ? fTransport->GetParameter(name) : 0;
}

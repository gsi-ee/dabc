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
                 BufferSize_t usrheadersize,
                 bool ackn) :
   ModuleItem(dabc::mitPort, parent, name),
   fPool(pool),
   fInputQueueCapacity(recvqueue),
   fInputPending(0),
   fOutputQueueCapacity(sendqueue),
   fOutputPending(0),
   fUseAcknoledges(ackn),
   fUsrHeaderSize(usrheadersize),
   fTransport(0),
   fInpRate(0),
   fOutRate(0)
{
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

   // this is additional buffers for sending ackn packets
   if (IsUseAcknoledges() && (InputQueueCapacity() > 0)) sz++;

   return sz;
}

unsigned dabc::Port::NumInputBuffersRequired() const
{
   // here we calculate number of buffers, which are required for data input

   unsigned sz = InputQueueCapacity();

   if (IsUseAcknoledges() && (OutputQueueCapacity() > 0) &&
         (InputQueueCapacity()<AcknoledgeQueueLength)) sz = AcknoledgeQueueLength;

   return sz;
}

bool dabc::Port::AssignTransport(Transport* tr, CommandClientBase* cli)
{
   Command* cmd = new Command("AssignTransport");
   cmd->SetPtr("#Transport", tr);

   if (ProcessorThread()!=0) {
      if (cli!=0) cmd = cli->Assign(cmd);
      Submit(cmd);
   }
                        else Execute(cmd);

   return true;
}

int dabc::Port::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("AssignTransport")) {
      DOUT5(("%s Get AssignTransport command", GetFullName().c_str()));
      Transport* oldtr = fTransport;
      fTransport = (Transport*) cmd->GetPtr("#Transport");

      if (oldtr!=0) oldtr->AssignPort(0);

      if (fTransport!=0) fTransport->AssignPort(this);

      fInputPending = 0;
      fOutputPending = 0;

      ProduceUserEvent(fTransport==0 ? evntPortDisconnect : evntPortConnect);
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

bool dabc::Port::Send(Buffer* buf) throw (PortOutputException)
{
   // Sends buffer to the Port
   // If return true, operation is succsessfull
   // If return false, error is occures
   // In all cases buffer is no longer available to user and will be released by the core

   if (buf==0) throw PortOutputException(this, "No buffer specified");

   if (fTransport==0) {
      dabc::Buffer::Release(buf);
      return false;
//      throw PortOutputException(this, "No transport");
   }

   if (OutputBlocked()) {
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

bool dabc::Port::Recv(Buffer* &buf) throw (PortInputException)
{
   if (fTransport==0) throw PortInputException(this, "No transport");

   if (!fTransport->Recv(buf)) return false;

   if (fInpRate && buf)
      fInpRate->AccountValue((buf->GetTotalSize() + buf->GetHeaderSize())/1024./1024.);

   fInputPending--;
   return true;
}

bool dabc::Port::SkipInputBuffers(unsigned num)
{
   dabc::Buffer* buf = 0;
   while (num-->0) {
      if (InputBlocked()) return false;
      if (!Recv(buf)) return false;
      dabc::Buffer::Release(buf);
   }
   return true;
}

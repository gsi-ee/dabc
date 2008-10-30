#include "dabc/LocalTransport.h"

#include "dabc/logging.h"

#include "dabc/Port.h"
#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/Module.h"
#include "dabc/Manager.h"
#include "dabc/CommandsSet.h"

dabc::LocalTransport::LocalTransport(LocalDevice* dev, Port* port, Mutex* mutex, bool owner, bool memcopy) :
   Transport(dev),
   fOther(0), 
   fQueue(port->InputQueueCapacity()),
   fMutex(mutex),
   fMutexOwner(owner),
   fMemCopy(memcopy)
{
}

dabc::LocalTransport::~LocalTransport()
{
   DOUT5((" dabc::LocalTransport::~LocalTransport %p calling queue = %d", this, fQueue.Size()));
   
   if (fMutex && fMutexOwner) {
      if (fOther) fOther->fMutex = 0;
      delete fMutex;
      fMutex = 0;   
   }
   
   if (fOther!=0)
      fOther->fOther = 0; // remove reference on ourself 
    
   fQueue.Cleanup();
}

bool dabc::LocalTransport::_IsReadyForCleanup()
{
   if (!Transport::_IsReadyForCleanup()) return false;
   
   return fOther ? !fOther->IsPortAssigned() : true;
}

void dabc::LocalTransport::PortChanged() 
{
   if (IsPortAssigned()) return;
   
   DOUT5(("dabc::LocalTransport::PortChanged %p", this));
   
   // do we need cleanup of queue here, using mutex
   fQueue.Cleanup(fMutex);
}


bool dabc::LocalTransport::Recv(Buffer* &buf)
{
   buf = 0; 
    
   { 
      LockGuard lock(fMutex);
      
      if (fQueue.Size()<=0) return false; 
      
      buf = fQueue.Pop();
   }

   if (fOther) fOther->FireOutput();
   
   return (buf!=0);
}

unsigned dabc::LocalTransport::RecvQueueSize() const
{
   LockGuard lock(fMutex);
   
   return fQueue.Size();
}

dabc::Buffer* dabc::LocalTransport::RecvBuffer(unsigned indx) const
{
   LockGuard lock(fMutex);
   
   return fQueue.Item(indx);
}

bool dabc::LocalTransport::Send(Buffer* buf)
{
   if (buf==0) return false;
   
   if (fOther==0) {
      dabc::Buffer::Release(buf);
      return false;
   }
   
   if (fMemCopy && buf) {
      MemoryPool* pool = fOther->GetPortPool();
      Buffer* newbuf = pool ? pool->TakeBuffer(buf->GetTotalSize(), buf->GetHeaderSize()) : 0;
      if (newbuf!=0) 
         newbuf->CopyFrom(buf);
      else
         EOUT(("Cannot memcpy in localtransport while no new buffer can be ordered"));
      dabc::Buffer::Release(buf);
      buf = newbuf;
   }
   
   bool res = false;
   
   {
      LockGuard lock(fMutex);
      
      res = fOther->fQueue.MakePlaceForNext();
      if (res) fOther->fQueue.Push(buf);
   }
   
   if (res) fOther->FireInput();
       else dabc::Buffer::Release(buf);
         
   return res;
}

unsigned dabc::LocalTransport::SendQueueSize()
{
   LockGuard lock(fMutex);
   
   return fOther ? fOther->fQueue.Size() : 0;
}

// _______________________________________________________________

dabc::NullTransport::NullTransport(LocalDevice* dev) : 
   Transport(dev)
{
}

bool dabc::NullTransport::Send(Buffer* buf)
{
   dabc::Buffer::Release(buf);
      
   FireOutput();
   
   return true;
}

// ____________________________________________________________

dabc::LocalDevice::LocalDevice(Basic* parent, const char* name) :
   Device(parent, name)
{
   AssignProcessorToThread(GetManager()->ProcessorThread());
}

bool dabc::LocalDevice::ConnectPorts(Port* port1, Port* port2, CommandClientBase* cli)
{
   if ((port1==0) || (port2==0)) return false;
   
   Module* m1 = port1->GetModule();
   Module* m2 = port2->GetModule();
   
   bool memcopy = false;
   
   Mutex* mutex = 0;
   
   if ((m1!=0) && (m2!=0))
      if (m1->ProcessorThread() != m2->ProcessorThread()) 
         mutex = new Mutex;
      
   if (port1->Pool() && port2->Pool()) {
      memcopy = ! port1->Pool()->IsName(port2->Pool()->GetName());
      if (memcopy) {
         EOUT(("Transport between ports %s %s will be with memopy",  
                 port1->GetFullName().c_str(), port2->GetFullName().c_str()));
      }
   }
   
   if (port1->InputQueueCapacity() < port2->OutputQueueCapacity())
      port1->ChangeInputQueueCapacity(port2->OutputQueueCapacity());

   if (port2->InputQueueCapacity() < port1->OutputQueueCapacity())
      port2->ChangeInputQueueCapacity(port1->OutputQueueCapacity());
   
   if (port1->UserHeaderSize() < port2->UserHeaderSize())
      port1->ChangeUserHeaderSize(port2->UserHeaderSize());
   else   
   if (port1->UserHeaderSize() > port2->UserHeaderSize())
      port2->ChangeUserHeaderSize(port1->UserHeaderSize());
   
   LocalTransport* tr1 = new LocalTransport(this, port1, mutex, true, memcopy);
   LocalTransport* tr2 = new LocalTransport(this, port2, mutex, false, memcopy);
   
   tr1->fOther = tr2;
   tr2->fOther = tr1;
   
   bool res = port1->AssignTransport(tr1, cli);
   res = res && port2->AssignTransport(tr2, cli);
   
   return res;
}

bool dabc::LocalDevice::MakeNullTransport(Port* port)
{
   if (port==0) return false;
   
   NullTransport* tr = new NullTransport(this); 
   
   port->AssignTransport(tr);
   
   DOUT3(("Create null device for port %s", port->GetFullName().c_str()));
   
   return true;
}

int dabc::LocalDevice::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   if (cmd->GetBool("IsNull")) 
      return MakeNullTransport(port);
      
   dabc::Port* port2 = GetManager()->FindPort(cmd->GetPar("Port2Name"));
   
   if (port2==0) return false;
   
   return ConnectPorts(port, port2);
}

int dabc::LocalDevice::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_true; 
    
   if (cmd->IsName(CommandPortConnect::CmdName())) {
      Port* port1 = GetManager()->FindPort(cmd->GetPar("Port1Name"));    
      Port* port2 = GetManager()->FindPort(cmd->GetPar("Port2Name"));
      
      if ((port1==0) || (port2==0)) {
         EOUT(("Port(s) are not found"));
         cmd_res = cmd_false;   
      } else {
         CommandsSet* set = new CommandsSet(GetManager(), cmd);
          
         bool res = ConnectPorts(port1, port2, set); 
         
         if (!res) {
            delete set;
            cmd_res = cmd_false;   
         } else {
            cmd_res = cmd_postponed;
            dabc::CommandsSet::Completed(set, 3.);
            DOUT1(("LocalTransport complets commandset %p", set));
         }
      }
   } else
      cmd_res = dabc::Device::ExecuteCommand(cmd);
      
   return cmd_res;
}


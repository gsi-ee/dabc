#include "dabc/Transport.h"

#include "dabc/Port.h"
#include "dabc/logging.h"
#include "dabc/Device.h"

const unsigned dabc::AcknoledgeQueueLength = 2;

long dabc::Transport::gNumTransports = 0;

dabc::Transport::Transport(Device* dev) : 
   fPortMutex(),
   fPort(0),
   fDevice(dev),
   fTransportStatus(stCreated),
   fErrorState(false)
{
   gNumTransports++;
   
   if (GetDevice()) GetDevice()->TransportCreated(this);
}

dabc::Transport::~Transport()
{
   if (GetDevice()) GetDevice()->TransportDestroyed(this);

   gNumTransports--;
   
   DOUT5(("Transport::~Transport destroyed"));
}

void dabc::Transport::AssignPort(Port* port)
{
   // method must be called from the port (module) thread
    
   DOUT5((" Transport::AssignPort 1 new = %p old = %p locked = %s", port, fPort, DBOOL(fPortMutex.IsLocked())));

   bool changed = false;
   {
      LockGuard guard(fPortMutex);
      
      changed = (port!=fPort);
      
      fPort = port;
      
      if (changed) fTransportStatus = fPort ? stAssigned : stNeedCleanup;
   }

   DOUT5((" Transport::AssignPort 2 new = %p changed = %s", fPort, DBOOL(changed)));
   
   if (changed) PortChanged();

   if (!IsPortAssigned() && fDevice)
      fDevice->InitiateCleanup();
}

bool dabc::Transport::_IsReadyForCleanup()
{
   // check if one ready to cleanup transport means destroy it
   return (fTransportStatus==stNeedCleanup); 
}

bool dabc::Transport::CheckNeedCleanup(bool force)
{
   LockGuard guard(fPortMutex);
   
   if (fTransportStatus==stDoingCleanup) return false;
   
   if (!_IsReadyForCleanup() && !force) return false;
   
   fTransportStatus = stDoingCleanup;
   
   return true;
}

void dabc::Transport::ErrorCloseTransport()
{
   // method must be called, when error is detected and one wants to close(destroy) transport
   
   bool old = fErrorState;
   fErrorState = true;
   
   if (!old) DettachPort();
}


bool dabc::Transport::IsDoingCleanup()
{
   LockGuard guard(fPortMutex);
   
   return fTransportStatus == stDoingCleanup;
}

void dabc::Transport::DettachPort()
{
   // one cannot set fPort to 0 directly, while it means that 
   // transport is inactive and can be cleaned or even destroyed
   // Port must confirm cleanup and only than one can start real object deletion

   // first, try to set transport normally, via port thread
   {

      LockGuard guard(fPortMutex);

      DOUT5(("DettachPort start %p %s locked %s", this, DNAME(fPort), DBOOL(fPortMutex.IsLocked())));
      
      if (fPort!=0)
         if (fPort->AssignTransport(0)) return;
   }

   // if fPort is not set or not like to disconnect, force it
   AssignPort(0);
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

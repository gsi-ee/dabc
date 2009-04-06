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

#include "roc/UdpTransport.h"

#include "roc/UdpDevice.h"

roc::UdpDataSocket::UdpDataSocket(UdpDevice* dev, int fd) :
   dabc::SocketIOProcessor(fd),
   dabc::Transport(dev),
   fDev(dev),
   fQueueMutex(),
   fQueue(1),
   daqState(daqInit)
{
   // we will react on all input packets
   SetDoingInput(true);
}

roc::UdpDataSocket::~UdpDataSocket()
{
   if (fDev) fDev->fDataCh = 0;
   fDev = 0;
}

void roc::UdpDataSocket::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {
      case evntSendReq:
//         DOUT0(("Send packet of size %u", fDev->controlSendSize));
//         StartSend(&(fDev->controlSend), fDev->controlSendSize);
         break;
      case evntSocketRead: {
//         ssize_t len = DoRecvBuffer(&fRecvBuf, sizeof(fRecvBuf));
//         if ((len>0) && fDev)
//            fDev->processCtrlMessage(&fRecvBuf, len);
         break;
      }
      default:
         dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

bool roc::UdpDataSocket::Recv(dabc::Buffer* &buf)
{
   buf = 0;
   dabc::LockGuard lock(fQueueMutex);
   if (fQueue.Size()<=0) return false;
   buf = fQueue.Pop();
   return buf!=0;
}

unsigned  roc::UdpDataSocket::RecvQueueSize() const
{
   dabc::LockGuard guard(fQueueMutex);

   return fQueue.Size();
}

dabc::Buffer* roc::UdpDataSocket::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fQueueMutex);

   return fQueue.Item(indx);
}

void roc::UdpDataSocket::StartTransport()
{

}

void roc::UdpDataSocket::StopTransport()
{

}



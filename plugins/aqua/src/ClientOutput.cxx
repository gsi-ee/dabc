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

#include "aqua/ClientOutput.h"

#include "dabc/DataTransport.h"


void aqua::ClientOutput::OnThreadAssigned()
{
   // one could connect server already here??

   if (ConnectAquaServer()) fState = oReady;
}

void aqua::ClientOutput::OnSendCompleted()
{
   fState = oCompleteSend;
   MakeCallBack(dabc::do_Ok);
}

void aqua::ClientOutput::OnRecvCompleted()
{
   // method not used - socket only send data

   // StartRecv(fRecvBuf, 16);
}

void aqua::ClientOutput::OnConnectionClosed()
{
    if (fState == oSendingBuffer) MakeCallBack(dabc::do_Error);
    fState = oDisconnected;
}

void aqua::ClientOutput::OnSocketError(int errnum, const std::string& info)
{
   if (fState == oSendingBuffer) MakeCallBack(dabc::do_Error);
   fState = oError;
}

double aqua::ClientOutput::ProcessTimeout(double lastdiff)
{
   return -1;
}

aqua::ClientOutput::ClientOutput(dabc::Url& url) :
   dabc::SocketIOAddon(),
   dabc::DataOutput(url),
   fState(oDisconnected)
{
   fServerName = url.GetHostName();
   fServerPort = url.GetPort();
}


aqua::ClientOutput::~ClientOutput()
{
}

void aqua::ClientOutput::MakeCallBack(unsigned arg)
{
   dabc::OutputTransport* tr = dynamic_cast<dabc::OutputTransport*> (fWorker());

   if (tr==0) {
      EOUT("Didnot found OutputTransport on other side worker %p", fWorker());
      fState = oError;
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
   } else {
      tr->Write_CallBack(dabc::do_Ok);
   }

}


bool aqua::ClientOutput::ConnectAquaServer()
{
   CloseSocket();

   int fd = dabc::SocketThread::StartClient(fServerName.c_str(), fServerPort);
   if (fd < 0) return false;

   SetSocket(fd);

   // receiving not used in the transport
   // StartRecv(fRecvBuf, 16);

   return true;
}


bool aqua::ClientOutput::Write_Init()
{
   if (fState != oReady)
      if (ConnectAquaServer()) fState = oReady;

   return true;
}

unsigned aqua::ClientOutput::Write_Check()
{
   switch (fState) {
      case oDisconnected:        // when server not connected
      case oError:               // error state
         if (!ConnectAquaServer()) return dabc::do_RepeatTimeOut;
         fState = oReady;
         return dabc::do_Ok;     // state changed, can continue

      case oReady:                // ready to send next buffer
         return dabc::do_Ok;

      default:
         EOUT("Write_Check at wrong state %d", fState);
         return dabc::do_Error;
         break;
   }

   return dabc::do_RepeatTimeOut;
}

unsigned aqua::ClientOutput::Write_Buffer(dabc::Buffer& buf)
{
   if (fState != oReady) {
      EOUT("Write_Buffer at wrong state %d", fState);
      return dabc::do_Error;
   }

   fState = oSendingBuffer;

   strcpy(fSendHdr,"any info");
   StartNetSend(fSendHdr, sizeof(fSendHdr), buf);

   return dabc::do_CallBack;
}

unsigned aqua::ClientOutput::Write_Complete()
{
   if (fState == oCompleteSend) {
      fState = oReady;
      return dabc::do_Ok;
   }

   return dabc::do_Error;
}

double aqua::ClientOutput::Write_Timeout()
{
   switch (fState) {
      case oReady:
      case oSendingBuffer:
      case oCompleteSend: return 0.001;
      default: break;
   }

   return 1.;
}

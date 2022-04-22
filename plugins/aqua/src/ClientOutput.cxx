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

void aqua::ClientOutput::OnSocketError(int err, const std::string &info)
{
   if (fState == oSendingBuffer) MakeCallBack(dabc::do_Ok);
   DOUT1("Connection to AQUA broken  %s:%d - %d:%s", fServerName.c_str(), fServerPort, err, info.c_str());

   CancelIOOperations();

   fState = err!=0 ? oError : oDisconnected;
}

double aqua::ClientOutput::ProcessTimeout(double)
{
   return -1;
}

aqua::ClientOutput::ClientOutput(dabc::Url& url) :
   dabc::SocketIOAddon(),
   dabc::DataOutput(url),
   fLastConnect(),
   fState(oDisconnected),
   fBufCounter(0)
{
   fServerName = url.GetHostName();
   fServerPort = url.GetPort();

   fReconnectTmout = url.GetOptionInt("tmout", 3.);
}


aqua::ClientOutput::~ClientOutput()
{
   DOUT0("Destroy AQUA output");
}

void aqua::ClientOutput::MakeCallBack(unsigned /* arg */)
{
   dabc::OutputTransport* tr = dynamic_cast<dabc::OutputTransport*> (fWorker());

   if (!tr) {
      EOUT("Did not found OutputTransport on other side worker %s", fWorker.GetName());
      fState = oError;
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
   } else {
      tr->Write_CallBack(dabc::do_Ok);
   }

}


bool aqua::ClientOutput::ConnectAquaServer()
{
   CloseSocket();

   // do not try connection request too often
   if (!fLastConnect.Expired(fReconnectTmout)) return false;

   fLastConnect.GetNow();

   int fd = dabc::SocketThread::StartClient(fServerName, fServerPort);
   if (fd < 0) return false;

   SetSocket(fd);

   DOUT1("Connect AQUA server %s:%d", fServerName.c_str(), fServerPort);

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
         if (!ConnectAquaServer()) {
            fBufCounter++;
            return dabc::do_Skip;
         }
         fState = oReady;
         return dabc::do_Ok;     // state changed, can continue

      case oReady:                // ready to send next buffer
         return dabc::do_Ok;

      default:
         EOUT("Write_Check at wrong state %d", fState);
         return dabc::do_Error;
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

   fSendHdr.bufsize = buf.GetTotalSize();
   fSendHdr.counter = fBufCounter++;

   StartNetSend(&fSendHdr, sizeof(fSendHdr), buf);

   return dabc::do_CallBack;
}

unsigned aqua::ClientOutput::Write_Complete()
{
   if (fState == oCompleteSend) {
      fState = oReady;
      return dabc::do_Ok;
   }

   // this is not normal, but return OK to try start from beginning
   return dabc::do_Ok;
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

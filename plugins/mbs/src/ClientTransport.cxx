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

#include "mbs/ClientTransport.h"

#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/Pointer.h"
#include "dabc/DataTransport.h"


mbs::ClientTransport::ClientTransport(int fd, int kind) :
   dabc::SocketIOAddon(fd),
   dabc::DataInput(),
   fState(ioInit),
   fSwapping(false),
   fKind(kind),
   fPendingStart(false)
{
   fServInfo.iStreams = 0; // by default, new format

   DOUT3("Create mbs::ClientTransport::ClientTransport() %p fd:%d kind:%d", this, fd, kind);
}

mbs::ClientTransport::~ClientTransport()
{
   DOUT3("Destroy mbs::ClientTransport::~ClientTransport() %p", this);
}

void mbs::ClientTransport::ObjectCleanup()
{
   DOUT3("mbs::ClientTransport::ObjectCleanup");

   strcpy(fSendBuf, "CLOSE");
   DoSendBuffer(fSendBuf, 12);
   fState = ioClosing;

   dabc::SocketIOAddon::ObjectCleanup();
}


bool mbs::ClientTransport::IsDabcEnabledOnMbsSide()
{
   return fServInfo.iStreams==0;
}

unsigned mbs::ClientTransport::ReadBufferSize()
{
   uint32_t sz = fHeader.BufferLength();
   if (sz < sizeof(fHeader)) {
      EOUT("Wrong buffer length %u", sz);
      return 0;
   }

   return sz - sizeof(mbs::BufferHeader);
}


void mbs::ClientTransport::ProcessEvent(const dabc::EventId& evnt)
{
   dabc::SocketIOAddon::ProcessEvent(evnt);
}

void mbs::ClientTransport::OnSendCompleted()
{
}

void mbs::ClientTransport::OnRecvCompleted()
{
//   DOUT0("mbs::ClientTransport::OnRecvCompleted() state = %d", fState);

   switch (fState) {
      case ioRecvInfo:

         fState = ioReady;
         if (fServInfo.iEndian != 1) {
            mbs::SwapData(&fServInfo, sizeof(fServInfo));
            fSwapping = true;
         }

         if (fServInfo.iEndian != 1) {
            EOUT("Cannot correctly define server endian");
            fState = ioError;
         }

         if ((fState != ioError) && (fServInfo.iStreams != 0) && (fServInfo.iBuffers != 1)) {
            EOUT("Number of buffers %u per stream bigger than 1", fServInfo.iBuffers);
            EOUT("This will lead to event spanning which is not supported by DABC");
            EOUT("Set buffers number to 1 or call \"enable dabc\" on mbs side");
            fState = ioError;
         }

         if (fState!=ioError) {
            std::string info = "new format";
            if (fServInfo.iStreams > 0) dabc::formats(info, "streams = %u", fServInfo.iStreams);

            DOUT0("Get MBS server info: %s, buf_per_stream = %u, swap = %s ",
                  info.c_str(), fServInfo.iBuffers, DBOOL(fSwapping));
         }

         if (fPendingStart) {
            fPendingStart = false;
            SubmitRequest();
         }

         break;

      case ioRecvHeder:

         if (fSwapping) mbs::SwapData(&fHeader, sizeof(fHeader));

//         DOUT0("MbsClient:: Header received, size %u, rest size = %u used %u", fHeader.BufferLength(), ReadBufferSize(), fHeader.UsedBufferSize());

         if (ReadBufferSize() > (unsigned) fServInfo.iMaxBytes) {
            EOUT("Buffer size %u bigger than allowed by info record %d", ReadBufferSize(), fServInfo.iMaxBytes);
            fState = ioError;
            MakeCallback(dabc::di_Error);
         } else
         if (ReadBufferSize() == 0) {
            fState = ioReady;
            DOUT0("Keep alive buffer from MBS side");
            MakeCallback(dabc::di_SkipBuffer);
         } else {
            fState = ioWaitBuffer;
            MakeCallback(ReadBufferSize());
         }

         break;

      case ioRecvBuffer:

//         DOUT1("Provide recv buffer %p to transport", fRecvBuffer);

         if (fHeader.UsedBufferSize()>0) {
            fState = ioComplBuffer;
            MakeCallback(dabc::di_Ok);

         } else {
            if (IsDabcEnabledOnMbsSide()) {
               EOUT("Empty buffer from mbs when dabc enabled?");
               fState = ioError;
               MakeCallback(dabc::di_Error);
            } else {
               DOUT1("Keep alive buffer from MBS");
               fState = ioReady;
               MakeCallback(dabc::di_SkipBuffer);
            }
         }

         break;
      default:
         EOUT("One should not complete recv in such state %d", fState);
         return;
   }
}

void mbs::ClientTransport::OnConnectionClosed()
{
//   DOUT0("Close mbs client connection");

   // from this moment we have nothing to do and can close transport
   SubmitWorkerCmd(dabc::Command("CloseTransport"));

   dabc::SocketIOAddon::OnConnectionClosed();
}


void mbs::ClientTransport::OnThreadAssigned()
{
   dabc::SocketIOAddon::OnThreadAssigned();

   if (fState != ioInit) {
      EOUT("Get thread assigned in not-init state - check why");
      exit(234);
   }

   StartRecv(&fServInfo, sizeof(fServInfo));
   fState = ioRecvInfo;

   // check that server info is received in reasonable time
   ActivateTimeout(5.);

//   DOUT0("Try to recv server info at the begin");
}

double mbs::ClientTransport::ProcessTimeout(double last_diff)
{
   if (fState == ioRecvInfo) {
      EOUT("Did not get server info in reasonable time");
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
   }

   return -1;
}

void mbs::ClientTransport::SubmitRequest()
{
   if (Kind() == mbs::StreamServer) {
      strcpy(fSendBuf, "GETEVT");
      StartSend(fSendBuf, 12);
   }

   StartRecv(&fHeader, sizeof(fHeader));
   fState = ioRecvHeder;
}

void mbs::ClientTransport::MakeCallback(unsigned arg)
{
   dabc::InputTransport* tr = dynamic_cast<dabc::InputTransport*> (fWorker());

   if (tr==0) {
      EOUT("Didnot found DataInputTransport on other side worker %p", fWorker());
      fState = ioError;
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
   } else {
      // DOUT0("Activate CallBack with arg %u", arg);
      tr->Read_CallBack(arg);
   }
}


unsigned mbs::ClientTransport::Read_Size()
{
   switch (fState) {
      case ioRecvInfo:
         if (fPendingStart) EOUT("Start already pending???");
         fPendingStart = true;
         return dabc::di_CallBack;
      case ioReady:
         SubmitRequest();
         return dabc::di_CallBack;
      default:
         EOUT("Get read_size at wrong state %d", fState);
   }

   return dabc::di_Error;
}

unsigned mbs::ClientTransport::Read_Start(dabc::Buffer& buf)
{
//   DOUT0("mbs::ClientTransport::Read_Start");

   if (fState != ioWaitBuffer) {
      EOUT("Start reading at wrong place");
      return dabc::di_Error;
   }

   if (buf.GetTotalSize() <  ReadBufferSize()) {
      EOUT("Provided buffer size too small %u, required %u",
             buf.GetTotalSize(), ReadBufferSize());
      return dabc::di_Error;
   }

   DOUT5("MBS transport start recv %u", ReadBufferSize());

   StartRecv(buf, ReadBufferSize());
   fState = ioRecvBuffer;

   return dabc::di_CallBack;
}

unsigned mbs::ClientTransport::Read_Complete(dabc::Buffer& buf)
{
//   DOUT0("mbs::ClientTransport::Read_Complete");

   if (fState!=ioComplBuffer) {
      EOUT("Reading complete at strange place!!!");
      return dabc::di_Error;
   }

   buf.SetTypeId(mbs::mbt_MbsEvents);
   buf.SetTotalSize(fHeader.UsedBufferSize());
   if (fSwapping) {
      dabc::Pointer ptr(buf);
      while (!ptr.null()) {
         mbs::SwapData(ptr(), ptr.rawsize());
         ptr.shift(ptr.rawsize());
      }
   }

   fState = ioReady;

   return dabc::di_Ok;
}

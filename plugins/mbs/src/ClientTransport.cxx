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

#include "mbs/ClientTransport.h"

#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/Pointer.h"
#include "dabc/DataTransport.h"

#include "mbs/Iterator.h"

mbs::ClientTransport::ClientTransport(int fd, int kind) :
   dabc::SocketIOAddon(fd),
   dabc::DataInput(),
   fState(ioInit),
   fSwapping(false),
   fSpanning(false),
   fKind(kind),
   fPendingStart(false),
   fSpanBuffer()
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
   DOUT3("mbs::ClientTransport::ObjectCleanup\n");

   if ((fState!=ioError) && (fState!=ioClosed)) {
      strcpy(fSendBuf, "CLOSE");
      DoSendBuffer(fSendBuf, 12);
      fState = ioClosing;
   }

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
            DOUT0("Number of buffers %u per stream bigger than 1", fServInfo.iBuffers);
            DOUT0("This will lead to event spanning which is not optimal for DABC");
            DOUT0("Set buffers number to 1 or call 'enable dabc' on mbs side");
            fSpanning = true;
            // fState = ioError;
         }

         if (fState != ioError) {
            std::string info = "";
            if (fServInfo.iStreams > 0) dabc::formats(info, "streams = %u", fServInfo.iStreams);

            DOUT0("Get MBS server info: %s buf_per_stream = %u, swap = %s spanning %s",
                  info.c_str(), fServInfo.iBuffers, DBOOL(fSwapping), DBOOL(fSpanning));
         }

         if (fPendingStart) {
            fPendingStart = false;
            SubmitRequest();
         }

         break;

      case ioRecvHeader:

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

            // when spanning is used, we need normal-size buffer
            MakeCallback(fSpanning ? dabc::di_DfltBufSize : ReadBufferSize());
         }

         break;

      case ioRecvBuffer:

//         DOUT1("Provide recv buffer %p to transport", fRecvBuffer);

         // DOUT0("RECV BUFFER Used size %u readsize %u", fHeader.UsedBufferSize(), ReadBufferSize());

         if (fHeader.UsedBufferSize() > 0) {
            fState = ioComplBuffer;
            MakeCallback(dabc::di_Ok);

         } else
         if (IsDabcEnabledOnMbsSide()) {
            EOUT("Empty buffer from mbs when dabc enabled?");
            fState = ioError;
            MakeCallback(dabc::di_Error);
         } else {
            DOUT1("Keep alive buffer from MBS");
            fState = ioReady;
            MakeCallback(dabc::di_SkipBuffer);
         }

         break;
      default:
         EOUT("One should not complete recv in such state %d", fState);
         return;
   }
}

void mbs::ClientTransport::OnSocketError(int err, const std::string& info)
{
   if (err==0) {
      DOUT3("MBS client Socket close\n");

      fState = ioClosed;

      SubmitWorkerCmd(dabc::CmdDataInputClosed());

   } else {

      DOUT3("MBS client Socket Error\n");

      fState = ioError;

      SubmitWorkerCmd(dabc::CmdDataInputFailed());
   }
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
   fState = ioRecvHeader;
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

   DOUT4("BUFFER_START %u USED %u h_beg %u h_end %u", ReadBufferSize(), fHeader.UsedBufferSize(), fHeader.h_begin, fHeader.h_end);

   if (fState != ioWaitBuffer) {
      EOUT("Start reading at wrong place");
      return dabc::di_Error;
   }

   if (buf.GetTotalSize() <  ReadBufferSize()) {
      EOUT("Provided buffer size too small %u, required %u",
             buf.GetTotalSize(), ReadBufferSize());
      return dabc::di_Error;
   }

   bool started = false;

   if (!fSpanBuffer.null()) {

      if (fHeader.h_begin==0) {
         EOUT("We expecting spanned buffer in the begin, but didnot get it");
         return dabc::di_Error;
      }

      dabc::Buffer extra = buf.Duplicate();

      if (extra.GetTotalSize() < ReadBufferSize() + fSpanBuffer.GetTotalSize()) {
         EOUT("Buffer size %u not enough to read %u and add spanned buffer %u", extra.GetTotalSize(), ReadBufferSize(), fSpanBuffer.GetTotalSize());
         return dabc::di_Error;
      }

      // we keep place in main buffer, but header for additional peace will be cutted
      extra.CutFromBegin(fSpanBuffer.GetTotalSize() - sizeof(mbs::Header));

      // extra buffer required only here, later normal buffer can be used
      started = StartRecv(extra, ReadBufferSize());
   } else {
      started = StartRecv(buf, ReadBufferSize());
   }

   if (!started) return dabc::di_Error;

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

   unsigned read_shift = 0;
   if (!fSpanBuffer.null()) read_shift = fSpanBuffer.GetTotalSize() - sizeof(mbs::Header);


   // first of all, swap data where it was received
   if (fSwapping) {
      dabc::Pointer ptr(buf);
      if (read_shift>0) ptr.shift(read_shift);
      ptr.setfullsize(fHeader.UsedBufferSize());

      while (!ptr.null()) {
         mbs::SwapData(ptr(), ptr.rawsize());
         ptr.shift(ptr.rawsize());
      }
   }

   // now we should find how big is block in the beginning
   // and copy spanned buffer to the beginning
   if (!fSpanBuffer.null()) {
      dabc::Pointer ptr(buf);
      ptr.shift(read_shift);

      mbs::Header* hdr = (mbs::Header*) ptr();
      unsigned new_block_size = hdr->FullSize();

      buf.CopyFrom(fSpanBuffer);

      DOUT4("Copy block %u to begin", fSpanBuffer.GetTotalSize());

      hdr = (mbs::Header*) buf.SegmentPtr();

      hdr->SetFullSize(read_shift + new_block_size);

      fSpanBuffer.Release();
   }

   // in any case release extra buffers
   buf.SetTypeId(mbs::mbt_MbsEvents);
   buf.SetTotalSize(read_shift + fHeader.UsedBufferSize());

   if (fSpanning) {

      // if there is block at the end, keep copy for the next operation
      if (fHeader.h_end != 0) {

         mbs::ReadIterator iter(buf);

         unsigned useful_sz(0), last_sz(0);

         while (iter.NextEvent()) {
            useful_sz += last_sz;
            last_sz = iter.evnt()->FullSize();
         }

         fSpanBuffer = buf.Duplicate();
         if (fSpanBuffer.null()) {
            EOUT("FAIL to duplicate buffer!!!");
            return dabc::di_Error;
         }

         buf.SetTotalSize(useful_sz);

         // span buffer remained until next request
         fSpanBuffer.CutFromBegin(useful_sz);

         DOUT4("Left block %u from the end", fSpanBuffer.GetTotalSize());
      }
   }

   fState = ioReady;

   if (buf.GetTotalSize()==0) {
      DOUT0("EXTREME CASE - FULL BUFFER IS JUST PEACE FROM THE MIDDLE");
      return dabc::di_SkipBuffer;
   }

   return dabc::di_Ok;
}

#include "roc/Transport.h"
#include "roc/Device.h"

#include "dabc/Port.h"
#include "dabc/Buffer.h"
#include "dabc/Pointer.h"
#include "dabc/logging.h"

#include "SysCoreControl.h"

roc::Transport::Transport(roc::Device* dev, dabc::Port* port, int bufsize, const char* boardIp, int trWindow) :
   // provide input buffers, but no output since data flow always unidirectional here
   DataTransport(dev, port, true, false),
   fxBoard(0),
   fuBoardId(0),
   fBufferSize(bufsize),
   fTransWidnow(trWindow),
   fReqNumMsgs(0)
{
   DOUT1(("roc::Transport constructor"));

   port->AssignTransport(this);

   // create board object:
   int id = dev->AddBoard(boardIp, true);
   if(id<0)
      EOUT(("!!! ROCTransport: failed to add board %s", boardIp));
   else {
      fuBoardId = (unsigned) id;
      fxBoard = dev->GetBoard(fuBoardId);
   }

   if (fBufferSize<1512) {
      EOUT(("Buffer size must be at least 1512 to keep UDP buffer"));
      fBufferSize = 1512;
   }

   fIntegrityChecks = false;

   fLastEvent = 0;
   fIsLastEvent = false;
   fLastEventStep = 0;
   fLastEpoch = 0;

   DOUT5(("Transport constructor done"));

}

roc::Transport::~Transport()
{
   DOUT5(("roc::Transport destructor."));
}


void roc::Transport::StartTransport()
{
   if(fxBoard==0) {
       EOUT(("!!! ROCTransport: NEVER COME HERE: board %d is not existing", fuBoardId));
       // exception here to get sm failure?
       return;
   }

   DOUT1(("roc::Transport::StartTransport().., using board id:%d, hostport:%u",fuBoardId,fxBoard->getHostDataPort()));

   fxBoard->startDaq(fTransWidnow);

   fLastEvent = 0;
   fIsLastEvent = false;
   fLastEventStep = 0;
   fLastEpoch = 0;

   int needpackets = fBufferSize / (6*MESSAGES_PER_PACKET);
   if (needpackets > fTransWidnow / 2)
      needpackets = fTransWidnow / 2;
   fReqNumMsgs = needpackets * MESSAGES_PER_PACKET;

   dabc::DataTransport::StartTransport();
}


void roc::Transport::StopTransport()
{
   DOUT5(("roc::Transport::StopTransport()..."));
   fxBoard->stopDaq();
   dabc::DataTransport::StopTransport();
}

unsigned roc::Transport::Read_Size()
{
   return fBufferSize;
}

unsigned roc::Transport::Read_Start(dabc::Buffer* buf)
{
   int req = fxBoard->requestData(fReqNumMsgs);

   if (req==2) return dabc::di_CallBack;

   if (req==1) return dabc::di_Ok;

   return dabc::di_Error;
}

unsigned roc::Transport::Read_Complete(dabc::Buffer* buf)
{
//   DOUT1(("Brd:%u Read_Complete begin", fuBoardId));

   buf->SetTypeId(roc::rbt_RawRocData);

   unsigned fullsz = buf->GetDataSize();

   char* tgt = (char*) buf->GetDataLocation();

/*
   {
      SysCoreData abc;
      unsigned sz = 0;

      while ((sz <= fullsz-6) && (fxBoard->getNextData(abc, 0.))) {
         memcpy(tgt, abc.getData(), 6);
         tgt+=6;
         sz+=6;
      }

      if (sz>0) {
         fullsz = sz;
         buf->SetDataSize(fullsz);
         tgt = (char*) buf->GetDataLocation();
      } else {
         EOUT(("Problem to read something !!!"));
         fullsz = 0;
      }
   }
*/

   if (fxBoard->fillData(tgt, fullsz)) {
//      DOUT1(("Fill data size %u", fullsz));
      buf->SetDataSize(fullsz);
   } else {
      EOUT(("Roc:%u No way to fill data - data size %u", fuBoardId, fullsz));
      return dabc::di_SkipBuffer;
   }

//   DOUT1(("Read buffer of size %u", fullsz));

   while ((fullsz>=6) && fIntegrityChecks) {
      SysCoreData* data = (SysCoreData*) tgt;

      if (data->getMessageType() == ROC_MSG_SYNC) {
         if (fIsLastEvent) {
            if (fLastEvent > data->getSyncEvNum())
               EOUT(("Roc:%u Last event %06x bigger than next %06x, overflow?", fuBoardId, fLastEvent, data->getSyncEvNum()));
            else {
               unsigned step = data->getSyncEvNum() - fLastEvent;
               if ((step!=0) && (fLastEventStep!=0) && (fLastEventStep!=step))
                  EOUT(("Roc:%u Event steps are different %x %x ", fuBoardId, fLastEventStep, step));
               fLastEventStep = step;
            }
         }
        fLastEvent = data->getSyncEvNum();
        fIsLastEvent = true;
      } else

      if (data->getMessageType() == ROC_MSG_EPOCH) {

          if (fLastEpoch!=0)
             if (data->getEpoch() <= fLastEpoch)
               EOUT(("Is epoch sequence ok %x %x", fLastEpoch, data->getEpoch()));

          fLastEpoch = data->getEpoch();
      }

      tgt += 6;
      fullsz -= 6;
   }

//   DOUT5(("Stop DataSize = %u", dataptr.fullsize()));

   // in the end finish all mbs event structure

   if (buf->GetDataSize()==0) {
      EOUT(("No data was read !!!!!!"));
      return dabc::di_SkipBuffer;
   }

//   DOUT1(("Brd:%u Read_Complete done", fuBoardId));

   return dabc::di_Ok;
}

void roc::Transport::ComplteteBufferReading()
{
   if (fCurrentBuf==0) { EOUT(("AAAAAAAAA")); exit(1); }

   unsigned res = Read_Complete(fCurrentBuf);

   Read_CallBack(res);
}


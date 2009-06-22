#ifndef SYSCORESORTER_H_
#define SYSCORESORTER_H_

#include "nxyter/Data.h"

namespace nxyter {

class Sorter {
   protected:
      // only because of losses of epoch markers tmBoundary is expanded, normally should be about 0x4100

      enum TimesConstants { tmLastValid = 0x3fffffff, tmEmpty = tmLastValid + 100, tmFailure = tmLastValid + 200, tmBoundary = 0x8100, tmFrontShift = 0x10000000 };

      unsigned     fRocId;

      unsigned     fIntBufMaxSize;    // maximum number of hits in intern buffer
      unsigned     fExtBufMaxSize;    // maximum size of extern buffer by addData

      nxyter::Data *fIntBuf;
      uint32_t    *fIntBufTimes;
      unsigned     fIntBufSize;   // number of hits in internal buffer
      uint32_t     fIntBufFrontEpoch;  // basis epoch number of intrn buffers
      uint32_t     fIntBufCurrEpoch; // current epoch of intern buffer in special form
      uint32_t     fIntBufCurrRealEpoch; // current epoch of intern buffer in real form
      uint32_t     fIntBuffCurrMaxTm;  // current maxtm of intern buffer

      unsigned     fAccumMissed;    // accumulated missed hits from epochs to write at generated epoch
      uint32_t     fLastOutEpoch;   // value of last written epoch to output

      nxyter::Data* fFillBuffer;     // buffer to fill
      unsigned     fFillTotalSize;  // total size of external buffer in bytes
      unsigned     fFillSize;       // already filled size in messages

      void*        fInternOutBuf;     // internal output buffer, which can be used for output
      unsigned     fInternOutBufSize; // size of internal output buffer in messages

      bool         fDoDataCheck;
      bool         fDoDebugOutput;

      bool flushBuffer(nxyter::Data* data, unsigned num_msg, bool force = false);

   public:
      Sorter(unsigned maxinpbuf = 1024, unsigned intern_out_buf = 1024, unsigned intbuf = 128);
      virtual ~Sorter();

      bool isDataChecked() const { return fDoDataCheck; }
      void setDataCheck(bool on = true) { fDoDataCheck = on; }

      bool isDebugOutput() const { return fDoDebugOutput; }
      void setDebugOutput(bool on = true) { fDoDebugOutput = on; }

      // is data in internal buffer
      bool isInternData() const { return fIntBufSize > 0; }

      // Add raw nxyter::Data as they come from ROC
      // Normally no more than 243 messages should be added - size of UDP packet
      // Return true if buffer is already filled
      bool addData(nxyter::Data* data, unsigned num_msg, bool flush_data = false);

      bool flush() { return flushBuffer(0, 0, true); }

      // clean all messages from buffers, can continue filling
      void cleanBuffers();

      // number of messages, filled in output buffer
      unsigned sizeFilled() const { return fFillSize; }

      nxyter::Data* filledBuf() const { return fFillBuffer; }

      // skip num messages in output buffer
      bool shiftFilledData(unsigned num);

      // initialise variables to be able fill external buffer
      // return false, if buffer size is too small
      // if buf==0, internal buffer of specified size will be allocated
      bool startFill(void* buf, unsigned totalsize);

      // Stop filling of external buffer, next messages goes into internal (if exists)
      void stopFill();
};

}

#endif

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

#ifndef HADAQ_MbsTransmitterModule
#define HADAQ_MbsTransmitterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

#ifndef HADAQ_Iterator
#include "hadaq/Iterator.h"
#endif

namespace hadaq {


/** \brief Transfer HLD into LMD events
 *
 * This module will take hadaq format input and wrap it into a Mbs subevent for further analysis.
 * Use same formatting as in hadaq eventsource of go4.
 */

   class MbsTransmitterModule : public dabc::ModuleAsync {

   protected:

      hadaq::ReadIterator fSrcIter;   // iterator over input buffer
      mbs::WriteIterator  fTgtIter;   // iterator over output buffer

      uint32_t            fCurrentEventNumber; // current event number in the output buffer
      bool                isCurrentEventNumber; //! true if current event number is valid
      uint32_t            fIgnoreEvent;  // id of event, which should be ignored
      bool                isIgnoreEvent;  // true if event should be ignored
      int                 fEvCounter; // simple counter

      unsigned            fSubeventId;

      /* true if subsequent hadaq events with same sequence number should be merged
       * into one mbs subevent*/
      bool                fMergeSyncedEvents;

      int                 fFlushCnt;

      dabc::Buffer        feofbuf;    // keep eof buffer, it is mark that module must be stopped

      bool retransmit();

   public:
      MbsTransmitterModule(const std::string& name, dabc::Command cmd = 0);

      virtual bool ProcessBuffer(unsigned pool) { return retransmit(); }

      virtual bool ProcessRecv(unsigned port) { return retransmit(); }

      virtual bool ProcessSend(unsigned port) { return retransmit(); }

      virtual void ProcessTimerEvent(unsigned timer);

   };

}


#endif

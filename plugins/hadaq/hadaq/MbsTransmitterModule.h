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


namespace hadaq {


/*
 * This module will take hadaq format input and wrap it into a Mbs subevent for further analysis.
 * Use same formatting as in hadaq eventsource of go4.
 */


   class MbsTransmitterModule : public dabc::ModuleAsync {

   protected:

      unsigned int fSubeventId;

      /* true if subsequent hadaq events with same sequence number should be merged
       * into one mbs subevent*/
      bool fMergeSyncedEvents;

      /** When specified, SYNC number is printed */
      bool fPrintSync;

      double fFlushTimeout;

      void FlushBuffer(bool force = false);

      bool retransmit();

      void CloseCurrentEvent();

      dabc::Buffer       fTgtBuf;   // buffer for data
      dabc::Pointer      fHdrPtr;   // pointer on header
      dabc::Pointer      fDataPtr;   // pointer on next raw data
      int                fLastEventNumber; // last number of HADAQ event count
      dabc::TimeStamp    fLastFlushTime; // time when last flushing was done
      int                fIgnoreEvent;  // id of event, which should be ignored
      int                fCounter; // simple counter

   public:
      MbsTransmitterModule(const char* name, dabc::Command cmd = 0);

      virtual void ProcessInputEvent(dabc::Port*) { retransmit(); }

      virtual void ProcessOutputEvent(dabc::Port*) { retransmit(); }

      virtual void ProcessTimerEvent(dabc::Timer* timer);

   };

}


#endif

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

#ifndef MBS_MonitorSlowControl
#define MBS_MonitorSlowControl

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

#ifndef MBS_SlowControlData
#include "mbs/SlowControlData.h"
#endif

namespace mbs {

   /** \brief Base class for monitoring of slow control data
    *
    * It delivers collected slow-control data to output
    *
    **/

   class MonitorSlowControl : public dabc::ModuleAsync {
      protected:
         mbs::SlowControlData  fRec;             ///< record used to store selected variables
         bool                  fDoRec;           ///< when true, record should be filled
         unsigned              fSubeventId;      ///< full id number for subevent
         unsigned              fEventNumber;     ///< Event number, written to MBS event
         dabc::TimeStamp       fLastSendTime;    ///< last time when buffer was send, used for flushing
         mbs::WriteIterator    fIter;            ///< iterator for creating of MBS events
         double                fFlushTime;       ///< time to flush event

         void SendDataToOutputs();

         virtual unsigned GetRecRawSize() { return fRec.GetRawSize(); }
         virtual unsigned GetNextEventNumber() { return ++fEventNumber; }
         virtual unsigned GetNextEventTime();
         virtual unsigned WriteRecRawData(void* ptr, unsigned maxsize)
            { return fRec.Write(ptr, maxsize); }

      public:

         MonitorSlowControl(const std::string &name, const std::string &prefix, dabc::Command cmd = nullptr);
         virtual ~MonitorSlowControl();

         virtual void ProcessTimerEvent(unsigned timer);

   };
}


#endif

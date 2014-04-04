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

#ifndef FESA_Monitor
#define FESA_Monitor

#ifndef MBS_MonitorSlowControl
#include "mbs/MonitorSlowControl.h"
#endif

class rdaDeviceHandle;
class rdaRDAService;
class rdaDabcHandler;
class rdaData;

namespace fesa {

   /** \brief Monitor for of FESA data
    *
    * Module builds hierarchy for connected FESA classes,
    * which could be served via DABC web server in any browser
    * Optionally, one able to convert data into MBS events, which could be delivered to analysis
    *
    **/

   class Monitor : public mbs::MonitorSlowControl {
      protected:
         dabc::Hierarchy fHierarchy;

         std::string fServerName;    ///< FESA server name
         std::string fDeviceName;    ///< FESA device name
         std::string fCycle;         ///< cycle parameter

         rdaRDAService* fRDAService;
         rdaDeviceHandle* fDevice;
         std::vector<rdaDabcHandler*> fHandlers;

         mbs::SlowControlData  fRec;   ///< record used to store selected variables
         bool fDoRec;  ///< when true, record will be filled
         unsigned           fSubeventId;      ///< full id number for dim subevent
         long               fEventNumber;     ///< Event number, written to MBS event
         dabc::TimeStamp    fLastSendTime;    ///< last time when buffer was send, used for flushing
         mbs::WriteIterator fIter;            ///< iterator for creating of MBS events
         double             fFlushTime;       ///< time to flush event

         double doGet(const std::string& service, const std::string& field);

         virtual unsigned GetRecRawSize();
         virtual unsigned WriteRecRawData(void* ptr, unsigned maxsize);

      public:

         Monitor(const std::string& name, dabc::Command cmd = 0);
         virtual ~Monitor();

         void ReportServiceChanged(const std::string& name, const rdaData* v);

         void ReportServiceError(const std::string& name, const std::string& err);

   };
}


#endif

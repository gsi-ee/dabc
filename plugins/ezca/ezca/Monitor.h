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

#ifndef EZCA_Monitor
#define EZCA_Monitor

#ifndef MBS_MonitorSlowControl
#include "mbs/MonitorSlowControl.h"
#endif

namespace ezca {

   /** \brief Monitor of EPICS data
    *
    * Module builds hierarchy for connected FESA classes,
    * which could be served via DABC web server in any browser
    *
    * In addition, module can provide data with stored EPICS records in form of MBS events
    *
    **/

   class Monitor : public mbs::MonitorSlowControl {
      protected:

         double  fEzcaTimeout;    ///< Timeout for ezca readout
         int     fEzcaRetryCnt;   ///< Number of retry in ezca readout
         bool    fEzcaDebug;      ///< Switch on/off epics debug messages
         bool    fEzcaAutoError;  ///< Automatic error printing
         double  fTimeout;        ///< timeout for readout
         std::string fNameSepar;  ///< separator symbol(s), which defines subfolder in epcis names
         std::string fTopFolder;  ///< name of top folder, which should exists also in every variable

         std::vector<std::string> fLongRecords;   ///< names of long records
         std::vector<long> fLongValues;           ///< values of long records
         std::vector<bool> fLongRes;              ///< results of record readout

         std::vector<std::string> fDoubleRecords; ///< names of double records
         std::vector<double> fDoubleValues;       ///< values of double records
         std::vector<bool> fDoubleRes;            ///< results of record readout

         /** Initialize some EZCA settings, do it from worker thread */
         virtual void OnThreadAssigned();

         /* Wrapper for ezca get with long values. Contains error message handling.
          * Returns ezca error code.*/
         int CA_GetLong(const std::string& name, long& val);

         /* Wrapper for ezca get with double values. Contains error message handling
          * Returns ezca error code.*/
         int CA_GetDouble(const std::string& name, double& val);

         /** Return error string with error description */
         std::string CA_ErrorString();

         const char* CA_RetCode(int ret);

         /** Perform readout of all variables */
         bool DoEpicsReadout();

         virtual unsigned GetRecRawSize();

         std::string GetItemName(const std::string& ezcaname);

      public:
         Monitor(const std::string& name, dabc::Command cmd = 0);

         virtual void ProcessTimerEvent(unsigned timer);
   };
}


#endif

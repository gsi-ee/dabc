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

#ifndef EZCA_Player
#define EZCA_Player

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

namespace ezca {

   /** \brief Player of EPICS data
    *
    * Module builds hierarchy for connected FESA classes,
    * which could be served via DABC web server in any browser
    *
    * In addition, module can provide data with stored EPICS records in form of MBS events
    *
    **/

   class Player : public dabc::ModuleAsync {
      protected:

         double  fEzcaTimeout;    ///< Timeout for ezca readout
         int     fEzcaRetryCnt;   ///< Number of retry in ezca readout
         bool    fEzcaDebug;      ///< Switch on/off epics debug messages
         bool    fEzcaAutoError;  ///< Automatic error printing
         double  fTimeout;        ///< timeout (in seconds) for readout polling.
         unsigned fSubeventId;    ///< full id number for epics subevent

         /* contains names of all long integer values to be requested*/
         std::vector<std::string> fLongRecords;
         std::vector<long> fLongValues;

         /* contains names of all double  values to be requested*/
         std::vector<std::string> fDoubleRecords;
         std::vector<double> fDoubleValues;

         /** Complete descriptor of long/double variables, packed into mbs event */
         std::string fDescriptor;

         /** Event number, written to MBS event */
         long fEventNumber;

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

         /** Build descriptor of EPICS variables */
         void BuildDescriptor();

         /** Perform readout of all variables */
         bool DoEpicsReadout();

         void SendDataToOutputs();

         std::string GetItemName(const std::string& ezcaname);

      public:
         Player(const std::string& name, dabc::Command cmd = 0);
         virtual ~Player();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);
   };
}


#endif

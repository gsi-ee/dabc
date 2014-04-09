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

#ifndef MBS_Monitor
#define MBS_Monitor

#ifndef MBS_MonitorSlowControl
#include "mbs/MonitorSlowControl.h"
#endif

#ifndef DABC_CommandsQueue
#include "dabc/CommandsQueue.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif


namespace mbs {


   /** \brief Addon class to retrieve status record from MBS server
    *
    **/


   class DaqStatusAddon : public dabc::SocketIOAddon {

      protected:

         enum IOState {
            ioInit,           // initial state
            ioRecvHeader,     // receiving server info
            ioRecvData,
            ioDone,
            ioError
         };

         enum EEvents {
            evDataInput = evntSocketLast,
            evReactivate
         };

         IOState              fState;
         mbs::DaqStatus       fStatus;
         bool                 fSwapping;
         uint32_t             fSendCmd;

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         virtual void OnRecvCompleted();

      public:

         DaqStatusAddon(int fd);

         mbs::DaqStatus& GetStatus() { return fStatus; }
   };

   // =====================================================================

   class DaqLogWorker : public dabc::Worker {
      protected:

         struct MbsLogRec {
            int32_t iOrder;
            int32_t iType;
            int32_t iStatus;
            char fBuffer[512];
         };

         std::string fMbsNode;
         int         fPort;
         MbsLogRec   fRec;

         bool CreateAddon();

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         virtual void ProcessEvent(const dabc::EventId&);

      public:
         DaqLogWorker(const dabc::Reference& parent, const std::string& name, const std::string& mbsnode, int port);
         virtual ~DaqLogWorker();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

   };

   // ======================================================================

   /** Worker to handle connection with MBS remote command server.
    * Server can be started in MBS with command 'start cmdrem'
    */

   class DaqRemCmdWorker : public dabc::Worker {
      protected:

         struct MbsRemCmdRec {
            uint32_t  l_order;      /* byte order. Set to 1 by sender */
            uint32_t  l_cmdid;      /* command id, 0xffffffff when dummy command is send */
            uint32_t  l_status;     /* command completion status      */
            char      c_cmd[512];   /* command to execute - or reply info */

            bool CheckByteOrder() {
               if (l_order!=1) mbs::SwapData(this, 3*4);
               return l_order==1;
            }
         };


         enum IOState {
            ioInit,           // initial state
            ioWaitReply,      // waiting for command reply
            ioError
         };

         std::string       fMbsNode;
         int               fPort;

         dabc::CommandsQueue fCmds;

         IOState           fState;
         MbsRemCmdRec      fSendBuf;
         uint32_t          fSendCmdId;  // command identifier, send to the MBS with the command
         MbsRemCmdRec      fRecvBuf;

         virtual void ProcessEvent(const dabc::EventId&);

         virtual double ProcessTimeout(double last_diff);

         virtual int ExecuteCommand(dabc::Command cmd);

         void ProcessNextMbsCommand();

         virtual void OnThreadAssigned();

         bool CreateAddon();


      public:
         DaqRemCmdWorker(const dabc::Reference& parent, const std::string& name, const std::string& mbsnode, int port);

         virtual ~DaqRemCmdWorker();
   };


   // ======================================================================


   /** \brief Interface module for MBS monitoring and control
    *
    * Module could access MBS via three different control ports
    *
    * + status port 6008 (always)
    * + remote logger port 6007 (optional)
    * + remote command port 6019 (optional)
    *
    * Status record readout periodically with specified interval and
    * used to calculate different rate values.
    *
    * Details about module usage are described in
    * [web interface for MBS](\ref mbs_web_interface).
    *
    **/

   class Monitor : public mbs::MonitorSlowControl {
      protected:

         dabc::Hierarchy   fHierarchy;
         unsigned          fCounter;

         std::string       fMbsNode;    ///< name of MBS node to connect
         double            fPeriod;     ///< period how often status is requested
         int               fLoggerPort; ///< port, providing log information
         int               fCmdPort;    ///< port, providing remote command access
         mbs::DaqStatus    fStatus;     ///< current DAQ status
         dabc::TimeStamp   fStatStamp;  ///< time when last status was obtained
         bool              fPrintf;     ///< use printf for major debug output

         void FillStatistic(const std::string& options, const std::string& itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double difftime);

         virtual void OnThreadAssigned();

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void ProcessTimerEvent(unsigned timer);

         virtual unsigned WriteRecRawData(void* ptr, unsigned maxsize);

      public:

         Monitor(const std::string& name, dabc::Command cmd = 0);
         virtual ~Monitor();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

         std::string MbsNodeName() const { return fMbsNode; }

         /** Called by LogWorker to inform about new message */
         void NewMessage(const std::string& msg);

         /** Called by CmdWorker to inform about new command send (or getting reply) */
         void NewSendCommand(const std::string& cmd, int res = -1);

         /** Called by DaqStatusAddon to inform about new daq status */
         void NewStatus(mbs::DaqStatus& stat);
   };
}


#endif

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

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif


namespace mbs {


   /** \brief Addon class to retrieve status record from MBS server  **/

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
         DaqLogWorker(const dabc::Reference& parent, const std::string &name, const std::string &mbsnode, int port);
         virtual ~DaqLogWorker();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

   };

   // ======================================================================

   /** Worker to handle connection with MBS remote command server.
    * Server can be started in MBS with command 'start cmdrem' */

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
         DaqRemCmdWorker(const dabc::Reference& parent, const std::string &name, const std::string &mbsnode, int port);

         virtual ~DaqRemCmdWorker();
   };


   // ======================================================================

   /** Worker to handle connection with MBS prompter.
    * Prompter should be started with command: rpm -r client_host_name */

   class PrompterWorker : public dabc::Worker {
      protected:

         enum IOState {
            ioInit,           // initial state
            ioWaitReply,      // waiting for command reply
            ioError
         };

         std::string       fMbsNode;    // real mbs node name
         int               fPort;
         std::string       fPrefix;     // prefix used to identify client

         dabc::CommandsQueue fCmds;

         IOState           fState;
         char              fSendBuf[256];
         uint32_t          fRecvBuf[2];

         virtual void ProcessEvent(const dabc::EventId&);

         virtual double ProcessTimeout(double last_diff);

         virtual int ExecuteCommand(dabc::Command cmd);

         void ProcessNextMbsCommand();

         virtual void OnThreadAssigned();

         bool CreateAddon();

      public:
         PrompterWorker(const dabc::Reference& parent, const std::string &name, const std::string &mbsnode, int port);

         virtual ~PrompterWorker();
   };

   // ======================================================================


   /** \brief Interface module for MBS monitoring and control
    *
    * Module could access MBS via three different control ports
    *
    * + status port 6008 (optional)
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
         std::string       fAlias;      // name appeared in hierarchy, node name by default
         double            fPeriod;     ///< period how often status is requested in seconds
         double            fRateInterval; ///< period for measuring the rate averages in seconds
         int               fHistory; ///< length of parameter history buffer
         int               fStatPort;   ///< port, providing stat information (normally 6008)
         int               fLoggerPort; ///< port, providing log information
         int               fCmdPort;    ///< port, providing remote command access
         mbs::DaqStatus    fStatus;     ///< current DAQ status
         dabc::TimeStamp   fStatStamp;  ///< time when last status was obtained
         bool              fPrintf;     ///< use printf for major debug output
         std::string       fFileStateName;    ///< name of filestate parameter
         std::string       fAcqStateName;    ///< name of acquisition running parameter
         std::string       fSetupStateName;    ///< name of "daq setup is loaded" parameter
         std::string       fRateIntervalName;    ///< name of rate sample interval parameter
         std::string       fHistoryName;    ///< name of variable history parameter

         void FillStatistic(const std::string &options, const std::string &itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double difftime);

         void CreateCommandWorker();

         virtual void OnThreadAssigned();

         virtual int ExecuteCommand(dabc::Command cmd);

         /** update file on/off state*/
         void UpdateFileState(int isopen);
         /** update mbs acq running state*/
         void UpdateMbsState(int isrunning);

         /** update mbs setup loaded state*/
         void UpdateSetupState(int isloaded);

         /** set sampling interval for rates calculation*/
         void SetRateInterval(double t);

         /** set depth of variable history buffer*/
         void SetHistoryDepth(int entries);

         bool IsPrompter() const { return fCmdPort == 6006; }

         virtual void ProcessTimerEvent(unsigned timer);

         virtual unsigned WriteRecRawData(void* ptr, unsigned maxsize);

      public:

         Monitor(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~Monitor();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

         std::string MbsNodeName() const { return fMbsNode; }

         /** Called by LogWorker to inform that connection is established */
         void LoggerAddonCreated();

         /** Called by LogWorker to inform about new message */
         void NewMessage(const std::string &msg);

         /** Called by CmdWorker to inform about new command send (or getting reply) */
         void NewSendCommand(const std::string &cmd, int res = -1);

         /** Called by DaqStatusAddon to inform about new daq status */
         void NewStatus(mbs::DaqStatus& stat);
   };
}


#endif

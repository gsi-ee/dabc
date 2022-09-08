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

         IOState              fState{ioInit};
         mbs::DaqStatus       fStatus;
         bool                 fSwapping{false};
         uint32_t             fSendCmd{0};

         void OnThreadAssigned() override;

         double ProcessTimeout(double last_diff) override;

         void OnRecvCompleted() override;

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
         int         fPort{0};
         MbsLogRec   fRec;
         bool        fFirstRecv{false};

         bool CreateAddon();

         void OnThreadAssigned() override;

         double ProcessTimeout(double last_diff) override;

         void ProcessEvent(const dabc::EventId&) override;

      public:
         DaqLogWorker(const dabc::Reference& parent, const std::string &name, const std::string &mbsnode, int port);
         virtual ~DaqLogWorker();

         std::string RequiredThrdClass() const override
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
               if (l_order != 1) mbs::SwapData(this, 3*4);
               return l_order == 1;
            }
         };


         enum IOState {
            ioInit,           // initial state
            ioWaitReply,      // waiting for command reply
            ioError
         };

         std::string       fMbsNode;
         int               fPort{0};

         dabc::CommandsQueue fCmds;

         IOState           fState{ioInit};
         MbsRemCmdRec      fSendBuf;
         uint32_t          fSendCmdId{0};  // command identifier, send to the MBS with the command
         MbsRemCmdRec      fRecvBuf;

         void ProcessEvent(const dabc::EventId&) override;

         double ProcessTimeout(double last_diff) override;

         int ExecuteCommand(dabc::Command cmd) override;

         void ProcessNextMbsCommand();

         void OnThreadAssigned() override;

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
         int               fPort{0};
         std::string       fPrefix;     // prefix used to identify client

         dabc::CommandsQueue fCmds;

         IOState           fState{ioInit};
         char              fSendBuf[256];
         uint32_t          fRecvBuf[2];

         void ProcessEvent(const dabc::EventId&) override;

         double ProcessTimeout(double last_diff) override;

         int ExecuteCommand(dabc::Command cmd) override;

         void ProcessNextMbsCommand();

         void OnThreadAssigned() override;

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
         unsigned          fCounter{0};

         std::string       fMbsNode;    ///< name of MBS node to connect
         std::string       fAlias;      // name appeared in hierarchy, node name by default
         double            fPeriod{0};     ///< period how often status is requested in seconds
         double            fRateInterval{0}; ///< period for measuring the rate averages in seconds
         int               fHistory{0}; ///< length of parameter history buffer
         int               fStatPort{0};   ///< port, providing stat information (normally 6008)
         int               fLoggerPort{0}; ///< port, providing log information
         int               fCmdPort{0};    ///< port, providing remote command access
         bool              fWaitingLogger{false}; ///< waiting logger to create cmd port
         dabc::Command     fWaitingCmd;    ///< command waiting when logger channel is ready
         mbs::DaqStatus    fStatus;     ///< current DAQ status
         dabc::TimeStamp   fStatStamp;  ///< time when last status was obtained
         bool              fPrintf{false};     ///< use printf for major debug output
         std::string       fFileStateName;    ///< name of filestate parameter
         std::string       fAcqStateName;    ///< name of acquisition running parameter
         std::string       fSetupStateName;    ///< name of "daq setup is loaded" parameter
         std::string       fRateIntervalName;    ///< name of rate sample interval parameter
         std::string       fHistoryName;    ///< name of variable history parameter

         void FillStatistic(const std::string &options, const std::string &itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double difftime);

         void CreateCommandWorker();

         void OnThreadAssigned() override;

         int ExecuteCommand(dabc::Command cmd) override;

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

         void ProcessTimerEvent(unsigned timer) override;

         unsigned WriteRecRawData(void* ptr, unsigned maxsize) override;

      public:

         Monitor(const std::string &name, dabc::Command cmd = nullptr);
         virtual ~Monitor();

         std::string RequiredThrdClass() const override
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

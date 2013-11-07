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

#ifndef MBS_Player
#define MBS_Player

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
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
         MbsLogRec  fRec;

         bool CreateAddon();

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         virtual void ProcessEvent(const dabc::EventId&);

      public:
         DaqLogWorker(const dabc::Reference& parent, const std::string& name, const std::string& mbsnode);
         virtual ~DaqLogWorker();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

   };

   // ========================================================================


   class DaqCmdWorker : public dabc::Worker {
      protected:

         enum IOState {
            ioInit,           // initial state
            ioRecvHeader,     // receiving header
            ioRecvExtra,      // receiving extra data
            ioDone,           // operation completed
            ioError
         };

         std::string       fMbsNode;
         std::string       fPrompter;

         dabc::CommandsQueue fCmds;

         IOState           fState;
         char              fSendBuf[256];
         char              fRecvBuf[8];
         int               fExtraBlock;
         dabc::Buffer      fExtraBuf;

         int GetStatus() const { return *((int32_t*) (fRecvBuf+4)); }

         int GetEndian() const { return *((int32_t*) (void*) fRecvBuf); }

         virtual void ProcessEvent(const dabc::EventId&);

         virtual double ProcessTimeout(double last_diff);

         virtual int ExecuteCommand(dabc::Command cmd);

         void ProcessNextMbsCommand();


      public:
         DaqCmdWorker(const dabc::Reference& parent, const std::string& name, const std::string& mbsnode, const std::string& prompter);

         virtual ~DaqCmdWorker();

   };


   // ======================================================================


   /** \brief Interface module for MBS monitoring and control
    *
    * Module could access MBS via three different control ports
    *
    * + status port 6008 (always)
    * + logger port 6007 (optional)
    * + prompter port 6006 (optional)
    *
    * Status record readout periodically with specified interval and
    * used to calculate different rate values.
    *
    * Details about module usage are described in
    * [web interface for MBS](\ref mbs_web_interface).
    *
    **/

   class Player : public dabc::ModuleAsync {
      protected:

         dabc::Hierarchy   fHierarchy;
         unsigned          fCounter;

         std::string       fMbsNode;
         double            fPeriod;

         /** If not empty, command connection will be established
          * Same argument should be specified when starting prompter in MBS like
          * prm -r myvalue
          */
         std::string       fPrompter;

         bool              fWithLogger; ///< if true, logger will be tried to created

         mbs::DaqStatus    fStatus;

         dabc::TimeStamp   fStatStamp; ///< time when last status was obtained

         void FillStatistic(const std::string& options, const std::string& itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double difftime);

         virtual void OnThreadAssigned();

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void ProcessTimerEvent(unsigned timer);


      public:

         Player(const std::string& name, dabc::Command cmd = 0);
         virtual ~Player();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

         std::string MbsNodeName() const { return fMbsNode; }

         void NewMessage(const std::string& msg);

         void NewStatus(mbs::DaqStatus& stat);
   };
}


#endif

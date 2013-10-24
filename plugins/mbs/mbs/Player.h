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

   // ======================================================================


   class DaqCommandAddon : public dabc::SocketIOAddon {
      protected:
         enum IOState {
            ioInit,           // initial state
            ioRecvHeader,     // receiving header
            ioRecvExtra,      // receiving extra data
            ioDone,           // operation completed
            ioError
         };

         IOState fState;          ///< if true, addon is active processing current command
         char fSendBuf[256];
         char fRecvBuf[8];
         int  fExtraBlock;

         dabc::Buffer fExtraBuf;

         virtual void OnThreadAssigned();

         virtual void OnRecvCompleted();

         virtual void OnConnectionClosed()
         {
            dabc::SocketIOAddon::OnConnectionClosed();
            EOUT("OnConnectionClosed");
         }
         virtual void OnSocketError(int errnum, const std::string& info)
         {
            dabc::SocketIOAddon::OnSocketError(errnum, info);
            EOUT("OnSocketError");
         }


         virtual double ProcessTimeout(double last_diff);

      public:
         DaqCommandAddon(int fd);

         bool AssignCmd(const std::string& prompter, const std::string& cmd, int extrablock = 0);

         bool FinishCmd();

         bool IsActive() const { return (fState==ioRecvHeader) || (fState==ioRecvExtra); }
         bool IsResultOk() const { return (fState==ioDone); }
         bool IsError() const { return (fState==ioError); }

         int GetEndian() const;
         int GetStatus() const;

   };




   // ======================================================================


   /** \brief Player of MBS data
    *
    * Module represents MBS data,
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

         mbs::DaqStatus    fStatus;

         dabc::TimeStamp   fStamp;

         dabc::SocketAddonRef fCmdAddon;

         dabc::CommandsQueue fCmds;

         void FillStatistic(const std::string& options, const std::string& itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double difftime);

         void ProcessNextMbsCommand();

      public:

         Player(const std::string& name, dabc::Command cmd = 0);
         virtual ~Player();

         virtual std::string RequiredThrdClass() const
           {  return dabc::typeSocketThread; }

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

   };
}


#endif

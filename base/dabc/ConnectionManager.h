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

#ifndef DABC_ConnectionManager
#define DABC_ConnectionManager

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_ConnectionRequest
#include "dabc/ConnectionRequest.h"
#endif

#ifndef DABC_ReferencesVector
#include "dabc/ReferencesVector.h"
#endif


namespace dabc {

   /** \brief Full description of connection request
    *
    * \ingroup dabc_all_classes
    *
    */

   class ConnectionRequestFull : public ConnectionRequest {

       DABC_REFERENCE(ConnectionRequestFull, ConnectionRequest, ConnectionObject)

       // these states are representing connection state during connection establishing

       void SetUseAcknDirectly(bool on) { SetAllowedField(xmlUseacknAttr); SetUseAckn(on); }

       void SetConnTimeoutDirectly(double tm) { SetAllowedField(xmlTimeoutAttr); SetConnTimeout(tm); }

       std::string GetServerId() const { GET_PAR_FIELD(fServerId,"") }
       void SetServerId(const std::string& id) { SET_PAR_FIELD(fServerId, id) }

       std::string GetClientId() const { GET_PAR_FIELD(fClientId,"") }
       void SetClientId(const std::string& id) { SET_PAR_FIELD(fClientId, id) }

       int GetInlineDataSize() const { GET_PAR_FIELD(fInlineDataSize,0) }
       void SetInlineDataSize(int size) { SET_PAR_FIELD(fInlineDataSize, size) }

       Reference TakeCustomData() { GET_PAR_FIELD(fCustomData,0) }
       void SetCustomData(Reference ref) { SET_PAR_FIELD(fCustomData, ref) }

       std::string GetConnId() const { GET_PAR_FIELD(fConnId,"") }
       void SetConnId(const std::string& id) { SET_PAR_FIELD(fConnId, id) }

       int progress() const { GET_PAR_FIELD(fProgress,0) }
       void SetProgress(int pr) { SET_PAR_FIELD(fProgress, pr) }

       void SetRemoteCommand(dabc::Command cmd) { SET_PAR_FIELD(fRemoteCmd, cmd) }
       void ReplyRemoteCommand(bool res)
       {
          Command cmd;
          if (GetObject()) {
             LockGuard lock(GetObject()->ObjectMutex());
             cmd = GetObject()->fRemoteCmd;
             GetObject()->fRemoteCmd.Release();
          }
          cmd.ReplyBool(res);
       }

       bool match(const std::string localurl, const std::string remoteurl)
       {
          return (localurl == GetLocalUrl()) && (remoteurl == GetRemoteUrl());
       }

       std::string GetConnInfo()
       {
          std::string res = GetLocalUrl();
          res += " <-> ";
          res += GetRemoteUrl();
          return res;
       }

       void ResetConnData()
       {
          SetProgress(0);
          SetConnTimeout(10.);

          // FIXME: should we protect fields here with mutex when clear ???
          SetServerId("");
          SetClientId("");
          TakeCustomData().Release();
          SetConnId("");

          ReplyRemoteCommand(false);

          if (GetObject())
             GetObject()->SetDelay(0, true);
       }
   };

   class ConnectionManagerHandleCmd : public dabc::Command {

      DABC_COMMAND(ConnectionManagerHandleCmd, "ConnectionManagerCmd")

         static const char* ReqArg() { return "#REQ"; }

         ConnectionManagerHandleCmd(ConnectionRequestFull& req);

         std::string GetReq() const { return GetStdStr(ReqArg()); }
   };


   class GlobalConnectCmd : public Command {

      DABC_COMMAND(GlobalConnectCmd, "GlobalConnect")

      void SetUrl1(const std::string& url1) { SetStr("Url1", url1); }
      void SetUrl2(const std::string& url2) { SetStr("Url2", url2); }

      std::string GetUrl1() const { return GetStdStr("Url1"); }
      std::string GetUrl2() const { return GetStdStr("Url2"); }
   };

   // ____________________________________________________________________________________

   /** \brief Connections manager class
    *
    * \ingroup dabc_all_classes
    *
    */

   class ConnectionManager : public ModuleAsync {
      protected:
         ReferencesVector   fRecs;

         Command            fConnCmd;          ///< actual global command for connections establishing/closing
         int                fDoingConnection;  ///< flag indicates that about connection activity: 0 - nothing, -1 - closing, 1 - establishing

         /** \brief Analyze reply of the command, send to the device */
         void HandleConnectRequestCmdReply(ConnectionManagerHandleCmd cmd);

         /** \brief React on the reply of global connect command */
         void HandleGlobalConnectCmdReply(GlobalConnectCmd cmd);

         /** \brief Fill answer on remote connection request and invoke device to start connection
          * When device confirms that connection starts, command will be replied  */
         bool FillAnswerOnRemoteConnectCmd(Command cmd, ConnectionRequestFull& req);

         /** \brief Check current situation with connections.
          *
            If all of them ready or some fails, connection command will be completed.
            Parameter finish_command indicate if current connection command should be finished due to timeout
            In case of timeout command can be finished successfully if connection is optional */
         void CheckConnectionRecs(bool finish_command = false);

         /** \brief Destroy all connections, if necessary - request to cleanup custom data by device */
         virtual void ObjectCleanup();

         /** \brief Process changes in connection recs */
         virtual void ProcessParameterEvent(const ParameterEvent& evnt);

         virtual int ExecuteCommand(Command cmd);
         virtual bool ReplyCommand(Command cmd);

         /** \brief Check status of connections establishing. Return interval for next timeout */
         virtual double ProcessTimeout(double last_diff);

      public:

         enum EConnProgress {
            progrInit,         ///< initial state
            progrDoingInit,    ///< state when record should be prepared by device
            progrPending,      ///< state when record is prepared and can try connect
            progrWaitReply,    ///< connection request is send to the remote side and waiting for reply
            progrDoingConnect, ///< at this state device should drive connection itself and inform about completion or failure
            progrConnected,    ///< this is normal state when connection is active
            progrFailed        ///< fail state, request will be ignored forever
         };

         ConnectionManager(const std::string& name, Command cmd = 0);
         virtual ~ConnectionManager();

         ConnectionRequestFull FindConnection(const std::string& url1, const std::string& url2);
   };

}


#endif

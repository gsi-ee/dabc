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

#ifndef DABC_ConnectionRequest
#define DABC_ConnectionRequest

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_ConfigBase
#include "dabc/ConfigBase.h"
#endif

namespace dabc {

   class Device;
   class Port;
   class Module;
   class ConnectionManager;
   class ConnectionRequest;
   class ConnectionRequestFull;


   /** \brief Container for connection parameters
    *
    *  \ingroup dabc_all_classes
    *
    * Connection object configures properties of connection,
    * which should be established for the port.
    * One can create the only connection object for each port.
    * Connection object is used by connection manager to establish connection with remote nodes.
    * Connection object derived from parameter class and issues events, which can be catched by any
    * worker in dabc. Such features used by connection manager to discover any changes in the connection and
    * recover connection (if it is allowed by the port).
    */

   class ConnectionObject : public ParameterContainer {
      public:

         enum EState {
            sInit,          ///< connection in initial state
            sPending,       ///< connection is pending  (want to be connected)
            sConnecting,    ///< connection is in progress
            sConnected,     ///< connection is up and working
            sDisconnected,  ///< connection is down by user, will not be reconnected
            sBroken,        ///< connection is broken , if allowed connection manager will reactivate connection immediately
            sFailed         ///< connection cannot be established by connection manager
         };

      protected:

         friend class Port;
         friend class ConnectionManager;
         friend class ConnectionRequest;
         friend class ConnectionRequestFull;

         EState        fConnState;    ///< actual state of the connection, is parameter value

         std::string   fLocalUrl;     ///< full url of local port

         std::string   fPoolName;     ///< pool which should be used for the connections

         // these are fields used only by connection manager and devices from their threads
         // connection manager for a time of connection keep reference on the connection
         // object and use these fields for establishing of connection
         // these fields used by conn mgr and devices, therefore protected by mutex

         int          fProgress;

         std::string  fServerId;     ///< identifier for server side
         std::string  fClientId;     ///< identifier for client side
         Reference    fCustomData;   ///< implementation specific data, managed by the device, cleanuped by the record when connection is destroyed
         std::string  fConnId;       ///< unique id for that connection, defined by the server (can be overwritten by device)
         Command      fRemoteCmd;    ///< command from remote, interpretation depends from the state
         int          fInlineDataSize;   ///< property of the port, if does not match, will be ruled by the server

         // these fields and methods used only by connection manager
         // therefore it is enough to have reference on it (no mutex required)
         double      fAccDelay;    ///< accounting spent time, when negative - record is active
         double      fSetDelay;    ///< set value for delay, will be activated first time timeout processing done

         std::string  fAllowedField;  ///< name of field which is allowed to change in any state


         ConnectionObject(Reference port, const std::string& localurl);
         virtual ~ConnectionObject();

         /** Reimplement virtual method of ParameterContainer to block field change if object in non-init state.
          * In some situation change is allowed via fAllowedField  */
         virtual bool _CanChangeField(const std::string&);

         virtual const std::string DefaultFiledName() const;

         /** Change state of connection object.
          * State change can be observed by connection manager and
          * it may start connection establishing. Only accessible by connection manager*/
         void ChangeState(EState state, bool force);

         ConnectionObject::EState GetState();

         /** Set delay how long record is inactive.
          * Timeout will be checked in ProcessTimeout routine. If SetDelay is called from this routine,
          * one should specify force=true to reset time counter directly while time is accounted between
          * two calls of ProcessTimeout method.
          */
         void SetDelay(double v, bool force = false)
         {
            if (force) {
               fSetDelay = 0.;
               fAccDelay = v;
            } else {
               if (v>0) fSetDelay = v;
                   else fSetDelay = 0.;
               fAccDelay = 0.;
            }
         }

         /** \brief Function accounts time spent and returns true, if delay is not yet finished */
         double CheckDelay(double shift)
         {
            if (fSetDelay > 0.) {
               fAccDelay = fSetDelay;
               fSetDelay = 0;
               return fAccDelay;
            }

            fAccDelay -= shift;
            return fAccDelay;
         }

      public:
         virtual const char* ClassName() const { return "ConnectionObject"; }

         static const char* ObjectName() { return "Connection"; }

         static const char* GetStateName(EState state);
   };


#define GET_PAR_FIELD( field_name, defvalue ) \
    if (GetObject()) { \
       LockGuard guard(GetObject()->fObjectMutex); \
       return GetObject()-> field_name; \
    } \
    return defvalue;

#define SET_PAR_FIELD( field_name, value ) \
  if (GetObject()) { \
     LockGuard guard(GetObject()->fObjectMutex); \
     GetObject()-> field_name = value; \
   }


   // _______________________________________________________________________________

   /** \brief Connection request
    *
    * \ingroup dabc_all_classes
    *
    * ConnectionRequest is reference on \ref ConnectionObject,
    * which is used to specify user-specific parameters.
    * Not all fields of ConnectionObject accessible via ConnectionRequest.
    */

   class ConnectionRequest : public Parameter {

      friend class Port;
      friend class Module;
      friend class ConnectionManager;

      DABC_REFERENCE(ConnectionRequest, Parameter, ConnectionObject)

      /** When reference released, it is checked if we are in the init state and change object to pending state
       * It is done to inform connection manger that connection is ready for work  */
      virtual ~ConnectionRequest() { ChangeState(ConnectionObject::sPending, false); }

      ConnectionObject::EState GetState() const { return GetObject() ? GetObject()->GetState() : ConnectionObject::sInit; }

      Reference GetPort() const { return Reference(GetObject() ? GetObject()->GetParent() : 0); }

      std::string GetLocalUrl() const { GET_PAR_FIELD(fLocalUrl,"") }

      std::string GetPoolName() const { GET_PAR_FIELD(fPoolName,"") }
      void SetPoolName(const std::string& name) { SET_PAR_FIELD(fPoolName, name) }

      /** Return url of data source to which connection should be established */
      std::string GetRemoteUrl() const { return Field("url").AsStdStr(); }

      /** Indicates if local node in connection is server or client */
      bool IsServerSide() const { return Field("server").AsBool(false); }

      /** indicate if connection is optional and therefore may be ignored during failure or long timeout */
      bool IsOptional() const { return Field(xmlOptionalAttr).AsBool(false); }

      /** Device name which may be used to create connection (depends from url) */
      std::string GetConnDevice() const { return Field(xmlDeviceAttr).AsStdStr(); }

      /** Thread name for transport */
      std::string GetConnThread() const { return Field(xmlThreadAttr).AsStdStr(); }

      /** Use of acknowledge in protocol */
      bool GetUseAckn() const { return Field(xmlUseacknAttr).AsBool(false); }

      /** time required to establish connection, if expired connection will be switched to "failed" state */
      double GetConnTimeout() const { return Field(xmlTimeoutAttr).AsDouble(10.); }

      // fields can be changed only in initial state right after creation of the request
      void SetRemoteUrl(const std::string& url) { Field("url").SetStr(url); }

      void SetServerSide(bool isserver = true) { Field("server").SetBool(isserver); }

      void SetConnDevice(const std::string& dev) { Field(xmlDeviceAttr).SetStr(dev); }

      void SetOptional(bool on = true) { Field(xmlOptionalAttr).SetBool(on); }

      void SetUseAckn(bool on = true) { Field(xmlUseacknAttr).SetBool(on); }

      void SetConnTimeout(double tm) { Field(xmlTimeoutAttr).SetDouble(tm); }

      void SetConnThread(const std::string& name) { Field(xmlThreadAttr).SetStr(name); }

      void SetConfigFromXml(XMLNodePointer_t node);

      protected:

      /** Change state of the connection to init that other parameters can be changed */
      void ChangeState(ConnectionObject::EState state, bool force);

      void SetInitState() { ChangeState(ConnectionObject::sInit, true); }

      void SetAllowedField(const std::string& name = "");

      /** Indicates that only connection kind can be assigned to the reference */
      virtual const char* ParReferenceKind() { return "connection"; }

   };

}


#endif



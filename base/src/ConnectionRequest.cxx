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

#include "dabc/ConnectionRequest.h"


dabc::ConnectionObject::ConnectionObject(Reference port, const std::string &localurl) :
   ParameterContainer(port, ObjectName(), "connection", true),
   fConnState(sInit), // specify non-init state to be able change it
   fLocalUrl(localurl),
   fProgress(0),
   fServerId(),
   fClientId(),
   fCustomData(),
   fConnId(),
   fRemoteCmd(),
   fInlineDataSize(0),
   fAccDelay(0.),
   fSetDelay(0.),
   fAllowedField()
{
   SetSynchron(true, 0.);
}

dabc::ConnectionObject::~ConnectionObject()
{
}

bool dabc::ConnectionObject::_CanChangeField(const std::string &name)
{
   // special case when only field can be changed once
   if (!fAllowedField.empty() && (fAllowedField==name)) {
      fAllowedField.clear();
      return true;
   }

   if ((name == "state") && (fAllowedField=="%%state%%")) {
      fAllowedField.clear();
      return true;
   }

   // changes of parameter fields (used by the configuration) only in init state are allowed
   if (fConnState == sInit) return true;

   return false;
}

std::string dabc::ConnectionObject::DefaultFiledName() const
{
   return "state";
}

const char* dabc::ConnectionObject::GetStateName(EState state)
{
   switch (state) {
      case sInit: return "Init";
      case sPending: return "Pending";
      case sConnecting: return "Connecting";
      case sConnected: return "Connected";
      case sDisconnected: return "Disconnected";
      case sBroken: return "Broken";
      case sFailed: return "Failed";
   }
   return "---";
}

dabc::ConnectionObject::EState dabc::ConnectionObject::GetState()
{
   LockGuard lock(ObjectMutex());
   return fConnState;
}

void dabc::ConnectionObject::ChangeState(EState state, bool force)
{
   bool signal(false);

   {
      LockGuard lock(ObjectMutex());
      if (fConnState==state) return;

      // if state transition is not forced by caller, first check if previous state is corresponds
      if (!force) {
         if ((state==sPending) && (fConnState!=sInit)) return;
      }

      fConnState = state;

      signal = (state!=sInit);

      if (signal) fAllowedField = "%%state%%";
   }

   DOUT3("Change connection state to %s", GetStateName(state));

   if (signal)
      SetField("", GetStateName(state));
}

// ------------------------------------------------------------------

void dabc::ConnectionRequest::ChangeState(ConnectionObject::EState state, bool force)
{
   if (GetObject()) GetObject()->ChangeState(state, force);
}


void dabc::ConnectionRequest::SetAllowedField(const std::string &name)
{
   LockGuard lock(ObjectMutex());

   if (GetObject()) GetObject()->fAllowedField = name;
}


void dabc::ConnectionRequest::SetConfigFromXml(XMLNodePointer_t node)
{
   if ((node==0) || null()) return;

   const char* thrdname = Xml::GetAttr(node, xmlThreadAttr);
   if (thrdname!=0) {
      SetAllowedField(xmlThreadAttr);
      SetConnThread(thrdname);
   }

   const char* useackn = Xml::GetAttr(node, xmlUseacknAttr);
   if (useackn!=0) {
      SetAllowedField(xmlUseacknAttr);
      SetUseAckn(strcmp(useackn, xmlTrueValue)==0);
   }

   const char* isserver = Xml::GetAttr(node, "server");
   if (isserver!=0) {
      SetAllowedField("server");
      SetServerSide(strcmp(isserver, xmlTrueValue)==0);
   }

   const char* optional = Xml::GetAttr(node, xmlOptionalAttr);
   if (optional!=0) {
      SetAllowedField(xmlOptionalAttr);
      SetOptional(strcmp(optional, xmlTrueValue)==0);
   }

   const char* poolname = Xml::GetAttr(node, xmlPoolAttr);
   if (poolname!=0) SetPoolName(poolname);

   const char* devname = Xml::GetAttr(node, xmlDeviceAttr);
   if (devname!=0) {
      SetAllowedField(xmlDeviceAttr);
      SetConnDevice(devname);
   }

   const char* tmout = Xml::GetAttr(node, xmlTimeoutAttr);
   double tmout_val(10.);
   if ((tmout!=0) && str_to_double(tmout, &tmout_val)) {
      SetAllowedField(xmlTimeoutAttr);
      SetConnTimeout(tmout_val);
   }
}

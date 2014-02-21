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

#include "hadaq/HadaqTypeDefs.h"

const char* hadaq::typeUdpDevice = "hadaq::UdpDevice";

const char* hadaq::protocolHld = "hld";
const char* hadaq::protocolHadaq = "hadaq";

// Command names used in combiner module:
//const char* hadaq::comStartServer       = "StartServer";
//const char* hadaq::comStopServer        = "StopServer";
//const char* hadaq::comStartFile         = "StartFile";
//const char* hadaq::comStopFile          = "StopFile";


// tag names for xml config file:
const char* hadaq::xmlBuildFullEvent = "HadaqBuildEvents";

//const char* hadaq::xmlServerKind        = "HadaqServerKind";

//const char* hadaq::xmlServerScale       = "HadaqServerScale";
//const char* hadaq::xmlServerLimit       = "HadaqServerLimit";

const char* hadaq::xmlObserverEnabled      = "DoShmControl";
const char* hadaq::xmlExternalRunid       = "HadaqEPICSControl";
const char* hadaq::xmlMbsSubeventId       = "SubeventFullId";
const char* hadaq::xmlMbsMergeSyncMode    = "DoMergeSyncedEvents";

const char* hadaq::xmlSyncSeqNumberEnabled ="UseSyncSequenceNumber";
const char* hadaq::xmlSyncSubeventId       = "SyncSubeventId";
const char* hadaq::xmlSyncAcceptedTriggerMask       = "SyncTriggerMask";

const char* hadaq::NetmemPrefix     = "Netmem";
const char* hadaq::EvtbuildPrefix   = "Evtbuild";

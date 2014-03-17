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

#ifndef HADAQ_HadaqTypeDefs
#define HADAQ_HadaqTypeDefs

#include "hadaq/defines.h"

namespace hadaq {

   enum EHadaqBufferTypes {
      mbt_HadaqEvents = 142,        // event/subevent structure
      mbt_HadaqTransportUnit = 143  //plain hadtu container with single subevents
  };

   extern const char* typeUdpDevice;

   extern const char* protocolHld;
   extern const char* protocolHadaq;

   extern const char* xmlBuildFullEvent;

   extern const char* xmlObserverEnabled;
   extern const char* xmlExternalRunid;
   extern const char* xmlMbsSubeventId;
   extern const char* xmlMbsMergeSyncMode;

   extern const char* xmlSyncSeqNumberEnabled;
   extern const char* xmlSyncSubeventId;
   extern const char* xmlSyncAcceptedTriggerMask;
   extern const char* xmlHadaqTrignumRange;

   extern const char* NetmemPrefix;
   extern const char* EvtbuildPrefix;
}

#endif

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
      mbt_HadaqEvents = 142,         // event/subevent structure
      mbt_HadaqTransportUnit = 143,  //plain hadtu container with single subevents
      mbt_HadaqSubevents = 144,      // only subevents, no event
      mbt_HadaqStopRun = 145         // buffer just indicates stop of current run
  };

   extern const char* typeUdpDevice;

   extern const char* protocolHld;
   extern const char* protocolHadaq;

   extern const char* xmlBuildFullEvent;

   extern const char* xmlObserverEnabled;
   extern const char* xmlExternalRunid;

   extern const char* xmlHadaqTrignumRange;
   extern const char* xmlHadaqTriggerTollerance;
   extern const char* xmlHadaqDiffEventStats;
   extern const char* xmlEvtbuildTimeout;
   extern const char* xmlHadesTriggerType;
   extern const char* xmlHadesTriggerHUB;

   extern const char* NetmemPrefix;
   extern const char* EvtbuildPrefix;

   /* helper function to evaluate core affinity for process pid_t
    * mostly stolen from daqdata/hadaq/stats.c
      we put it here since event structures are known almost everywhere*/
   extern int CoreAffinity(pid_t pid);

   /*
    * Format a HADES-convention filename string
    * from a given run id and optional eventbuilder id
    */
   extern std::string FormatFilename (uint32_t runid, uint16_t ebid = 0);

   extern uint32_t CreateRunId();

}

#endif

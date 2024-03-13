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

#ifndef HADAQ_defines
#include "hadaq/defines.h"
#endif

namespace hadaq {

   enum EHadaqBufferTypes {
      mbt_HadaqEvents = 142,         // event/subevent structure
      mbt_HadaqTransportUnit = 143,  // plain hadtu container with single subevents
      mbt_HadaqSubevents = 144,      // only subevents, no event
      mbt_HadaqStopRun = 145         // buffer just indicates stop of current run
  };

   extern const char* protocolHld;

   extern const char* xmlBuildFullEvent;

   extern const char* xmlHadaqTrignumRange;
   extern const char* xmlHadaqTriggerTollerance;
   extern const char* xmlHadaqDiffEventStats;
   extern const char* xmlEvtbuildTimeout;
   extern const char* xmlMaxNumBuildEvt;
   extern const char* xmlHadesTriggerType;
   extern const char* xmlHadesTriggerHUB;

   /*
    * Format a HADES-convention filename string
    * from a given run id and optional eventbuilder id
    */
   extern std::string FormatFilename (uint32_t runid, uint16_t ebid = 0);

   extern uint32_t CreateRunId();

}

#endif

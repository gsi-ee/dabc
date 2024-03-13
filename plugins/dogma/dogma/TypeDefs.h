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

#ifndef DOGMA_TypeDefs
#define DOGMA_TypeDefs

#ifndef DOGMA_defines
#include "dogma/defines.h"
#endif

namespace dogma {

   enum EHadaqBufferTypes {
      mbt_DogmaEvents = 152,         // event/subevent structure
      mbt_DogmaTransportUnit = 153,  // plain container with single subevents
      mbt_DogmaSubevents = 154,      // only subevents, no event
      mbt_DogmaStopRun = 155         // buffer just indicates stop of current run
  };

   extern const char* protocolDogma;

   extern const char* xmlBuildFullEvent;

   extern const char* xmlHadaqTrignumRange;
   extern const char* xmlHadaqTriggerTollerance;
   extern const char* xmlHadaqDiffEventStats;
   extern const char* xmlEvtbuildTimeout;
   extern const char* xmlMaxNumBuildEvt;
   extern const char* xmlHadesTriggerType;
   extern const char* xmlHadesTriggerHUB;
}

#endif

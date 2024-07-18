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

#include <cstdio>
#include <sys/time.h>
#include <ctime>


const char* hadaq::protocolHld = "hld";

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

const char* hadaq::xmlHadaqTrignumRange   = "TriggerNumRange";
const char* hadaq::xmlHadaqTriggerTollerance = "TriggerTollerance";
const char* hadaq::xmlHadaqDiffEventStats = "AccountLostEventDiff";
const char* hadaq::xmlEvtbuildTimeout     = "BuildDropTimeout";
const char* hadaq::xmlMaxNumBuildEvt      = "MaxNumBuildEvt";
const char* hadaq::xmlHadesTriggerType    = "HadesTriggerType";
const char* hadaq::xmlHadesTriggerHUB     = "HadesTriggerHUB";

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

const char* hadaq::xmlHadaqTrignumRange   = "TriggerNumRange";
const char* hadaq::xmlHadaqTriggerTollerance = "TriggerTollerance";
const char* hadaq::xmlHadaqDiffEventStats = "AccountLostEventDiff";
const char* hadaq::xmlEvtbuildTimeout     = "BuildDropTimeout";
const char* hadaq::xmlMaxNumBuildEvt      = "MaxNumBuildEvt";
const char* hadaq::xmlHadesTriggerType    = "HadesTriggerType";
const char* hadaq::xmlHadesTriggerHUB     = "HadesTriggerHUB";


std::string hadaq::FormatFilename (uint32_t runid, uint16_t ebid)
{
   // same
   char buf[128];
   time_t iocTime = runid + HADAQ_TIMEOFFSET;
   struct tm tm_res;
   size_t off = strftime(buf, 128, "%y%j%H%M%S", localtime_r(&iocTime, &tm_res));
   if(ebid != 0) snprintf(buf+off, 128-off, "%02d", ebid);
   return std::string(buf);
}

uint32_t hadaq::CreateRunId()
{
   struct timeval tv;
   gettimeofday(&tv, nullptr);
   return tv.tv_sec - hadaq::HADAQ_TIMEOFFSET;
}

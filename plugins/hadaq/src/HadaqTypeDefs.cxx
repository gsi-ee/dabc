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

#include <string.h>
#include <byteswap.h>
#include <sys/time.h>
#include <time.h>

const char* hadaq::typeHldInput = "hadaq::HldInput";
const char* hadaq::typeHldOutput = "hadaq::HldOutput";
const char* hadaq::typeUdpDevice = "hadaq::UdpDevice";
const char* hadaq::typeUdpInput = "hadaq::UdpInput";

const char* hadaq::protocolHld = "hld";
const char* hadaq::protocolHadaq = "hadaq";

// Command names used in combiner module:
//const char* hadaq::comStartServer       = "StartServer";
//const char* hadaq::comStopServer        = "StopServer";
//const char* hadaq::comStartFile         = "StartFile";
//const char* hadaq::comStopFile          = "StopFile";

// port names and name formats:
const char* hadaq::portOutput           = "Output";
const char* hadaq::portOutputFmt        = "Output%d";
const char* hadaq::portInput            = "Input";
const char* hadaq::portInputFmt         = "Input%d";
const char* hadaq::portFileOutput       = "FileOutput";
const char* hadaq::portServerOutput     = "ServerOutput";

// tag names for xml config file:
const char* hadaq::xmlFileName = "HadaqFileName";
const char* hadaq::xmlSizeLimit = "HadaqFileSizeLimit";
const char* hadaq::xmlUdpAddress = "HadaqUdpSender";
const char* hadaq::xmlUdpPort = "HadaqUdpPort";
const char* hadaq::xmlUdpBuffer = "HadaqUdpBuffer";
const char* hadaq::xmlMTUsize = "HadaqUdpMTU";
const char* hadaq::xmlBuildFullEvent = "HadaqBuildEvents";

//const char* hadaq::xmlServerKind        = "HadaqServerKind";

//const char* hadaq::xmlServerScale       = "HadaqServerScale";
//const char* hadaq::xmlServerLimit       = "HadaqServerLimit";
const char* hadaq::xmlNormalOutput      = "DoOutput";
const char* hadaq::xmlFileOutput        = "DoFile";
const char* hadaq::xmlServerOutput      = "DoServer";
const char* hadaq::xmlObserverEnabled      = "DoShmControl";
const char* hadaq::xmlExternalRunid       = "HadaqEPICSControl";
const char* hadaq::xmlMbsSubeventId       = "SubeventFullId";

const char* hadaq::NetmemPrefix     = "Netmem";
const char* hadaq::EvtbuildPrefix   = "Evtbuild";

hadaq::Event* hadaq::Event::PutEventHeader(char** buf, EvtId id)
{
   hadaq::Event* ev = (hadaq::Event*) *buf;
   *buf += sizeof(hadaq::Event);
   ev->tuDecoding = EvtDecoding_64bitAligned;
   ev->SetId(id);
   ev->SetSize(sizeof(hadaq::Event));
   // timestamp at creation of structure:
   time_t tempo = time(NULL);
   struct tm* gmTime = gmtime(&tempo);
   uint32_t date = 0, clock = 0;
   date |= gmTime->tm_year << 16;
   date |= gmTime->tm_mon << 8;
   date |= gmTime->tm_mday;
   ev->SetDate(date);
   clock |= gmTime->tm_hour << 16;
   clock |= gmTime->tm_min << 8;
   clock |= gmTime->tm_sec;
   ev->SetTime(clock);
   return ev;
}

hadaq::RunId  hadaq::Event::CreateRunId()
{
   hadaq::RunId runNr;
   struct timeval tv;
   gettimeofday(&tv, NULL);
   runNr = tv.tv_sec - HADAQ_TIMEOFFSET;
   return runNr;
}


std::string hadaq::Event::FormatFilename (hadaq::RunId id)
{
   // same
   char buf[128];
   time_t iocTime;
   iocTime = id + HADAQ_TIMEOFFSET;
   strftime(buf, 18, "%y%j%H%M%S", localtime(&iocTime));
   return std::string(buf);
}


void  hadaq::Event::Init(EventNumType evnt, RunId run, EvtId evid)
{
   char* my=(char*) (this);
   hadaq::Event::PutEventHeader(&my, evid);
   evtSeqNr = evnt;
   evtRunNr = run;
   evtPad = 0;
}


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


const char* hadaq::typeHldInput          = "hadaq::HldInput";
const char* hadaq::typeHldOutput         = "hadaq::HldOutput";
const char* hadaq::typeUdpDevice = "hadaq::UdpDevice";
const char* hadaq::typeUdpInput  = "hadaq::UdpInput";

const char* hadaq::protocolHld          = "hld";
const char* hadaq::protocolHadaq          = "hadaq";

// Command names used in combiner module:
//const char* hadaq::comStartServer       = "StartServer";
//const char* hadaq::comStopServer        = "StopServer";
//const char* hadaq::comStartFile         = "StartFile";
//const char* hadaq::comStopFile          = "StopFile";

// port names and name formats:
//const char* hadaq::portOutput           = "Output";
//const char* hadaq::portOutputFmt        = "Output%d";
//const char* hadaq::portInput            = "Input";
//const char* hadaq::portInputFmt         = "Input%d";
//const char* hadaq::portFileOutput       = "FileOutput";
//const char* hadaq::portServerOutput     = "ServerOutput";


// tag names for xml config file:
const char* hadaq::xmlFileName          = "HadaqFileName";
const char* hadaq::xmlSizeLimit         = "HadaqFileSizeLimit";
const char* hadaq::xmlUdpAddress        = "HadaqUdpSender";
const char* hadaq::xmlUdpPort           = "HadaqUdpPort";

//const char* hadaq::xmlServerKind        = "HadaqServerKind";

//const char* hadaq::xmlServerScale       = "HadaqServerScale";
//const char* hadaq::xmlServerLimit       = "HadaqServerLimit";
//const char* hadaq::xmlNormalOutput      = "DoOutput";
//const char* hadaq::xmlFileOutput        = "DoFile";
//const char* hadaq::xmlServerOutput      = "DoServer";






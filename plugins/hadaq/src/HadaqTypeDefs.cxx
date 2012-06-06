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




//unsigned hadaq::EventHeader::NumSubevents() const
//{
//   unsigned cnt = 0;
//   SubeventHeader* sub = 0;
//   while ((sub = NextSubEvent(sub)) != 0) cnt++;
//   return cnt;
//}
//
//void hadaq::BufferHeader::Init(bool newformat)
//{
//   iWords = 0;
//   iType = newformat ? MBS_TYPE(100,1) : MBS_TYPE(10,1);
//   iBufferId = 0;
//   iNumEvents = 0;
//   iTemp = 0;
//   iSeconds = 0;
//   iNanosec = 0;
//   iEndian = 0;
//   iLast = 0;
//   iUsedWords = 0;
//   iFree3 = 0;
//   SetEndian();
//}
//
//uint32_t hadaq::BufferHeader::BufferLength() const
//{
//   switch (iType) {
//      // new buffer type
//      case MBS_TYPE(100,1): return sizeof(BufferHeader) + iUsedWords * 2;
//
//      // old buffer type
//      case MBS_TYPE(10,1): return sizeof(BufferHeader) + iWords * 2;
//   }
//
//   return 0;
//
//}
//
//uint32_t hadaq::BufferHeader::UsedBufferSize() const
//{
//   switch (iType) {
//      // new buffer type
//      case MBS_TYPE(100,1): return iUsedWords * 2;
//
//      // old buffer type
//      case MBS_TYPE(10,1): return i_used * 2;
//
//      default: break;
//      //         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
//   }
//
//   return 0;
//}
//
//void hadaq::BufferHeader::SetUsedBufferSize(uint32_t len)
//{
//   switch (iType) {
//      // new buffer type
//      case MBS_TYPE(100,1):
//               iUsedWords = len / 2;
//      if (iWords==0) SetFullSize(len + sizeof(BufferHeader));
//
//      break;
//
//      // old buffer type
//      case MBS_TYPE(10,1):
//               i_used = len / 2;
//      if (iWords==0) SetFullSize(len + sizeof(BufferHeader));
//      break;
//
//      default:
//         //         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
//         break;
//   }
//}
//
//void hadaq::SwapData(void* data, unsigned bytessize)
//{
//   if (data==0) return;
//   unsigned cnt = bytessize / 4;
//   uint32_t* d = (uint32_t*) data;
//
//   while (cnt-- != 0) {
//      *d = bswap_32(*d);
//      d++;
//   }
//}
//
//const char* hadaq::ServerKindToStr(int kind)
//{
//   switch (kind) {
//      case TransportServer: return "Transport";
//      case StreamServer: return "Stream";
//      case OldTransportServer: return "OldTransport";
//      case OldStreamServer: return "OldStream";
//   }
//   return "NoServer";
//}
//
//int hadaq::DefualtServerPort(int kind)
//{
//   switch (kind) {
//      case TransportServer: return 6000;
//      case StreamServer: return 6002;
//      case OldTransportServer: return 6000;
//      case OldStreamServer: return 6002;
//   }
//   return 0;
//}
//
//int hadaq::StrToServerKind(const char* str)
//{
//   if (str==0) return NoServer;
//   if (strcmp(str, "Transport") == 0) return TransportServer;
//   if (strcmp(str, "Stream") == 0) return StreamServer;
//   if (strcmp(str, "OldTransport") == 0) return OldTransportServer;
//   if (strcmp(str, "OldStream") == 0) return OldStreamServer;
//   return NoServer;
//}



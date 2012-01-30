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

#include "MbsTypeDefs.h"

#include <string.h>
#include <byteswap.h>


const char* mbs::typeLmdInput         = "mbs::LmdInput";
const char* mbs::typeLmdOutput        = "mbs::LmdOutput";
const char* mbs::typeTextInput        = "mbs::TextInput";
const char* mbs::typeServerTransport  = "mbs::ServerTransport";
const char* mbs::typeClientTransport  = "mbs::ClientTransport";


const char* mbs::protocolLmd          = "lmd";
const char* mbs::protocolMbs          = "mbs";
const char* mbs::protocolMbsTransport = "mbst";
const char* mbs::protocolMbsStream    = "mbss";

// Command names used in combiner module:
const char* mbs::comStartServer       = "StartServer";
const char* mbs::comStopServer        = "StopServer";
const char* mbs::comStartFile         = "StartFile";
const char* mbs::comStopFile          = "StopFile";

// port names and name formats:
const char* mbs::portOutput           = "Output";
const char* mbs::portOutputFmt        = "Output%d";
const char* mbs::portInput            = "Input";
const char* mbs::portInputFmt         = "Input%d";
const char* mbs::portFileOutput       = "FileOutput";
const char* mbs::portServerOutput     = "ServerOutput";


// tag names for xml config file:
const char* mbs::xmlFileName          = "MbsFileName";
const char* mbs::xmlSizeLimit         = "MbsFileSizeLimit";
const char* mbs::xmlServerName        = "MbsServerName";
const char* mbs::xmlServerKind        = "MbsServerKind";
const char* mbs::xmlServerPort        = "MbsServerPort";
const char* mbs::xmlServerScale       = "MbsServerScale";
const char* mbs::xmlServerLimit       = "MbsServerLimit";
const char* mbs::xmlNormalOutput      = "DoOutput";
const char* mbs::xmlFileOutput        = "DoFile";
const char* mbs::xmlServerOutput      = "DoServer";

const char* mbs::xmlTextDataFormat    = "MbsTextFormat";
const char* mbs::xmlTextNumData       = "MbsTextNumData";
const char* mbs::xmlTextHeaderLines   = "MbsTextHeader";
const char* mbs::xmlTextCharBuffer    = "MbsCharBuffer";


const char* mbs::xmlCombineCompleteOnly   = "BuildCompleteEvents";
const char* mbs::xmlCheckSubeventIds      = "CheckSubIds";
const char* mbs::xmlEvidMask              = "EventIdMask";
const char* mbs::xmlEvidTolerance         = "MaxDeltaEventId";
const char* mbs::xmlSpecialTriggerLimit   = "SpecialTriggerLimit";
const char* mbs::xmlCombinerRatesPrefix   = "CombinerRatesPrefix";

unsigned mbs::EventHeader::NumSubevents() const
{
   unsigned cnt = 0;
   SubeventHeader* sub = 0;
   while ((sub = NextSubEvent(sub)) != 0) cnt++;
   return cnt;
}

void mbs::BufferHeader::Init(bool newformat)
{
   iWords = 0;
   iType = newformat ? MBS_TYPE(100,1) : MBS_TYPE(10,1);
   iBufferId = 0;
   iNumEvents = 0;
   iTemp = 0;
   iSeconds = 0;
   iNanosec = 0;
   iEndian = 0;
   iLast = 0;
   iUsedWords = 0;
   iFree3 = 0;
   SetEndian();
}

uint32_t mbs::BufferHeader::BufferLength() const
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE(100,1): return sizeof(BufferHeader) + iUsedWords * 2;

      // old buffer type
      case MBS_TYPE(10,1): return sizeof(BufferHeader) + iWords * 2;
   }

   return 0;

}

uint32_t mbs::BufferHeader::UsedBufferSize() const
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE(100,1): return iUsedWords * 2;

      // old buffer type
      case MBS_TYPE(10,1): return i_used * 2;

      default: break;
      //         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
   }

   return 0;
}

void mbs::BufferHeader::SetUsedBufferSize(uint32_t len)
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE(100,1):
               iUsedWords = len / 2;
      if (iWords==0) SetFullSize(len + sizeof(BufferHeader));

      break;

      // old buffer type
      case MBS_TYPE(10,1):
               i_used = len / 2;
      if (iWords==0) SetFullSize(len + sizeof(BufferHeader));
      break;

      default:
         //         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
         break;
   }
}

void mbs::SwapData(void* data, unsigned bytessize)
{
   if (data==0) return;
   unsigned cnt = bytessize / 4;
   uint32_t* d = (uint32_t*) data;

   while (cnt-- != 0) {
      *d = bswap_32(*d);
      d++;
   }
}

const char* mbs::ServerKindToStr(int kind)
{
   switch (kind) {
      case TransportServer: return "Transport";
      case StreamServer: return "Stream";
      case OldTransportServer: return "OldTransport";
      case OldStreamServer: return "OldStream";
   }
   return "NoServer";
}

int mbs::DefualtServerPort(int kind)
{
   switch (kind) {
      case TransportServer: return 6000;
      case StreamServer: return 6002;
      case OldTransportServer: return 6000;
      case OldStreamServer: return 6002;
   }
   return 0;
}

int mbs::StrToServerKind(const char* str)
{
   if (str==0) return NoServer;
   if (strcmp(str, "Transport") == 0) return TransportServer;
   if (strcmp(str, "Stream") == 0) return StreamServer;
   if (strcmp(str, "OldTransport") == 0) return OldTransportServer;
   if (strcmp(str, "OldStream") == 0) return OldStreamServer;
   return NoServer;
}



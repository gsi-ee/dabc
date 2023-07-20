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

#include "mbs/MbsTypeDefs.h"

#include <cstring>
#include <cstdio>

#if defined(__APPLE__)
// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#else
#include <byteswap.h>
#endif

const char* mbs::protocolLmd          = "lmd";
const char* mbs::protocolMbs          = "mbs";
const char* mbs::protocolMbss         = "mbss";
const char* mbs::protocolMbst         = "mbst";

// Command names used in combiner module:
const char* mbs::comStartServer       = "StartServer";
const char* mbs::comStopServer        = "StopServer";
const char* mbs::comStartFile         = "StartFile";
const char* mbs::comStopFile          = "StopFile";

// tag names for xml config file:
const char* mbs::xmlServerName        = "MbsServerName";
const char* mbs::xmlServerKind        = "MbsServerKind";
const char* mbs::xmlServerPort        = "MbsServerPort";
const char* mbs::xmlServerScale       = "MbsServerScale";
const char* mbs::xmlServerLimit       = "MbsServerLimit";

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


void mbs::SubeventHeader::PrintHeader()
{
   printf("  SubEv ID %6u Type %2u/%1u Length %5u[w] Control %2u Subcrate %2u\n",
            (unsigned) ProcId(), (unsigned) Type(), (unsigned) SubType(), (unsigned) FullSize()/2, (unsigned) Control(), (unsigned) Subcrate());
}

void mbs::SubeventHeader::PrintData(bool ashex, bool aslong)
{
   if (aslong) {
      uint32_t* pl_data = (uint32_t*) RawData();
      uint32_t ll = RawDataSize() / 4;

      for(uint32_t l=0; l<ll; l++) {
         if(l%8 == 0) printf("  ");
         if (ashex)
            printf("%04x.%04x ",(unsigned) ((*pl_data>>16) & 0xffff), (unsigned) (*pl_data & 0xffff));
         else
            printf("%8u ", (unsigned) *pl_data);
         pl_data++;
         if(l%8 == 7) printf("\n");
      }

      if (ll%8 != 0) printf("\n");

   } else {
      uint16_t* pl_data = (uint16_t*) RawData();
      uint32_t ll = RawDataSize() / 2;

      for(uint32_t l=0; l<ll; l++) {
         if(l%8 == 0) printf("  ");
         printf("%8u ", (unsigned) *pl_data);
         pl_data++;
         if(l%8 == 7) printf("\n");
      }

      if (ll%8 != 0) printf("\n");
   }
}

unsigned mbs::EventHeader::NumSubevents() const
{
   unsigned cnt = 0;
   SubeventHeader* sub = nullptr;
   while ((sub = NextSubEvent(sub)) != nullptr) cnt++;
   return cnt;
}

void mbs::EventHeader::PrintHeader()
{
   if (Type() == 10) {
      printf("Event   %9u Type %2u/%1u Length %5u[w] Trigger %2u\n",
            (unsigned) EventNumber(), (unsigned) Type(), (unsigned) SubType(), (unsigned) FullSize()/2, (unsigned) TriggerNumber());
   } else {
      printf("Event type %u, subtype %u, data longwords %u",
            (unsigned) Type(), (unsigned) SubType(), (unsigned) FullSize()/2);
   }
}


void mbs::BufferHeader::Init(bool newformat)
{
   iWords = 0;
   iType = newformat ? MBS_TYPE(100,1) : MBS_TYPE(10,1);
   iUsed = 0;
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

uint32_t mbs::BufferHeader::FullSize() const
{
   return sizeof(BufferHeader) + iWords*2;
}

void mbs::BufferHeader::SetFullSize(uint32_t sz)
{
   iWords = (sz - sizeof(BufferHeader)) /2;
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
      case MBS_TYPE(10,1):
	 // For buffer sizes > 32k, i_used is not used, use iUsedWords. */
	 if (iWords > 16360 /* MAX__DLEN */) return iUsedWords * 2;
         return i_used * 2;

      default: break;
      //         EOUT("Unknown buffer type %d-%d", i_type, i_subtype);
   }

   return 0;
}

void mbs::BufferHeader::SetUsedBufferSize(uint32_t len)
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE(100,1):
         iUsedWords = len / 2;
         if (iWords == 0) SetFullSize(len + sizeof(BufferHeader));
         break;

      // old buffer type
      case MBS_TYPE(10,1):
         i_used = len / 2;
         if (iWords == 0) SetFullSize(len + sizeof(BufferHeader));
         break;

      default:
         //         EOUT("Unknown buffer type %d-%d", i_type, i_subtype);
         break;
   }
}

void mbs::BufferHeader::SetNumEvents(int32_t events)
{
   iNumEvents = events;
}

void mbs::SwapData(void* data, unsigned bytessize)
{
   if (!data) return;
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

int mbs::StrToServerKind(const std::string &str)
{
   if (str.empty()) return NoServer;
   if (str == "Transport") return TransportServer;
   if (str == "Stream") return StreamServer;
   if (str == "OldTransport") return OldTransportServer;
   if (str == "OldStream") return OldStreamServer;
   return NoServer;
}



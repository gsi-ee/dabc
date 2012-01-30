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

#ifndef MBS_MbsTypeDefs
#define MBS_MbsTypeDefs

#ifndef MBS_LmdTypeDefs
#include "LmdTypeDefs.h"
#endif

namespace mbs {

   enum EMbsTriggerTypes {
      tt_Event         = 1,
      tt_SpillOn       = 12,
      tt_SpillOff      = 13,
      tt_StartAcq      = 14,
      tt_StopAcq       = 15
   };

#pragma pack(push, 1)


   struct SubeventHeader : public Header {
      union {

        struct {

#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t  iProcId;     /*  Processor ID [as loaded from VAX] */
      int8_t   iSubcrate;   /*  Subcrate number */
      int8_t   iControl;    /*  Processor type code */
#else
      int8_t   iControl;    /*  Processor type code */
      int8_t   iSubcrate;   /*  Subcrate number */
      int16_t  iProcId;     /*  Processor ID [as loaded from VAX] */
#endif
        };

        uint32_t fFullId;   /** full subevent id */
      };

      void Init(uint8_t crate = 0, uint16_t procid = 0, uint8_t control = 0)
      {
         iWords = 0;
         iType = MBS_TYPE(10,1);
         iProcId = procid;
         iSubcrate = crate;
         iControl = control;
      }

      void *RawData() const { return (char*) this + sizeof(SubeventHeader); }

      // RawDataSize - size of raw data without subevent header
      uint32_t RawDataSize() const { return FullSize() - sizeof(SubeventHeader); }
      void SetRawDataSize(uint32_t sz) { SetFullSize(sz + sizeof(SubeventHeader)); }

   };

   typedef uint32_t EventNumType;

   struct EventHeader : public Header {
#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t       iDummy;     /*  Not used yet */
      int16_t       iTrigger;   /*  Trigger number */
#else
      int16_t       iTrigger;   /*  Trigger number */
      int16_t       iDummy;     /*  Not used yet */
#endif
      EventNumType  iEventNumber;

      void Init(EventNumType evnt = 0)
      {
         iWords = 0;
         iType = MBS_TYPE(10,1);
         iDummy = 0;
         iTrigger = tt_Event;
         iEventNumber = evnt;
      }

      void CopyHeader(EventHeader* src)
      {
         iDummy = src->iDummy;
         iTrigger = src->iTrigger;
         iEventNumber = src->iEventNumber;
      }

      inline EventNumType EventNumber() const { return iEventNumber; }
      inline void SetEventNumber(EventNumType ev) { iEventNumber = ev; }

      // SubEventsSize - size of all subevents, not includes events header
      inline uint32_t SubEventsSize() const { return FullSize() - sizeof(EventHeader); }
      inline void SetSubEventsSize(uint32_t sz) { SetFullSize(sz + sizeof(EventHeader)); }

      SubeventHeader* SubEvents() const
         { return (SubeventHeader*) ((char*) this + sizeof(EventHeader)); }

      SubeventHeader* NextSubEvent(SubeventHeader* prev) const
         { return prev == 0 ? (FullSize() > sizeof(EventHeader) ? SubEvents() : 0):
                (((char*) this + FullSize() > (char*) prev + prev->FullSize() + sizeof(SubeventHeader)) ?
                   (SubeventHeader*) (((char*) prev) + prev->FullSize()) : 0); }

      unsigned NumSubevents() const;
   };


   struct BufferHeader : public Header {
      union {
         struct {
#if BYTE_ORDER == LITTLE_ENDIAN
            int16_t  i_used;   /*  Used length of data field in words */
            int8_t   h_end;   /*  Fragment at begin of buffer */
            int8_t   h_begin;   /*  Fragment at end of buffer */
#else
            int8_t   h_begin;   /*  Fragment at end of buffer */
            int8_t   h_end;    /*  Fragment at begin of buffer */
            int16_t  i_used;   /*  Used length of data field in words */
#endif
         };
         int32_t iUsed;        // not used for type=100, low 16bits only
      };

      int32_t iBufferId;    // consequent buffer id
      int32_t iNumEvents;   // number of events in buffer
      int32_t iTemp;        // Used volatile
      int32_t iSeconds;
      int32_t iNanosec;
      int32_t iEndian;      // compatible with s_bufhe free[0]
      int32_t iLast;        // length of last event, free[1]
      int32_t iUsedWords;   // total words without header to read for type=100, free[2]
      int32_t iFree3;       // free[3]

      void Init(bool newformat);

      // length of buffer, which will be transported over socket
      uint32_t BufferLength() const;

      // UsedBufferSize - size of data after buffer header
      uint32_t UsedBufferSize() const;
      void SetUsedBufferSize(uint32_t len);

      void SetEndian() { iEndian = 1; }
      bool IsCorrectEndian() const { return iEndian == 1; }
   };

   struct TransportInfo {
      int32_t iEndian;      // byte order. Set to 1 by sender
      int32_t iMaxBytes;    // maximum buffer size
      int32_t iBuffers;     // buffers per stream
      int32_t iStreams;     // number of streams (could be set to -1 to indicate variable length buffers)
   };


#pragma pack(pop)

   enum EMbsBufferTypes {
      mbt_MbsEvents   = 103   // several mbs events without buffer header
   };

   // this is list of server kind, supported up to now by DABC
   enum EMbsServerKinds {
      NoServer = 0,
      TransportServer = 1,
      StreamServer = 2,
      OldTransportServer = 3,
      OldStreamServer = 4
   };

   extern const char* ServerKindToStr(int kind);
   extern int StrToServerKind(const char* str);
   extern int DefualtServerPort(int kind);

   extern void SwapData(void* data, unsigned bytessize);

   extern const char* typeLmdInput;
   extern const char* typeLmdOutput;
   extern const char* typeTextInput;
   extern const char* typeServerTransport;
   extern const char* typeClientTransport;

   extern const char* protocolLmd;
   extern const char* protocolMbs;
   extern const char* protocolMbsTransport;
   extern const char* protocolMbsStream;

   extern const char* comStartServer;
   extern const char* comStopServer;
   extern const char* comStartFile;
   extern const char* comStopFile;

   extern const char* portOutput;
   extern const char* portOutputFmt;
   extern const char* portInput;
   extern const char* portInputFmt;
   extern const char* portFileOutput;
   extern const char* portServerOutput;

   extern const char* xmlFileName;
   extern const char* xmlSizeLimit;
   extern const char* xmlServerName;
   extern const char* xmlServerKind;
   extern const char* xmlServerPort;
   extern const char* xmlServerScale;
   extern const char* xmlServerLimit;

   extern const char* xmlTextDataFormat;
   extern const char* xmlTextNumData;
   extern const char* xmlTextHeaderLines;
   extern const char* xmlTextCharBuffer;

   extern const char* xmlNormalOutput;
   extern const char* xmlFileOutput;
   extern const char* xmlServerOutput;
   extern const char* xmlCombineCompleteOnly;
   extern const char* xmlCheckSubeventIds;
   extern const char* xmlEvidMask;
   extern const char* xmlEvidTolerance;
   extern const char* xmlSpecialTriggerLimit;
   extern const char* xmlCombinerRatesPrefix;

};

#endif

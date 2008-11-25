#ifndef MBS_MbsTypeDefs
#define MBS_MbsTypeDefs

#include <stdint.h>
#include <endian.h>
#include <byteswap.h>

#include "dabc/Pointer.h"

#include "mbs/LmdTypeDefs.h"

namespace mbs {

   extern const char* xmlFileName;
   extern const char* xmlSizeLimit;
   extern const char* xmlFirstMultiple;
   extern const char* xmlNumMultiple;

   enum EMbsBufferTypes {
      mbt_Mbs10_1      = 101,  // old (fixed-length) mbs buffer with events and, probably, spanning
      mbt_Mbs100_1     = 102,  // new (variable-length) mbs buffer with events, no spanning
      mbt_MbsEvs10_1   = 103   // several mbs events (without buffer header)
   };

   // this is list of server kind, supported up to now by DABC
   enum EMbsServerKinds {
      NoServer = 0,
      TransportServer = 1,
      StreamServer = 2,
      OldTransportServer = 3,
      OldStreamServer = 4
   };

   class ReadIterator {
      public:
         ReadIterator(dabc::Buffer* buf);

         bool IsOk() const { return fBuffer!=0; }

         bool NextEvent();
         bool NextSubEvent();

         EventHeader* evnt() const { return (EventHeader*) fEvPtr(); }
         SubeventHeader* subevnt() const { return (SubeventHeader*) fSubPtr(); }
         void* rawdata() const { return fRawPtr(); }
         uint32_t rawdatasize() const { return subevnt() ? subevnt()->RawDataSize() : 0; }

      protected:
         dabc::Buffer*    fBuffer;
         dabc::Pointer    fEvPtr;
         dabc::Pointer    fSubPtr;
         dabc::Pointer    fRawPtr;
   };


   class WriteIterator {
      public:
         WriteIterator(dabc::Buffer* buf);
         ~WriteIterator();

         bool NewEvent(uint32_t event_number = 0, uint32_t mineventsize = 0);
         bool NewSubevent(uint32_t minrawsize = 0, uint8_t crate = 0, uint16_t procid = 0);
         bool FinishSubEvent(uint32_t rawdatasz);

         void Close();

         EventHeader* evnt() const { return (EventHeader*) fEvPtr(); }
         SubeventHeader* subevnt() const { return (SubeventHeader*) fSubPtr(); }
         void* rawdata() const { return subevnt() ? subevnt()->RawData() : 0; }
         uint32_t maxrawdatasize() const { return fSubPtr.null() ? 0 : fSubPtr.fullsize() - sizeof(SubeventHeader); }


      protected:
         dabc::Buffer*    fBuffer;
         dabc::Pointer    fEvPtr;
         dabc::Pointer    fSubPtr;
         dabc::BufferSize_t fFullSize;
   };




#pragma pack(1)

   typedef struct sMbsTransportInfo {
      int32_t iEndian;      // byte order. Set to 1 by sender
      int32_t iMaxBytes;    // maximum buffer size
      int32_t iBuffers;     // buffers per stream
      int32_t iStreams;     // number of streams (could be set to -1 to indicate var length buffers, size l_free[1])
   };

   // Buffer header, maps s_bufhe
   typedef struct sMbsBufferHeader {
      int32_t iMaxWords;    // compatible with s_bufhe (total buffer size - header)

      union {
        struct {
#if BYTE_ORDER == LITTLE_ENDIAN
          uint16_t i_type;
          uint16_t i_subtype;
#else
          uint16_t i_subtype;
          uint16_t i_type;
#endif
        };
        uint32_t iType; // compatible with s_bufhe, low=type (=100), high=subtype
      };

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

      bool Check();
      void Swap();
      int BufferLength();
      unsigned UsedBufferLength();
      void SetUsedLength(int len);
      void SetEndian() { iEndian = 1; }
      bool IsCorrectEndian() const { return iEndian == 1; }
   };

   typedef struct eMbsEventHeader {
      int32_t iMaxWords;    // compatible with s_bufhe (total buffer size - header)
      union {
        struct {
#if BYTE_ORDER == LITTLE_ENDIAN
          uint16_t i_type;
          uint16_t i_subtype;
#else
          uint16_t i_subtype;
          uint16_t i_type;
#endif
        };
        uint32_t iType; // compatible with s_bufhe, low=type (=100), high=subtype
      };
      int DataSize() const { return 8 + iMaxWords*2; } // returns complete size of the event
      void SetDataSize(int32_t len) { iMaxWords = (len - 8) / 2; }

   };

   typedef int32_t MbsEventId;


   typedef struct eMbs101EventHeader : public eMbsEventHeader {
#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t iDummy;     /*  Not used yet */
      int16_t iTrigger;   /*  Trigger number */
#else
      int16_t iTrigger;   /*  Trigger number */
      int16_t iDummy;    /*  Not used yet */
#endif
      int32_t  iCount;   /*  Current event number */

      void Dump101Event();

      void CopyFrom(const eMbs101EventHeader& src);

      int SubeventsDataSize() const { return DataSize() - sizeof(eMbs101EventHeader); }
   };

   typedef struct eMbs101SubeventHeader : public eMbsEventHeader {
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


#pragma pack(0)

   extern void IterateBuffer(void* buf, bool reset);
   extern void IterateEvent(void* evhdr);
   extern void SwapMbsData(void* data, int bytessize);

   extern bool FirstEvent(const dabc::Pointer& bufptr, dabc::Pointer& evptr, eMbs101EventHeader* &evhdr, void* tmp = 0);
   extern bool GetEventHeader(dabc::Pointer& evptr, eMbs101EventHeader* &evhdr, void* tmp = 0);
   extern bool NextEvent(dabc::Pointer& evptr, eMbs101EventHeader* &evhdr, void* tmp = 0);
   extern bool FirstSubEvent(const dabc::Pointer& evptr, eMbs101EventHeader* evhdr, dabc::Pointer& subevptr, eMbs101SubeventHeader* &subevhdr, void* tmp = 0);
   extern bool NextSubEvent(dabc::Pointer& subevptr, eMbs101SubeventHeader* &subevhdr, void* tmp = 0);

   extern bool StartBuffer(dabc::Buffer* buf, dabc::Pointer& evptr, sMbsBufferHeader* &bufhdr, void* tmp = 0, bool newformat = true);
   extern bool StartBuffer(dabc::Pointer& bufptr, dabc::Pointer& evptr, sMbsBufferHeader* &bufhdr, void* tmp = 0, bool newformat = true);
   extern bool StartEvent(dabc::Pointer& evptr, dabc::Pointer& subevptr, eMbs101EventHeader* &evhdr, void* tmp = 0);
   extern bool StartSubEvent(dabc::Pointer& subevptr, dabc::Pointer& subevdata, eMbs101SubeventHeader* &subevhdr, void* tmp = 0);
   extern bool FinishSubEvent(dabc::Pointer& subevptr, dabc::Pointer& subevdata, eMbs101SubeventHeader* subevhdr);
   extern dabc::BufferSize_t FinishEvent(dabc::Pointer& evptr, dabc::Pointer& subevptr, eMbs101EventHeader* evhdr, sMbsBufferHeader* bufhdr = 0);
   extern dabc::BufferSize_t FinishBuffer(dabc::Pointer& bufptr, dabc::Pointer& evptr, sMbsBufferHeader* bufhdr);
   extern bool FinishBuffer(dabc::Buffer* buf, dabc::Pointer& evptr, sMbsBufferHeader* bufhdr);

};

#endif

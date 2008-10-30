#ifndef MBS_LmdTypeDefs
#define MBS_LmdTypeDefs

#include <stdint.h>
#include <endian.h>

namespace mbs {

   enum EMbsTriggerTypes {
      tt_Event         = 1,
      tt_SpillOn       = 12,
      tt_SpillOff      = 13,
      tt_StartAcq      = 14,
      tt_StopAcq       = 15
   };

#if BYTE_ORDER == LITTLE_ENDIAN
#define MBS_TYPE(typ, subtyp) ((int32_t) typ | ((int32_t) subtyp) << 16)
#else
#define MBS_TYPE(typ, subtyp) ((int32_t) subtyp | ((int32_t) typ) << 16)
#endif

#define MBS_TYPE_10_1 MBS_TYPE(10,1)
#define MBS_TYPE_100_1 MBS_TYPE(100,1)


#pragma pack(push, 1)
    
   struct Header {
      uint32_t iWords;       // data words + 2
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

      // FullSize - size of data with header (8 bytes)
      uint32_t FullSize() const { return 8 + iWords*2; }
      void SetFullSize(uint32_t sz) { iWords = (sz - 8) /2; }
   }; 

   struct SubeventHeader : public Header {
#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t  iProcId;     /*  Processor ID [as loaded from VAX] */
      int8_t   iSubcrate;   /*  Subcrate number */
      int8_t   iControl;    /*  Processor type code */
#else
      int8_t   iControl;    /*  Processor type code */
      int8_t   iSubcrate;   /*  Subcrate number */
      int16_t  iProcId;     /*  Processor ID [as loaded from VAX] */
#endif

      void Init() 
      { 
         iWords = 0; 
         iType = MBS_TYPE_10_1; 
         iProcId = 0;
         iSubcrate = 0;
         iControl = 0;
      }

      void *RawData() const { return (char*) this + sizeof(SubeventHeader); }
      
      // RawDataSize - size of raw data without subevent header
      uint32_t RawDataSize() const { return FullSize() - sizeof(SubeventHeader); }
      void SetRawDataSize(uint32_t sz) { SetFullSize(sz + sizeof(SubeventHeader)); }
      
   };
    
   struct EventHeader : public Header {
#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t iDummy;     /*  Not used yet */
      int16_t iTrigger;   /*  Trigger number */
#else
      int16_t iTrigger;   /*  Trigger number */
      int16_t iDummy;     /*  Not used yet */
#endif
      uint32_t iEventNumber;

      void Init() 
      { 
         iWords = 0; 
         iType = MBS_TYPE_10_1; 
         iDummy = 0;
         iTrigger = tt_Event;
         iEventNumber = 0;
      }

      // SubEventsSize - size of all subevents, not includes events header
      uint32_t SubEventsSize() const { return FullSize() - sizeof(EventHeader); }
      void SetSubEventsSize(uint32_t sz) { SetFullSize(sz + sizeof(EventHeader)); }

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
      
      // UsedBufferSize - size of data after buffer header
      uint32_t UsedBufferSize();
      void SetUsedBufferSize(uint32_t len);
      
      void SetEndian() { iEndian = 1; }
      bool IsCorrectEndian() const { return iEndian == 1; }
   };
}

#pragma pack(pop)


#endif

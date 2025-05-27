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

#ifndef DOGMA_defines
#define DOGMA_defines

#include <cstdint>
#include <iostream>
#include <cstring>
#include <byteswap.h>

#pragma pack(push, 1)

#define DOGMA_MAGIC 0xecc1701d

#define SWAP32(v)  (((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24))

#define SWAP_VALUE(v) (((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24))

namespace dogma {

   /** \brief DOGMA transport unit header
    *
    * used as base for event and subevent
    * also common envelope for trd network data packets
    */

   struct DogmaTu {
      protected:
         uint32_t tuMagic = 0;
         uint32_t tuTrigTypeNumber = 0;
         uint32_t tuAddr = 0;
         uint32_t tuTrigTime = 0;
         uint32_t tuLocalTrigTime = 0;
         uint32_t tuLenPayload = 0; // number of 4bytes words

         /*
         inline uint32_t Value(const uint32_t *member) const
         {
            return IsSwapped() ? __bswap_32(*member) : *member;
         }

         inline void SetValue(uint32_t *member, uint32_t value)
         {
            *member = IsSwapped() ? __bswap_32(value) : value;
         }
         */

      public:

         inline bool IsSwapped() const  { return tuMagic != DOGMA_MAGIC; }

         inline bool IsMagic() const { return SWAP_VALUE(tuMagic) == DOGMA_MAGIC; }

         inline uint32_t GetAddr() const { return SWAP_VALUE(tuAddr); }

         inline uint32_t GetTrigType() const { return SWAP_VALUE(tuTrigTypeNumber) >> 24; }

         inline uint32_t GetTrigNumber() const { return SWAP_VALUE(tuTrigTypeNumber) & 0xffffff; }

         inline uint32_t GetTrigTypeNumber() const { return SWAP_VALUE(tuTrigTypeNumber); }

         inline uint32_t GetTrigTime() const { return SWAP_VALUE(tuTrigTime); }

         inline uint32_t GetLocalTrigTime() const { return SWAP_VALUE(tuLocalTrigTime); }

         inline uint32_t GetPayloadLen() const { return SWAP_VALUE(tuLenPayload) & 0xffff; }

         inline uint32_t GetErrorBits() const { return SWAP_VALUE(tuLenPayload) >> 24; }

         inline uint32_t GetFrameBits() const { return (SWAP_VALUE(tuLenPayload) >> 16) & 0xff; }

         inline void SetPayloadLen(uint32_t len)
         {
            uint32_t new_payload = (len & 0xffff) | (SWAP_VALUE(tuLenPayload) & 0xffff0000);
            tuLenPayload = SWAP_VALUE(new_payload);
         }

         inline uint32_t GetSize() const { return sizeof(DogmaTu) + GetPayloadLen()*4; }

         inline uint32_t GetPayload(uint32_t indx) const
         {
            uint32_t v = *(&tuLenPayload + 1 + indx);
            return SWAP_VALUE(v);
         }

         void *RawData() const { return (char *) this + sizeof(DogmaTu); }

         void SetTrigTypeNumber(uint32_t type_number) { tuTrigTypeNumber = SWAP_VALUE(type_number); }

         void Init(uint32_t type_number)
         {
            tuMagic = SWAP32(DOGMA_MAGIC);
            tuAddr = 0;
            SetTrigTypeNumber(type_number);
            tuTrigTime = 0;
            tuLocalTrigTime = 0;
            tuLenPayload = 0;
         }

   };

   struct DogmaEvent {
      protected:
         uint32_t tuMagic = 0;
         uint32_t tuSeqId = 0;
         uint32_t tuTrigTypeNumber = 0;
         uint32_t tuLenPayload = 0; // paylod len in 4bytes words

         /*
         inline uint32_t Value(const uint32_t *member) const
         {
            return IsSwapped() ? __bswap_32(*member) : *member;
         }

         inline void SetValue(uint32_t *member, uint32_t value)
         {
            *member = IsSwapped() ? __bswap_32(value) : value;
         }
         */

      public:

         inline bool IsSwapped() const  { return tuMagic != DOGMA_MAGIC; }
         inline bool IsMagic() const { return SWAP_VALUE(tuMagic) == DOGMA_MAGIC; }

         inline uint32_t GetSeqId() const { return SWAP_VALUE(tuSeqId); }
         inline uint32_t GetTrigType() const { return SWAP_VALUE(tuTrigTypeNumber) >> 24; }
         inline uint32_t GetTrigNumber() const { return SWAP_VALUE(tuTrigTypeNumber) & 0xffffff; }

         void Init(uint32_t seqid, uint32_t trig_type, uint32_t trig_number)
         {
            tuMagic = SWAP32(DOGMA_MAGIC);
            tuSeqId = SWAP_VALUE(seqid);
            uint32_t v = (trig_type << 24) | (trig_number & 0xffffff);
            tuTrigTypeNumber = SWAP_VALUE(v);
            tuLenPayload = 0;
         }

         inline uint32_t GetPayloadLen() const { return SWAP_VALUE(tuLenPayload) & 0xffff; }
         inline void SetPayloadLen(uint32_t len) { tuLenPayload = SWAP_VALUE(len); }

         inline uint32_t GetEventLen() const { return sizeof(DogmaEvent) + GetPayloadLen() * 4; }

         DogmaTu *FirstSubevent() const { return (DogmaTu *)((char *) this + sizeof(DogmaEvent)); }

         DogmaTu *NextSubevent(DogmaTu *prev) const
         {
            if (!prev)
               return FirstSubevent();
            char *next = (char *) prev + prev->GetSize();

            return (next >= (char *) this + GetEventLen()) ? nullptr : (DogmaTu *) next;
         }
   };

}

#pragma pack(pop)

#endif

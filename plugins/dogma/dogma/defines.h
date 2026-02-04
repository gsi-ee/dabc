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

#pragma pack(push, 1)

#define DOGMA_MAGIC 0xecc1701d

#define SWAP_VALUE(v) (((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24))

namespace dogma {

   /** \brief DOGMA transport unit header
    *
    * used for subevents transport
    * also common envelope for trd network data packets
    */

   struct DogmaTu {
      protected:
         uint32_t tuAddr = 0;
         uint32_t tuDeviceId = 0;
         uint32_t tuLenPayload = 0; // number of bytes in payload
         uint32_t tuTrigTimeHigh = 0;
         uint32_t tuTrigTimeLow = 0;
         uint32_t tuTrigTypeNumber = 0;

      public:

         inline uint32_t GetAddr() const { return SWAP_VALUE(tuAddr); }

         inline uint32_t GetDeviceId() const { return SWAP_VALUE(tuDeviceId); }

         inline uint32_t GetTrigType() const { return SWAP_VALUE(tuTrigTypeNumber) >> 28; }

         inline uint32_t GetTrigNumber() const { return SWAP_VALUE(tuTrigTypeNumber) & 0xfffffff; }

         inline uint32_t GetTrigTypeNumber() const { return SWAP_VALUE(tuTrigTypeNumber); }

         inline uint64_t GetTrigTime() const { return (((uint64_t) SWAP_VALUE(tuTrigTimeHigh)) << 32) | SWAP_VALUE(tuTrigTimeLow); }

         inline uint32_t GetPayloadLen() const { return SWAP_VALUE(tuLenPayload); }

         // size always rounded to 4 bytes and includes header
         inline uint32_t GetSize() const
         {
            uint32_t len = GetPayloadLen(), cut = len % 4;
            return sizeof(DogmaTu) + len + (cut > 0 ? 4 - cut : 0);
         }

         inline uint8_t GetPayload(uint32_t indx) const
         {
            uint8_t *beg = (uint8_t *)(&tuLenPayload + 4);
            return *(beg + indx);
         }

         // this points on the start of raw header send from front end
         void *RawHeader() const { return (char *) this + 12; }

         uint32_t GetRawPacketSize() const { return GetPayloadLen() + 12; }

         void *RawData() const { return (char *) this + sizeof(DogmaTu); }

         inline void SetAddr(uint32_t addr) { tuAddr = SWAP_VALUE(addr); }

         inline void SetDeviceId(uint32_t id) { tuDeviceId = SWAP_VALUE(id); }

         void SetTrigTypeNumber(uint32_t type_number) { tuTrigTypeNumber = SWAP_VALUE(type_number); }

         inline void SetPayloadLen(uint32_t len) { tuLenPayload = SWAP_VALUE(len); }

         inline void SetRawPacketSize(uint32_t sz)
         {
            SetPayloadLen(sz > 12 ? sz - 12 : 0);
         }

         void Init(uint32_t type_number)
         {
            tuAddr = 0;
            tuDeviceId = 0;
            SetTrigTypeNumber(type_number);
            tuTrigTimeHigh = 0;
            tuTrigTimeLow = 0;
            tuLenPayload = 0;
         }

   };

   struct DogmaEvent {
      protected:
         uint32_t tuSeqId = 0;
         uint32_t tuTrigTypeNumber = 0;
         uint32_t tuLenPayload = 0; // payload len in 4bytes words

      public:

         inline uint32_t GetSeqId() const { return SWAP_VALUE(tuSeqId); }
         inline uint32_t GetTrigType() const { return SWAP_VALUE(tuTrigTypeNumber) >> 24; }
         inline uint32_t GetTrigNumber() const { return SWAP_VALUE(tuTrigTypeNumber) & 0xffffff; }
         inline uint32_t GetPayloadLen() const { return SWAP_VALUE(tuLenPayload) & 0xffff; }
         inline uint32_t GetEventLen() const { return sizeof(DogmaEvent) + GetPayloadLen() * 4; }

         void Init(uint32_t seqid, uint32_t trig_type, uint32_t trig_number)
         {
            tuSeqId = SWAP_VALUE(seqid);
            uint32_t v = (trig_type << 24) | (trig_number & 0xffffff);
            tuTrigTypeNumber = SWAP_VALUE(v);
            tuLenPayload = 0;
         }

         inline void SetPayloadLen(uint32_t len) { tuLenPayload = SWAP_VALUE(len); }

         inline void SetEventLen(uint32_t sz) { SetPayloadLen((sz - sizeof(DogmaEvent)) / 4); }

         inline DogmaTu *FirstSubevent() const { return GetPayloadLen() * 4 < sizeof(DogmaTu) ? nullptr : (DogmaTu *)((char *) this + sizeof(DogmaEvent)); }

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

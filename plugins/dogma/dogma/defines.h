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

#define SWAP32(v) (((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24))

namespace dogma {

   /** \brief DOGMA transport unit header
    *
    * used as base for event and subevent
    * also common envelope for trd network data packets
    */

   struct DogmaTu {
      protected:
         uint32_t tuMagic = 0;
         uint32_t tuAddr = 0;
         uint32_t tuTrigTypeNumber = 0;
         uint32_t tuTrigTime = 0;
         uint32_t tuLenPayload = 0;
         inline uint32_t Value(const uint32_t *member) const
         {
            return IsSwapped() ? SWAP32(*member) : *member;
         }

         inline void SetValue(uint32_t *member, uint32_t value)
         {
            *member = IsSwapped() ? SWAP32(value) : value;
         }


      public:

         inline bool IsSwapped() const  { return tuMagic != DOGMA_MAGIC; }

         inline bool IsMagic() const { return Value(&tuMagic) == DOGMA_MAGIC; }

         inline uint32_t GetAddr() const { return Value(&tuAddr); }

         inline uint32_t GetTrigType() const { return Value(&tuTrigTypeNumber) >> 24; }

         inline uint32_t GetTrigNumber() const { return Value(&tuTrigTypeNumber) & 0xffffff; }

         inline uint32_t GetTrigTime() const { return Value(&tuTrigTime); }

         inline uint32_t GetPayloadLen() const { return Value(&tuLenPayload) & 0xffff; }

         inline uint32_t GetTuLen() const { return sizeof(DogmaTu) + GetPayloadLen(); }

         inline void SetPayloadLen(uint32_t len) { SetValue(&tuLenPayload, len); }

         inline uint32_t GetSize() const { return sizeof(DogmaTu) + GetPayloadLen(); }

         inline uint32_t GetPayload(uint32_t indx) const { return Value(&tuLenPayload + 1 + indx); }

         void *RawData() const { return (char *) this + sizeof(DogmaTu); }

         void Init(uint32_t type_number)
         {
            tuMagic = SWAP32(DOGMA_MAGIC);
            tuAddr = 0;
            tuTrigTypeNumber = 0;
            tuTrigTypeNumber = SWAP32(type_number);
            tuLenPayload = 0;
         }
   };

   struct DogmaEvent {
      protected:
         uint32_t tuMagic = 0;
         uint32_t tuTrigTypeNumber = 0;
         uint32_t tuLenPayload = 0;

         inline uint32_t Value(const uint32_t *member) const
         {
            return IsSwapped() ? SWAP32(*member) : *member;
         }

         inline void SetValue(uint32_t *member, uint32_t value)
         {
            *member = IsSwapped() ? SWAP32(value) : value;
         }

      public:

         inline bool IsSwapped() const  { return tuMagic != DOGMA_MAGIC; }
         inline bool IsMagic() const { return Value(&tuMagic) == DOGMA_MAGIC; }

         inline uint32_t GetTrigType() const { return Value(&tuTrigTypeNumber) >> 24; }
         inline uint32_t GetTrigNumber() const { return Value(&tuTrigTypeNumber) & 0xffffff; }

         void Init(uint32_t type_number)
         {
            tuMagic = SWAP32(DOGMA_MAGIC);
            SetValue(&tuTrigTypeNumber, type_number);
            tuLenPayload = 0;
         }

         inline uint32_t GetPayloadLen() const { return Value(&tuLenPayload) & 0xffff; }
         inline void SetPayloadLen(uint32_t len) { SetValue(&tuLenPayload, len); }

         inline uint32_t GetEventLen() const { return sizeof(DogmaEvent) + GetPayloadLen(); }


         DogmaTu *FirstSubevent() const { return (DogmaTu *)((char *) this + sizeof(DogmaEvent)); }


   };

}

#pragma pack(pop)

#endif

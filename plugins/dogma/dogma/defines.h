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
            return IsSwapped() ? ((*member & 0xFF) << 24) |
                                 ((*member & 0xFF00) << 8) |
                                 ((*member & 0xFF0000) >> 8) |
                                 ((*member & 0xFF000000) >> 24) : *member;
         }

      public:

         inline bool IsSwapped() const  { return tuMagic != 0xecc1701d; }

         inline bool IsMagic() const { return Value(&tuMagic) == 0xecc1701d; }

         inline uint32_t GetAddr() const { return Value(&tuAddr); }

         inline uint32_t GetTrigType() const { return Value(&tuTrigTypeNumber) << 24; }

         inline uint32_t GetTrigNumber() const { return Value(&tuTrigTypeNumber) & 0xffffff; }

         inline uint32_t GetTrigTime() const { return Value(&tuTrigTime); }

         inline uint32_t GetPayloadLen() const { return Value(&tuLenPayload) & 0xffff; }

         inline uint32_t GetSize() const { return 20 + GetPayloadLen(); }

   };

}

#pragma pack(pop)

#endif

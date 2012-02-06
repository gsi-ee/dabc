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

#ifndef DABC_EventId
#define DABC_EventId

#include <stdint.h>

#include <string>

namespace dabc {

   struct EventId {
      uint64_t value;

      inline EventId() : value(0) {}

      inline EventId(const EventId& src) : value(src.value) {}

      inline EventId(uint16_t code) : value(code) {}

      inline EventId(uint16_t code, uint16_t item) : value((uint64_t) item << 48 | code) {}

      inline EventId(uint16_t code, uint16_t item, uint32_t arg) :
         value((((uint64_t) item) << 48) | (((uint64_t) arg) << 16) | code) {}

      inline EventId& operator=(const EventId& src) { value = src.value; return *this; }

      inline EventId& operator=(uint64_t _value) { value = _value; return *this; }

      inline bool isnull() const { return value==0; }

      inline bool notnull() const { return value!=0; }

      inline uint16_t GetCode() const { return value &0xffffLU; }
      inline uint16_t GetItem() const { return value >> 48; }
      inline uint32_t GetArg() const { return (value >> 16) & 0xffffffffLU; }

      std::string asstring() const;

      inline static EventId Null() { return EventId(); }
   };

}

#endif

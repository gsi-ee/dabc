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

#ifndef MBS_LmdTypeDefs
#define MBS_LmdTypeDefs

#include <stdint.h>

#ifdef DABC_MAC
#include <machine/endian.h>
#else
#include <endian.h>
#endif

namespace mbs {

#if BYTE_ORDER == LITTLE_ENDIAN
#define MBS_TYPE(typ, subtyp) ((int32_t) typ | ((int32_t) subtyp) << 16)
#else
#define MBS_TYPE(typ, subtyp) ((int32_t) subtyp | ((int32_t) typ) << 16)
#endif

#pragma pack(push, 1)

   /** \brief Structure of any entry in LMD file */

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
        uint32_t iType;
      };

      // FullSize - size of data with header (8 bytes)
      inline uint32_t FullSize() const { return 8 + iWords*2; }
      inline void SetFullSize(uint32_t sz) { iWords = (sz - 8) /2; }

      inline uint32_t FullType() const { return iType; }
      inline uint16_t Type() const { return i_type; }
      inline uint16_t SubType() const { return i_subtype; }

      inline void SetFullType(uint32_t typ) { iType = typ; }
      inline void SetTypePair(unsigned typ, unsigned subtyp) { iType = (typ & 0xffff) | ((subtyp << 16) & 0xffff0000); }

      inline bool isFullType(uint32_t typ) const { return iType == typ; }
      inline bool isTypePair(unsigned typ, unsigned subtyp) const { return iType == ((typ & 0xffff) | ((subtyp << 16) & 0xffff0000)); }
   };

   // ____________________________________________________________________

   /** \brief Structure of the LMD file header */

   struct FileHeader : public Header {
      uint64_t iTableOffset;    ///< optional offset to element offset table in file
      uint32_t iElements;       ///< compatible with s_bufhe
      uint32_t iOffsetSize;     ///< Offset size, 4 or 8 [bytes]
      uint32_t iTimeSpecSec;    ///< compatible with s_bufhe (2*32bit)
      uint32_t iTimeSpecNanoSec; ///< compatible with s_bufhe (2*32bit)
      //  struct timespec TimeSpec;
      uint32_t iEndian;         ///< compatible with s_bufhe free[0]
      uint32_t iWrittenEndian;  ///< one of LMD__ENDIAN_x
      uint32_t iUsedWords;      ///< total words without header to read for type=100, free[2]
      uint32_t iFree3;          ///< free[3]
   };

#pragma pack(pop)

}

#endif

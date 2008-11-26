#ifndef MBS_LmdTypeDefs
#define MBS_LmdTypeDefs

#include <stdint.h>
#include <endian.h>

namespace mbs {

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
        uint32_t iType;
      };

      // FullSize - size of data with header (8 bytes)
      inline uint32_t FullSize() const { return 8 + iWords*2; }
      inline void SetFullSize(uint32_t sz) { iWords = (sz - 8) /2; }

      inline uint32_t Type() const { return iType; }
   };


#pragma pack(pop)

}

#endif

#ifndef CRC32_H
#define CRC32_H

#include "xbasic_types.h"

void init_crc32();
Xuint32 crc32(void *data, Xuint32 len);
Xuint16 crc16(Xuint16 *iph, int len);

#endif

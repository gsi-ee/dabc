#ifndef ETHER_H
#define ETHER_H

#include "xbasic_types.h"

//Ethernet CRC and padding is done by xilinx driver
#define ETHER_CRC_SIZE 4
#define MAX_ETHER_FRAME_SIZE (1518 - ETHER_CRC_SIZE)
#define MIN_ETHER_FRAME_SIZE (64 - ETHER_CRC_SIZE)

#define ETHER_ADDR_LEN 6
#define ETHERTYPE_IP   0x0800
#define ETHERTYPE_ARP   0x0806

struct ether_header {
   Xuint8 dest_addr[ETHER_ADDR_LEN];
   Xuint8 src_addr[ETHER_ADDR_LEN];
   Xuint16 ether_type;
};

#define ETHER_HEADER_OFFSET 0
#define ETHER_PAYLOAD_OFFSET ETHER_HEADER_OFFSET + sizeof(struct ether_header)
#define MAX_ETHER_PAYLOAD MAX_ETHER_FRAME_SIZE - sizeof(struct ether_header)

void cpy_ether_addr(Xuint8 *dest, Xuint8 *src);
int ether_addr_equal(Xuint8 *one, Xuint8 *two);

#endif

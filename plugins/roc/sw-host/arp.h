#ifndef ARP_H
#define ARP_H

#include "ether.h"
#include "ip.h"

#define ARP_HTYPE_ETHER 1
#define ARP_PTYPE_IP 0x0800
#define ARP_REQUEST 1
#define ARP_REPLY 2

struct arp_header {
   Xuint16 htype;
   Xuint16 ptype;
   Xuint8 hlen;
   Xuint8 plen;
   Xuint16 oper;
   Xuint8 sha[ETHER_ADDR_LEN];
   Xuint8 spa[IP_ADDR_LEN];
   Xuint8 tha[ETHER_ADDR_LEN];
   Xuint8 tpa[IP_ADDR_LEN];
};

#define ARP_HEADER_OFFSET ETHER_PAYLOAD_OFFSET
#define ARP_PACKET_SIZE sizeof(struct ether_header) + sizeof(struct arp_header)

#endif

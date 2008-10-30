#ifndef UDP_H
#define UDP_H

#include "ip.h"

struct udp_header {
   Xuint16 sport;
   Xuint16 dport;
   //using data and header for len
   Xuint16 len;
   Xuint16 chksum;
};

#define UDP_HEADER_OFFSET IP_PAYLOAD_OFFSET
#define UDP_PAYLOAD_OFFSET UDP_HEADER_OFFSET + sizeof(struct udp_header)
#define MAX_UDP_PAYLOAD MAX_IP_PAYLOAD - sizeof(struct udp_header)

#endif

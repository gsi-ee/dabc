#ifndef IP_H
#define IP_H

#include "ether.h"

#define IP_ADDR_LEN 4
#define IP_VERSION_4 4
#define IP_HEADER_LENGTH 5 //is stored in 32bit units 5byte*32bit/8bit = 20 bytes
/**********************************************
 * time to live: Hop-count: Number of surviving
 * routing points
 *********************************************/
#define IP_TTL 5
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17
/**********************************************
 * QoS priorities
 * Bits 0-5:   DSCP (Differentiated Services Code Point)
 *          -   IP_DSCP_LOWEST_PRIORITY =  b"000000"
 *          -   IP_DSCP_HIGHEST_PRIORITY = b"101110"
 * Bits 6-7:   ECN (Explicit Congestion Notification - IP Flusskontrolle)
 *          -   0     0         Not-ECT
 *********************************************/
#define IP_DSCP_LOWEST_PRIORITY 0
#define IP_DSCP_HIGHEST_PRIORITY 184
//do not allow fragmentation of packets
#define IP_FLAGS 2

struct ip_header {
   Xuint8 version : 4;
   Xuint8 header_length : 4;
   Xuint8 type_of_service;
   //ip header + data length
   Xuint16 total_length;
   Xuint16 packet_number_id;
   Xuint16 flags : 3;
   Xuint16 fragment_offset : 13;
   Xuint8 time_to_live;
   Xuint8 proto;
   Xuint16 header_chksum;
   Xuint8 src[IP_ADDR_LEN];
   Xuint8 dest[IP_ADDR_LEN];
   //max 40 bytes
   //xuint8 options;
   //has to be padded to a multiple of 32 bits
   //Xuint8 pad : X;
};

#define IP_HEADER_OFFSET ETHER_PAYLOAD_OFFSET
#define IP_PAYLOAD_OFFSET IP_HEADER_OFFSET + sizeof(struct ip_header)
#define MAX_IP_PAYLOAD MAX_ETHER_PAYLOAD - sizeof(struct ip_header)

#define ICMP_TYPE_PING_REQUEST   0x8
#define ICMP_TYPE_PING_REPLY    0x0

struct icmp_header {
   // ICMP Message Typ
   Xuint8 type;
   // Message Interner (Fehler-)Code
   Xuint8 code;
   // Checksum für das ICMP Paket
   Xuint16 chksum;
   // ID des Pakets, oft wie Port bei TCP / UDP genutzt
   Xuint16 usID;
   // Sequenznummer, bei mehreren typgleichen, (sinn-)zusammenhängenden Paketen
   Xuint16 usSequence;
    // Zeitstempel beim Abesenden
   Xuint32 ulTimeStamp;
};

#define ICMP_HEADER_OFFSET IP_PAYLOAD_OFFSET

#define cpy_ip_addr(dest, src) ( *( (Xuint32*)dest ) = *( (Xuint32*)src ) )
#define ip_addr_equal(addr1, addr2) ( *((Xuint32*)addr1) == *((Xuint32*)addr2) )

#endif

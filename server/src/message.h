#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <stdint.h>

typedef struct dhcp_packet {
        uint8_t opcode;
        uint8_t htype;
        uint8_t hlen;
        uint8_t hops;

        uint32_t xid;

        uint16_t secs;
        uint16_t flags;

        uint32_t ciaddr;
        uint32_t yiaddr;
        uint32_t siaddr;
        uint32_t giaddr;
        uint8_t chaddr[16];

        uint8_t overflow_space[192];

        uint32_t cookie;
        uint8_t options[336];
} dhcp_packet_t;

/* dumps dhcp message header to stdout */
void dhcp_packet_dump(dhcp_packet_t *h);

#endif /* __MESSAGE_H__ */

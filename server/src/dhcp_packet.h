#ifndef __PACKET_H__
#define __PACKET_H__

#include "utils/llist.h"

#include <stdint.h>

#define CHADDR_STRLEN 18

enum dhcp_message_type {
    DHCP_DISCOVER,
    DHCP_OFFER,
    DHCP_REQUEST,
    DHCP_ACK,
    DHCP_NAK,
    DHCP_RELEASE,
    DHCP_INFORM
};

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

/**
 * Semi-parsed version of dhcp_packet_t struct. 
 * Contains the raw dhcp packet and any information that makes sense to parse 
 * out of the packet (e.g. DHCP options, type of message)
 */
typedef struct dhcp_message {
    dhcp_packet_t packet;
    enum dhcp_message_type type;
    char chaddr[CHADDR_STRLEN];
    llist_t *dhcp_options;
} dhcp_message_t;

/* dumps dhcp message header to stdout */
void dhcp_packet_dump(dhcp_packet_t *p);

/*
 * Convert dhcp packet feilds from network byte order to host byte order
 * Returns 0 on success, -1 on failure
 */
void dhcp_packet_convert_to_local(dhcp_packet_t *p);

/*
 * Convert dhcp packet feilds from host byte order to network byte order
 * Returns 0 on success, -1 on failure
 */
void dhcp_packet_convert_to_network(dhcp_packet_t *p);

/**
 * Parses the relevant contents of m->dhcp_packet_t and stores the result in 
 * the parent dhcp_message_t structure
 * Returns 0 on success, -1 on error
 */
int dhcp_packet_parse(dhcp_message_t *m);

/**
 * Allocates space in memory to store a dhcp_message_t struct.
 * Returns null on failure, pointer to allocated struct on success 
 */
dhcp_message_t *dhcp_message_new();

/**
 * Frees allocated memory from dhcp_message_t struct and sets the pointer to it 
 * to null
 */
void dhcp_message_destroy(dhcp_message_t **m);

#endif /* __PACKET_H__ */

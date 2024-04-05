#ifndef __PACKET_H__
#define __PACKET_H__

#include "utils/llist.h"
#include "RFC/RFC-2131.h"

#include <stdint.h>

#define CHADDR_LEN 18

/*
 * Raw dhcp packet as received from recv() syscall. 
 * WARNING: This is RAW packet in NETWORK byte order, do NOT use as a means 
 * of getting information, use parent dhcp_message_t parsed structure.
 */
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

    char sname[64];
    char filename[128];

    uint32_t cookie;
    uint8_t options[336];
} dhcp_packet_t;

/**
 * Parsed version of dhcp_packet_t struct in host byte order. 
 * Contains the raw dhcp packet and and parsed information from said packet
 */
typedef struct dhcp_message {
    /* raw dhcp packet as received from recv() syscall, in NETWORK byte order */
    dhcp_packet_t packet;
    enum dhcp_message_type type;

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
    
    uint8_t chaddr[CHADDR_LEN];
    char sname[64];
    char filename[128];
    uint32_t cookie;
   
    /* Linked list containing parsed dhcp options */
    llist_t *dhcp_options;
    /* UNIX time indicating when was the message sent/received */
    uint32_t time;
} dhcp_message_t;

/* dumps dhcp message header to stdout */
void dhcp_packet_dump(dhcp_packet_t *p);

/**
 * Parses the contents of m->dhcp_packet_t and stores the result in 
 * the parent dhcp_message_t structure
 * Returns 0 on success, -1 on error
 */
int dhcp_packet_parse(dhcp_message_t *m);

/**
 * Builds a raw packet in network byte order (m->dhcp_packet_t) using information 
 * from parent dhcp_message_t struct. This packet is than ready to be sent.
 */
int dhcp_packet_build(dhcp_message_t *m);

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

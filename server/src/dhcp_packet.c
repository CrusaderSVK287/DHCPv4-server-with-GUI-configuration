#include "dhcp_packet.h"
#include "RFC/RFC-2131.h"
#include "RFC/RFC-2132.h"
#include "dhcp_options.h"
#include "logging.h"
#include "utils/llist.h"
#include "utils/xtoy.h"

#include <cclog_macros.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void dhcp_packet_dump(dhcp_packet_t *p)
{
        if (!p)
                return;

        printf("opcode: %02x\n", p->opcode);
        printf("htype:  %02x\n", p->htype);
        printf("hlen:   %02x\n", p->hlen);
        printf("hops:   %02x\n", p->hops);

        printf("xid:    %08x\n", p->xid);

        printf("secs:   %04x\n", p->secs);
        printf("flags:  %04x\n", p->flags);

        printf("ciaddr: %s\n", uint32_to_ipv4_address(p->ciaddr));
        printf("yiaddr: %s\n", uint32_to_ipv4_address(p->yiaddr));
        printf("siaddr: %s\n", uint32_to_ipv4_address(p->siaddr));
        printf("giaddr: %s\n", uint32_to_ipv4_address(p->giaddr));
        printf("chaddr: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                        p->chaddr[0],
                        p->chaddr[1], 
                        p->chaddr[2], 
                        p->chaddr[3],
                        p->chaddr[4],
                        p->chaddr[5]);

        printf("cookie: %08x\n", p->cookie);

        for (size_t i = 0; i < 336; i++)
        {
                if (i % 16 == 0) {
                        printf("\n");
                }
                printf("%02x ", p->options[i]);
        }
        printf("\n");
}

int dhcp_packet_parse(dhcp_message_t *m)
{
        int rv = -1;
        if_null(m, exit);

        m->opcode = m->packet.opcode;
        m->htype = m->packet.htype;
        m->hlen = m->packet.hlen;
        m->hops = m->packet.hops;

        m->xid = ntohl(m->packet.xid);
        m->secs = ntohs(m->packet.secs);
        m->flags = ntohs(m->packet.flags);

        m->ciaddr = ntohl(m->packet.ciaddr);
        m->yiaddr = ntohl(m->packet.yiaddr);
        m->siaddr = ntohl(m->packet.siaddr);
        m->giaddr = ntohl(m->packet.giaddr);

        memcpy(m->chaddr, m->packet.chaddr, 16);
        memcpy(m->sname, m->packet.sname, 64);
        memcpy(m->filename, m->packet.filename, 128);

        m->cookie = ntohl(m->packet.cookie);
        if (m->cookie != MAGIC_COOKIE) {
                cclog(LOG_WARN, NULL, 
                        "Received DHCP message with invalid cookie, message will be dropped");
                goto exit;
        }

        if_null(m->dhcp_options, exit);
        if_failed(dhcp_option_parse(m->dhcp_options, m->packet.options), exit);

        /* 
         * Assign the message a type. This is required by this implementation of 
         * DHCP server. If the option 53 is not present, the message MUST be rejected
         */ 
        
        dhcp_option_t *o = dhcp_option_retrieve(m->dhcp_options, DHCP_OPTION_DHCP_MESSAGE_TYPE);
        if_null_log(o, exit, LOG_WARN, NULL,
                        "Received DHCP message of with missing option 53, message will be dropped");

        m->type = o->value.number;

        rv = 0;
exit:
        return rv;
}

int dhcp_packet_build(dhcp_message_t *m)
{
        int rv = -1;
        if_null(m, exit);

        m->packet.opcode = m->opcode;
        m->packet.htype = m->htype;
        m->packet.hlen = m->hlen;
        m->packet.hops = m->hops;

        m->packet.xid = htonl(m->xid);
        m->packet.secs = htons(m->secs);
        m->packet.flags = htons(m->flags);


        m->packet.ciaddr = htonl(m->ciaddr);
        m->packet.yiaddr = htonl(m->yiaddr);
        m->packet.siaddr = htonl(m->siaddr);
        m->packet.giaddr = htonl(m->giaddr);

        memcpy(m->packet.chaddr, m->chaddr, 16);
        memcpy(m->packet.sname, m->sname, 64);
        memcpy(m->packet.filename, m->filename, 128);
        
        if (m->cookie != MAGIC_COOKIE) {
                cclog(LOG_WARN, NULL, 
                        "Created DHCP message has invalid cookie, terminating packet build");
                goto exit;
        }

        m->packet.cookie = htonl(m->cookie); 

        if_failed_log(dhcp_options_serialize(m->dhcp_options, m->packet.options), exit, LOG_ERROR,
                        NULL, "Failed to serialize dhcp options, terminating packet build");

        rv = 0;
exit:
        return rv;
}

dhcp_message_t *dhcp_message_new()
{
        dhcp_message_t *m = calloc(1, sizeof(dhcp_message_t));
        if_null_log(m, error, LOG_ERROR, NULL, "Could not allocate memory for dhcp_message");

        m->dhcp_options = llist_new();
        if_null_log(m->dhcp_options, error, LOG_ERROR, NULL, "Could not allocate memory for llist");

        return m;
error:
        return NULL;
}

void dhcp_message_destroy(dhcp_message_t **m)
{
        if (!m)
                return;

        dhcp_option_destroy_list(&(*m)->dhcp_options);
        
        free(*m);
        *m = NULL;
}


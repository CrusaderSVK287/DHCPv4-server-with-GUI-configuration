#include "dhcp_packet.h"
#include "logging.h"
#include "utils/llist.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
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

        printf("ciaddr: %s\n", inet_ntop(AF_INET, &p->ciaddr, NULL, sizeof(p->ciaddr)));
        printf("yiaddr: %s\n", inet_ntop(AF_INET, &p->yiaddr, NULL, sizeof(p->yiaddr)));
        printf("siaddr: %s\n", inet_ntop(AF_INET, &p->siaddr, NULL, sizeof(p->siaddr)));
        printf("giaddr: %s\n", inet_ntop(AF_INET, &p->giaddr, NULL, sizeof(p->giaddr)));
        printf("chaddr: %02x:%02x:%02x:%02x:%02x:%02x\n", p->chaddr[0], p->chaddr[1], p->chaddr[2], p->chaddr[3], p->chaddr[4], p->chaddr[5]);

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

void dhcp_packet_convert_to_local(dhcp_packet_t *p)
{
        p->xid    = ntohl(p->xid);
        p->ciaddr = ntohl(p->ciaddr);
        p->yiaddr = ntohl(p->yiaddr);
        p->siaddr = ntohl(p->siaddr);
        p->giaddr = ntohl(p->giaddr);
        p->cookie = ntohl(p->cookie);
}

void dhcp_packet_convert_to_network(dhcp_packet_t *p)
{
        p->xid    = htonl(p->xid);
        p->ciaddr = htonl(p->ciaddr);
        p->yiaddr = htonl(p->yiaddr);
        p->siaddr = htonl(p->siaddr);
        p->giaddr = htonl(p->giaddr);
        p->cookie = htonl(p->cookie);
}

int dhcp_packet_parse(dhcp_message_t *m)
{
        int rv = -1;
        if_null(m, exit);

        

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

        llist_destroy(&(*m)->dhcp_options);
        
        free(*m);
        *m = NULL;
}


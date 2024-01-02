#define _POSIX_C_SOURCE 200112L

#include "message.h"
#include "logging.h"

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


void dhcp_packet_dump(dhcp_packet_t *h)
{
        if_null(h, exit);

        printf("opcode: %02x\n", h->opcode);
        printf("htype:  %02x\n", h->htype);
        printf("hlen:   %02x\n", h->hlen);
        printf("hops:   %02x\n", h->hops);

        printf("xid:    %08x\n", h->xid);

        printf("secs:   %04x\n", h->secs);
        printf("flags:  %04x\n", h->flags);

        printf("ciaddr: %s\n", inet_ntop(AF_INET, &h->ciaddr, NULL, sizeof(h->ciaddr)));
        printf("yiaddr: %s\n", inet_ntop(AF_INET, &h->yiaddr, NULL, sizeof(h->yiaddr)));
        printf("siaddr: %s\n", inet_ntop(AF_INET, &h->siaddr, NULL, sizeof(h->siaddr)));
        printf("giaddr: %s\n", inet_ntop(AF_INET, &h->giaddr, NULL, sizeof(h->giaddr)));
        printf("chaddr: %02x:%02x:%02x:%02x:%02x:%02x\n", h->chaddr[0], h->chaddr[1], h->chaddr[2], h->chaddr[3], h->chaddr[4], h->chaddr[5]);

        printf("cookie: %08x\n", h->cookie);

        for (size_t i = 0; i < 336; i++)
        {
                if (i % 16 == 0) {
                        printf("\n");
                }
                printf("%02x ", h->options[i]);
        }
        printf("\n");

exit:
}
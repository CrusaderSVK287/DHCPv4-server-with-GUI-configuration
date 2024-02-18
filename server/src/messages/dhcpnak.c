
#include "dhcpnak.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "../logging.h"
#include "../utils/xtoy.h"
#include "cclog_macros.h"

int message_nak_send(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(68);
        // TODO: make proper broadcast domain
        addr.sin_addr.s_addr = inet_addr("192.168.1.255");

        cclog(LOG_MSG, NULL, "Sending DHCP NAK message");
        if_failed_log_n(sendto(server->sock_fd, &message->packet, sizeof(dhcp_packet_t), 0,
                        (struct sockaddr*)&addr, sizeof(addr)), 
                        exit, LOG_ERROR, NULL, "Failed to send dhcp NAK message: %s", 
                                                strerror(errno));

        rv = 0;              
exit:
        return rv;
}

int message_dhcpnak_build(dhcp_server_t *server, dhcp_message_t *request)
{
        if (!server || !request)
                return -1;

        int rv = -1;

        dhcp_message_t *nak = dhcp_message_new();
        if_null(nak, exit);

        nak->opcode = BOOTREPLY;
        nak->htype = request->htype;
        nak->hlen = request->hlen;
        nak->hops = 0;
        nak->xid = request->xid;
        nak->secs = 0;
        nak->ciaddr = 0;
        nak->yiaddr = 0;
        nak->siaddr = 0;
        nak->flags = request->flags;
        nak->giaddr = request->giaddr;
        memcpy(nak->chaddr, request->chaddr, CHADDR_LEN);
        nak->cookie = request->cookie;
 
        // TODO: add message type dhcp option DHCPNAK
        if_failed(dhcp_packet_build(nak), exit);
        if_failed(message_nak_send(server, nak), exit);
        if_failed(trans_cache_add_message(server->trans_cache, nak), exit);

        rv = 0;
exit:
        return rv;
}


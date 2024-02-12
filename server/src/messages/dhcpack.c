
#include "dhcpack.h"
#include "../utils/xtoy.h"
#include "../logging.h"
#include "../dhcp_options.h"
#include "cclog_macros.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int message_dhcpack_send(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        // TODO: make possible to unicast
        addr.sin_port = htons(68);
        addr.sin_addr.s_addr = inet_addr("192.168.1.255");

        cclog(LOG_MSG, NULL, "Sending dhcp ack message acknownledging address %s",
                        uint32_to_ipv4_address(message->yiaddr));
        // TODO: make proper unicast messaging if applicable
        if_failed_log_n(sendto(server->sock_fd, &message->packet, sizeof(dhcp_packet_t), 0,
                        (struct sockaddr*)&addr, sizeof(addr)), 
                        exit, LOG_ERROR, NULL, "Failed to send dhcp ACK message: %s", 
                                                strerror(errno));

        rv = 0;              
exit:
        return rv;
}

static int get_requested_dhcp_options(address_allocator_t *allocator, dhcp_message_t *dhcp_request,
                uint32_t acked_lease_duration, uint32_t leased_address, dhcp_message_t *dhcp_ack)
{
        if (!allocator || !dhcp_request)
                return -1;

        int rv = -1;

        /* We get requested options */
        dhcp_option_t *requested_options_list = dhcp_option_retrieve(dhcp_request->dhcp_options, 
                                                        DHCP_OPTION_PARAMETER_REQUEST_LIST);
        uint8_t requested_options[256];
        memset(requested_options, 0, 256);
        memcpy(requested_options, requested_options_list->value.binary_data, 
                        requested_options_list->lenght);

        /* Get the pool options of the address */
        address_pool_t *pool = allocator_get_pool_by_address(allocator, leased_address);
        if_null(pool, exit);

        if_failed_log(dhcp_option_build_required_options(dhcp_ack->dhcp_options, requested_options, 
                        /* required */   (uint8_t[]) {51, 54, 0}, 
                        /* blacklised */ (uint8_t[]) {50, 55, 61, 57, 0},
                        allocator->default_options, pool->dhcp_option_override, DHCP_ACK),
                        exit, LOG_WARN, NULL, "Failed to build dhcp options for DHCP_ACK message");

        rv = 0;
exit:
        return rv;
}

int message_dhcpack_build(dhcp_server_t *server, dhcp_message_t *dhcp_request, 
        uint32_t acked_lease_duration, uint32_t leased_address)
{
        if (!server || !dhcp_request) {
                return -1;
        }

        int rv = -1;

        dhcp_message_t *ack = dhcp_message_new();

        ack->opcode = BOOTREPLY;
        ack->htype  = dhcp_request->htype;
        ack->hlen   = dhcp_request->hlen;
        ack->hops   = 0;
        ack->xid    = dhcp_request->xid;
        ack->secs   = 0;
        ack->ciaddr = dhcp_request->ciaddr;
        ack->yiaddr = leased_address;
        // TODO: Make a proper way to get server IP address
        ack->siaddr = ipv4_address_to_uint32("192.168.1.250");
        ack->flags  = dhcp_request->flags;
        ack->giaddr = dhcp_request->giaddr;
        ack->cookie = dhcp_request->cookie;
        memcpy(ack->chaddr, dhcp_request->chaddr, CHADDR_LEN);
        if_failed(get_requested_dhcp_options(server->allocator, dhcp_request, 
                                acked_lease_duration, leased_address, ack), 
                        exit);

        if_failed_log(dhcp_packet_build(ack), exit, LOG_ERROR, NULL, 
                        "Failed to build DHCPACK message");

        if_failed(message_dhcpack_send(server,ack), exit);
        dhcp_message_destroy(&ack);
exit:
        return rv;
}


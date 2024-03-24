#include "dhcpoffer.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../RFC/RFC-2131.h"
#include "../utils/xtoy.h"
#include "../allocator.h"
#include "../utils/llist.h"
#include "../logging.h"
#include "../dhcp_packet.h"
#include <unistd.h>

int message_dhcpoffer_send(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(68);
        addr.sin_addr.s_addr = server->config.broadcast_addr;

        cclog(LOG_MSG, NULL, "Sending DHCP offer message offering address %s to client %s",
                        uint32_to_ipv4_address(message->yiaddr), 
                        uint8_array_to_mac((uint8_t*)message->chaddr));
        /* No need to implement unicast, since dhcpoffer is always broadcasted */
        if_failed_log_n(sendto(server->sock_fd, &message->packet, sizeof(dhcp_packet_t), 0,
                                (struct sockaddr*)&addr, sizeof(addr)), 
                        exit, LOG_ERROR, NULL, "Failed to send dhcp OFFER message: %s", 
                                                strerror(errno));

        rv = 0;              
exit:
        return rv;
}

static int get_requested_dhcp_options(address_allocator_t *allocator, dhcp_message_t *dhcp_discover,
                uint32_t offered_lease_duration, uint32_t offered_address,
                dhcp_message_t *dhcp_offer)
{
        if (!allocator || !dhcp_discover)
                return -1;

        int rv = -1;

        /* We get requested options */
        dhcp_option_t *requested_options_list = dhcp_option_retrieve(dhcp_discover->dhcp_options, 
                                                        DHCP_OPTION_PARAMETER_REQUEST_LIST);
        uint8_t requested_options[256];
        memset(requested_options, 0, 256);
        memcpy(requested_options, requested_options_list->value.binary_data, 
                        requested_options_list->lenght);

        /* Get the pool options of the address */
        address_pool_t *pool = allocator_get_pool_by_address(allocator, offered_address);
        if_null(pool, exit);

        if_failed_log(dhcp_option_build_required_options(dhcp_offer->dhcp_options, requested_options, 
                        /* required */   (uint8_t[]) {51, 54, 0}, 
                        /* blacklised */ (uint8_t[]) {50, 55, 61, 57, 0},
                        allocator->default_options, pool->dhcp_option_override, DHCP_OFFER),
                        exit, LOG_WARN, NULL, "Failed to build dhcp options for DHCP_OFFER message");

        dhcp_option_t *o61_client = dhcp_option_retrieve(dhcp_discover->dhcp_options, DHCP_OPTION_CLIENT_IDENTIFIER);
        if (o61_client) {
                dhcp_option_t *o61 = dhcp_option_new_values(DHCP_OPTION_CLIENT_IDENTIFIER, 
                                                            o61_client->lenght, 
                                                            o61_client->value.binary_data);
                if_failed_log(dhcp_option_add(dhcp_offer->dhcp_options, o61), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 61 during dhcp offer building");
        }

        rv = 0;
exit:
        return rv;
}

int message_dhcpoffer_build(dhcp_server_t *server, dhcp_message_t *dhcp_discover, 
        uint32_t offered_address, uint32_t offered_lease_duration)
{
        if (!server || !dhcp_discover) {
                return -1;
        }

        int rv = -1;

        dhcp_message_t *offer = dhcp_message_new();

        offer->opcode = BOOTREPLY;
        offer->htype  = dhcp_discover->htype;
        offer->hlen   = dhcp_discover->hlen;
        offer->hops   = 0;
        offer->xid    = dhcp_discover->xid;
        offer->secs   = 0;
        offer->ciaddr = 0;
        offer->yiaddr = offered_address;
        offer->siaddr = ntohl(server->config.bound_ip);
        offer->flags  = dhcp_discover->flags;
        offer->giaddr = dhcp_discover->giaddr;
        offer->cookie = dhcp_discover->cookie;
        offer->type   = DHCP_OFFER;
        memcpy(offer->chaddr, dhcp_discover->chaddr, CHADDR_LEN);
        if_failed(get_requested_dhcp_options(server->allocator, dhcp_discover, 
                                offered_lease_duration, offered_address, offer), 
                        exit);

        if_failed(dhcp_packet_build(offer), exit);
        if_failed(message_dhcpoffer_send(server,offer), exit);
        if_failed(trans_cache_add_message(server->trans_cache, offer), exit);

        rv = 0;
exit:
        return rv;
}


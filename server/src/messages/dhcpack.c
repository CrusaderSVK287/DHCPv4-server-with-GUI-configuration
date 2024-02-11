
#include "dhcpack.h"
#include "../utils/xtoy.h"
#include "../logging.h"
#include "../dhcp_options.h"
#include <errno.h>
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


// TODO: Make this function generic, move it to dhcp_options API, do like and array and must be 
// options and must not options for the requested options (e.g. no lease time if response to DHCPINFORM)
static int get_requested_dhcp_options(address_allocator_t *allocator, dhcp_message_t *dhcp_request,
                uint32_t acked_lease_duration, dhcp_message_t *dhcp_ack)
{
        if (!allocator || !dhcp_request)
                return -1;

        int rv = -1;

        uint8_t val = DHCP_ACK;
        dhcp_option_t *o53 = dhcp_option_new_values(DHCP_OPTION_DHCP_MESSAGE_TYPE, 1, &val);
        if_failed_log(dhcp_option_add(dhcp_ack->dhcp_options, o53), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 53 during dhcp ack building");
        
        dhcp_option_t *o51 = dhcp_option_new_values(DHCP_OPTION_IP_ADDRESS_LEASE_TIME, 4, 
                                                        &acked_lease_duration);
        if_failed_log(dhcp_option_add(dhcp_ack->dhcp_options, o51), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 51 during dhcp ack building");

        // TODO: get proper IP address
        val = ipv4_address_to_uint32("192.168.1.250");
        dhcp_option_t *o54 = dhcp_option_new_values(DHCP_OPTION_SERVER_IDENTIFIER, 4, &val);
        if_failed_log(dhcp_option_add(dhcp_ack->dhcp_options, o54), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 54 during dhcp ack building");
        
        dhcp_option_t *o61_client = dhcp_option_retrieve(dhcp_request->dhcp_options, DHCP_OPTION_CLIENT_IDENTIFIER);
        if (o61_client) {
                dhcp_option_t *o61 = dhcp_option_new_values(DHCP_OPTION_CLIENT_IDENTIFIER, 
                                o61_client->lenght, o61_client->value.binary_data);
                if_failed_log(dhcp_option_add(dhcp_request->dhcp_options, o61), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 61 during dhcp offer building");
        }

        dhcp_option_t *requested_options = dhcp_option_retrieve(dhcp_request->dhcp_options, 
                                                        DHCP_OPTION_PARAMETER_REQUEST_LIST);
        
        uint32_t tag = 0;
        dhcp_option_t *option;
        for (int i = 0; i < requested_options->lenght; i++) {
                tag = requested_options->value.binary_data[i];

                /* RFC-2131 specifies theese options MUST NOT be used in ack */
                if (    tag == DHCP_OPTION_REQUESTED_IP_ADDRESS     ||
                        tag == DHCP_OPTION_PARAMETER_REQUEST_LIST   ||
                        tag == DHCP_OPTION_CLIENT_IDENTIFIER        ||
                        tag == DHCP_OPTION_MAX_DHCP_MESSAGE_SIZE)
                        continue;

                option = dhcp_option_retrieve(allocator->default_options, tag);

                if (!option)
                        continue;

                if_failed_log_ng(dhcp_option_add(dhcp_ack->dhcp_options, option), LOG_ERROR, NULL,
                        "Failed to add dhcp option %d during dhcp ack building", tag);
        }

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
                                acked_lease_duration, ack), 
                        exit);

        if_failed_log(dhcp_packet_build(ack), exit, LOG_ERROR, NULL, 
                        "Failed to build DHCPACK message");

        if_failed(message_dhcpack_send(server,ack), exit);
        dhcp_message_destroy(&ack);
exit:
        return rv;
}


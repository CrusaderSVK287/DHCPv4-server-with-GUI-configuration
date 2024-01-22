
#include "dhcpack.h"
#include "../utils/xtoy.h"
#include "../logging.h"
#include "../dhcp_options.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int message_dhcpack_send(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;

        // TODO: make proper unicast messaging if applicable
        if_failed_log_n(send(server->sock_fd, &message->packet, sizeof(dhcp_packet_t), 0), 
                        exit, LOG_ERROR, NULL, "Failed to send dhcp OFFER message");

        rv = 0;              
exit:
        return rv;
}

static int get_requested_dhcp_options(address_allocator_t *allocator, dhcp_message_t *dhcp_request,
                uint32_t offered_lease_duration, dhcp_message_t *dhcp_ack)
{
        if (!allocator || !dhcp_request)
                return -1;

        int rv = -1;

        dhcp_option_t *o53 = dhcp_option_new();
        o53->tag = DHCP_OPTION_DHCP_MESSAGE_TYPE;
        o53->type = DHCP_OPTION_NUMERIC;
        o53->lenght = 1;
        o53->value.number = DHCP_ACK;

        if_failed_log(dhcp_option_add(dhcp_ack->dhcp_options, o53), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 53 during dhcp ack building");
        
        dhcp_option_t *o51 = dhcp_option_new();
        o51->tag = DHCP_OPTION_IP_ADDRESS_LEASE_TIME;
        o51->type = DHCP_OPTION_NUMERIC;
        o51->lenght = 4;
        o51->value.number = offered_lease_duration;

        if_failed_log(dhcp_option_add(dhcp_ack->dhcp_options, o51), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 51 during dhcp ack building");

        dhcp_option_t *o54 = dhcp_option_new();
        o54->tag = DHCP_OPTION_SERVER_IDENTIFIER;
        o54->type = DHCP_OPTION_IP;
        o54->lenght = 4;
        // TODO: get proper IP address
        o54->value.ip = ipv4_address_to_uint32("192.168.0.250");
     
        if_failed_log(dhcp_option_add(dhcp_ack->dhcp_options, o54), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 54 during dhcp ack building");
        
        dhcp_option_t *requested_options = dhcp_option_retrieve(dhcp_request->dhcp_options, 
                                                        DHCP_OPTION_PARAMETER_REQUEST_LIST);
        
        uint32_t tag = 0;
        dhcp_option_t *option;
        for (int i = 0; i < requested_options->lenght; i++) {
                tag = requested_options->value.binary_data[i];

                /* RFC-2131 specifies theese options MUST NOT be used in offer */
                if (    tag == DHCP_OPTION_REQUESTED_IP_ADDRESS     ||
                        tag == DHCP_OPTION_PARAMETER_REQUEST_LIST   ||
                        tag == DHCP_OPTION_CLIENT_IDENTIFIER        ||
                        tag == DHCP_OPTION_MAX_DHCP_MESSAGE_SIZE)
                        continue;

                option = dhcp_option_retrieve(allocator->default_options, tag);

                if (!option)
                        continue;

                if_failed_log_ng(dhcp_option_add(dhcp_ack->dhcp_options, option), LOG_ERROR, NULL,
                        "Failed to add dhcp option %d during dhcp offer building", tag);
        }

        rv = 0;
exit:
        return rv;
}

int message_dhcpack_build(dhcp_server_t *server, dhcp_message_t *dhcp_request, 
        uint32_t offered_lease_duration)
{
        if (!server || !dhcp_request) {
                return -1;
        }

        int rv = -1;

        dhcp_message_t *offer = dhcp_message_new();

        offer->opcode = BOOTREPLY;
        offer->htype  = dhcp_request->htype;
        offer->hlen   = dhcp_request->hlen;
        offer->hops   = 0;
        offer->xid    = dhcp_request->xid;
        offer->secs   = 0;
        offer->ciaddr = dhcp_request->ciaddr;
        offer->yiaddr = dhcp_request->yiaddr;
        // TODO: Make a proper way to get server IP address
        offer->siaddr = ipv4_address_to_uint32("192.168.0.250");
        offer->flags  = dhcp_request->flags;
        offer->giaddr = dhcp_request->giaddr;
        memcpy(offer->chaddr, dhcp_request->chaddr, CHADDR_LEN);
        if_failed(get_requested_dhcp_options(server->allocator, dhcp_request, 
                                offered_lease_duration, offer), 
                        exit);

        if_failed_log(dhcp_packet_build(offer), exit, LOG_ERROR, NULL, 
                        "Failed to build DHCPOFFER message");

        if_failed(message_dhcpack_send(server,offer), exit);
        dhcp_message_destroy(&offer);
exit:
        return rv;
}


#include "dhcpack.h"
#include "../utils/xtoy.h"
#include "../logging.h"
#include "../dhcp_options.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>

int message_dhcpack_send(dhcp_server_t *server, dhcp_message_t *message, const char *reason)
{
        if (!server || !message)
                return -1;

        int rv = -1;

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(68);
        // TODO: Replace inet_addr with inet_aton etc. also retrieve proper broadcast address if applicable
        addr.sin_addr.s_addr = (message->ciaddr) ? htonl(message->ciaddr) : inet_addr("192.168.1.255");

        int fd = open("./test/ack.bin", O_RDWR | O_TRUNC | O_CREAT, 0666);
        rv = write(fd, &message->packet, sizeof(dhcp_packet_t));
        close(fd);
        if (rv < 0) printf("ERROR writing %s\n", strerror(errno));
        
        cclog(LOG_MSG, NULL, "Sending dhcp ack message %s address %s to %s",
                        reason, uint32_to_ipv4_address(message->yiaddr), uint32_to_ipv4_address(addr.sin_addr.s_addr));
        if_failed_log_n(sendto(server->sock_fd, &message->packet, sizeof(dhcp_packet_t), 0,
                        (struct sockaddr*)&addr, sizeof(addr)), 
                        exit, LOG_ERROR, NULL, "Failed to send dhcp ACK message: %s", 
                                                strerror(errno));

        rv = 0;              
exit:
        return rv;
}

static int get_requested_dhcp_options(address_allocator_t *allocator, dhcp_message_t *dhcp_request,
                uint32_t leased_address, dhcp_message_t *dhcp_ack)
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

// static int get_requested_dhcp_options_lease_renewal(address_allocator_t *allocator,
//                 dhcp_message_t *dhcp_ack)
// {
//         if (!allocator)
//                 return -1;
//
//         int rv = -1;
//
//         /*
//          * Since we are acknownledging already existing lease with configured client, 
//          * we make a dummy parameter request list 
//          */
//         dhcp_option_t *requested_options_list = dhcp_option_new_values(DHCP_OPTION_PARAMETER_REQUEST_LIST,
//                                                                         1, (uint8_t[]) {0});
//
//         /* Get the pool options of the address */
//         address_pool_t *pool = allocator_get_pool_by_address(allocator, dhcp_ack->ciaddr);
//         if_null(pool, exit);
//
//         if_failed_log(dhcp_option_build_required_options(dhcp_ack->dhcp_options, 
//                         requested_options_list->value.binary_data, 
//                         /* required */   (uint8_t[]) {DHCP_OPTION_IP_ADDRESS_LEASE_TIME, 0}, 
//                         /* blacklised */ (uint8_t[]) {0},
//                         allocator->default_options, pool->dhcp_option_override, DHCP_ACK),
//                         exit, LOG_WARN, NULL, "Failed to build dhcp options for DHCP_ACK message");
//
//         rv = 0;
// exit:
//         return rv;
// }

static int get_requested_dhcp_options_inform_response(address_allocator_t *allocator, 
                dhcp_message_t *dhcp_inform, dhcp_message_t *dhcp_ack)
{
        if (!allocator || !dhcp_inform)
                return -1;

        int rv = -1;

        /* We get requested options */
        dhcp_option_t *requested_options_list = dhcp_option_retrieve(dhcp_inform->dhcp_options, 
                                                        DHCP_OPTION_PARAMETER_REQUEST_LIST);
        uint8_t requested_options[256];
        memset(requested_options, 0, 256);
        memcpy(requested_options, requested_options_list->value.binary_data, 
                        requested_options_list->lenght);

        /* Get the pool options of the address */
        address_pool_t *pool = allocator_get_pool_by_address(allocator, dhcp_inform->ciaddr);
        if_null(pool, exit);

        if_failed_log(dhcp_option_build_required_options(dhcp_ack->dhcp_options, requested_options, 
                        /* required */   (uint8_t[]) {54, 0}, 
                        /* blacklised */ (uint8_t[]) {50, 51, 55, 61, 57, 0},
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
                                 leased_address, ack), 
                        exit);

        if_failed(dhcp_packet_build(ack), exit);
        if_failed(message_dhcpack_send(server, ack, "acknownledging new lease of"), exit);
        if_failed(trans_cache_add_message(server->trans_cache, ack), exit);

        rv = 0;
exit:
        return rv;
}

int message_dhcpack_build_lease_renew(dhcp_server_t *server, dhcp_message_t *request)
{
        if (!server || !request)
                return -1;

        int rv = -1;

        dhcp_message_t *ack = dhcp_message_new();

        ack->opcode = BOOTREPLY;
        ack->htype  = request->htype;
        ack->hlen   = request->hlen;
        ack->hops   = 0;
        ack->xid    = request->xid;
        ack->secs   = 0;
        ack->ciaddr = request->ciaddr;
        ack->yiaddr = request->ciaddr;
        // TODO: Make a proper way to get server IP address
        ack->siaddr = ipv4_address_to_uint32("192.168.1.250");
        ack->flags  = request->flags;
        ack->giaddr = 0;
        ack->cookie = request->cookie;
        memcpy(ack->chaddr, request->chaddr, CHADDR_LEN);
        if_failed(get_requested_dhcp_options(server->allocator, request, ack->ciaddr, ack), exit);

        if_failed(dhcp_packet_build(ack), exit);
        if_failed(message_dhcpack_send(server,ack, "renewing lease of"), exit);
        if_failed(trans_cache_add_message(server->trans_cache, ack), exit);

        rv = 0;
exit:
        return rv;
}

int message_dhcpack_build_inform_response(dhcp_server_t *server, dhcp_message_t *inform)
{
        if (!server || !inform)
                return -1;

        int rv = -1;

        /* If client didnt provide its IP address, dhcpinform is invalid */
        if_false(inform->ciaddr, exit);

        dhcp_message_t *ack = dhcp_message_new();

        ack->opcode = BOOTREPLY;
        ack->htype  = inform->htype;
        ack->hlen   = inform->hlen;
        ack->hops   = 0;
        ack->xid    = inform->xid;
        ack->secs   = 0;
        ack->ciaddr = inform->ciaddr;
        ack->yiaddr = 0;
        // TODO: Make a proper way to get server IP address
        ack->siaddr = ipv4_address_to_uint32("192.168.1.250");
        ack->flags  = inform->flags;
        ack->giaddr = inform->giaddr;
        ack->cookie = inform->cookie;
        memcpy(ack->chaddr, inform->chaddr, CHADDR_LEN);
        if_failed(get_requested_dhcp_options_inform_response(server->allocator, inform, ack),
                        exit);

        if_failed(dhcp_packet_build(ack), exit);
        if_failed(message_dhcpack_send(server,ack, "informing client on"), exit);
        if_failed(trans_cache_add_message(server->trans_cache, ack), exit);

        rv = 0;
exit:
        return rv;
}


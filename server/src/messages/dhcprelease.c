
#include "dhcprelease.h"
#include "../allocator.h"
#include "../logging.h"
#include "../lease.h"
#include "../utils/xtoy.h"

int message_dhcprelease_handle(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;
 
        if (message->ciaddr == 0) {
                cclog(LOG_INFO, NULL, "Received DHCPRELEASE from %s but ciaddr is 0", 
                                uint8_array_to_mac((uint8_t*)message->chaddr));
                goto exit;
        }

        lease_t lease = {0};
        if_failed_log(
                lease_retrieve_address(&lease, message->ciaddr, server->allocator->address_pools),
                exit, LOG_WARN, NULL, 
                "Received DHCPRELEASE on address %s from %s but such lease doesnt exist",
                        uint32_to_ipv4_address(message->ciaddr), 
                        uint8_array_to_mac((uint8_t*)message->chaddr));

        if_failed_log(lease_remove(&lease), exit, LOG_ERROR, NULL, 
                        "Failed to remove lease of address %s from pool %s",
                        uint32_to_ipv4_address(lease.address), lease.pool_name);
        if_failed_log(allocator_release_address(server->allocator, lease.address), exit, LOG_ERROR, 
                        NULL, "Failed to release address %s", uint32_to_ipv4_address(lease.address));

        rv = 0;
exit:
        return rv;
}


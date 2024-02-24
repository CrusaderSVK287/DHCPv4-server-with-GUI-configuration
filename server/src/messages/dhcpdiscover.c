#include "dhcpdiscover.h"
#include "dhcpoffer.h"
#include <locale.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include "../dhcp_options.h"
#include "../logging.h"
#include "cclog.h"
#include "../allocator.h"
#include "../utils/xtoy.h"

static uint32_t allocate_particular_address(dhcp_server_t *server, 
                dhcp_message_t *msg)
{
        if (!server || !msg)
                return 0;

        int rv = ALLOCATOR_ERROR;
        uint32_t address = 0;

        dhcp_option_t *o50 = dhcp_option_retrieve(msg->dhcp_options, 
                                                  DHCP_OPTION_REQUESTED_IP_ADDRESS);
        if_null(o50, exit);
        
        rv = allocator_request_this_address(server->allocator, o50->value.ip, &address);
        if_failed_log_n_ng(rv, LOG_WARN, NULL, "Error allocating address %s: %s", 
                                uint32_to_ipv4_address(o50->value.ip), allocator_strerror(rv));
        
exit:
        return address;
}

static uint32_t allocate_random_address(address_allocator_t *allocator)
{
        if (!allocator)
                return 0;

        int rv = ALLOCATOR_ERROR;
        uint32_t address = 0;

        rv = allocator_request_any_address(allocator, &address);
        if_failed_log_n_ng(rv, LOG_WARN, NULL, "Failed to allocate any address: %s",
                        allocator_strerror(rv));

        return address;
}

static uint32_t get_lease_duration(address_allocator_t *allocator, dhcp_message_t *msg)
{
        if (!allocator || !msg)
                return 0;

        uint32_t lease_time = 0;
        dhcp_option_t *o51_requested = dhcp_option_retrieve(msg->dhcp_options, 
                                                  DHCP_OPTION_IP_ADDRESS_LEASE_TIME);
        dhcp_option_t *o51_global    = dhcp_option_retrieve(allocator->default_options,
                                                  DHCP_OPTION_IP_ADDRESS_LEASE_TIME);

        if_null_log(o51_global, exit, LOG_ERROR, NULL, "Option 51 (Ip address lease time) is "
                                "not specified globaly, cannot proceed with configuration");

        if (o51_requested) {
                if (o51_requested->value.number <= o51_global->value.number)
                        lease_time = o51_requested->value.number;
                else
                        lease_time = o51_global->value.number;
        } else {
                lease_time = o51_global->value.number;
        }

exit:
        return lease_time;
}

int message_dhcpdiscover_handle(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;
        uint32_t new_address = 0;
        uint32_t lease_time = 0;

        if (dhcp_option_retrieve(message->dhcp_options, 
                                        DHCP_OPTION_REQUESTED_IP_ADDRESS)) {
                new_address = allocate_particular_address(server, message);
        }
        
        if (new_address == 0) {
                new_address = allocate_random_address(server->allocator);
        }

        /* Check whether we still dont have IP address. Errors are logged in allocation functions */
        if (new_address == 0)
                goto exit;

        /* Get lease duration and check if we got it */
        lease_time = get_lease_duration(server->allocator, message);
        if (lease_time == 0)
                goto exit;

        if_failed(message_dhcpoffer_build(server, message, new_address, lease_time), exit);
        
        rv = 0;
exit:
        return rv;
}




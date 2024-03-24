#include "init.h"
#include "RFC/RFC-2132.h"
#include "address_pool.h"
#include "allocator.h"
#include "dhcp_options.h"
#include "logging.h"
#include "transaction_cache.h"
#include "utils/xtoy.h"
#include <stdint.h>

/**
 * Temporary solution until proper configuration API is made
 */
int init_allocator(dhcp_server_t *server)
{
        if_null(server, error);

        server->allocator = address_allocator_new();
        if_null(server->allocator, error);
        
        return 0;
error:
        cclog(LOG_CRITICAL, NULL, "Failed to initialise allocator");
        return -1;
}

int init_address_pools(dhcp_server_t *server)
{
        if_null(server, error);
        if_null(server->allocator, error);

        // address_pool_t *p = address_pool_new_str("LAN", "192.168.1.1",
                                                        // "192.168.1.254",
                                                        // "255.255.255.0");
        // if_null(p, error);
        // if_failed(allocator_add_pool(server->allocator, p), error);
        
        return 0;
error:
        cclog(LOG_CRITICAL, NULL, "Failed to initialise address pools");
        return -1;
}

int init_dhcp_options(dhcp_server_t *server)
{
        if_null(server, error);
        if_null(server->allocator, error);
 
        // TODO: server identifier and subnet mask removed
        
        uint32_t lease_time = 60;
        if_failed(dhcp_option_add(server->allocator->default_options, dhcp_option_new_values(
                                        DHCP_OPTION_IP_ADDRESS_LEASE_TIME, 4, &lease_time)), error);
 
        return 0;
error:
        cclog(LOG_CRITICAL, NULL, "Failed to initialise dhcp options");
        return -1;
}

int init_cache(dhcp_server_t *server)
{
        if_null(server, error);
        
        // TODO: check after config
        server->trans_cache = trans_cache_new(TRANSACTION_CACHE_DEFAULT_SIZE);
        if_null(server->trans_cache, error);

        return 0;
error:
        cclog(LOG_CRITICAL, NULL, "Failed to initialise transaction cache");
        return -1;
}

#include "init.h"
#include "RFC/RFC-2132.h"
#include "allocator.h"
#include "dhcp_options.h"
#include "logging.h"
#include "transaction_cache.h"
#include <stdint.h>
#include "config.h"
#include "security/acl.h"

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

int init_dhcp_options(dhcp_server_t *server)
{
        if_null(server, error);
        if_null(server->allocator, error);
 
        uint32_t lease_time = server->config.lease_time;
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
        
        server->trans_cache = trans_cache_new(server->config.cache_size,
                                              server->config.trans_duration);
        if_null(server->trans_cache, error);

        return 0;
error:
        cclog(LOG_CRITICAL, NULL, "Failed to initialise transaction cache");
        return -1;
}

int ACL_init(dhcp_server_t *server)
{
        if (!server)
                return -1;

        if (server->config.acl_enable == CONFIG_UNTOUCHED || 
            server->config.acl_blacklist == CONFIG_UNTOUCHED) {
                cclog(LOG_CRITICAL, NULL, "ACL config not set, please contact the developer");
                goto error;
        }
        
        server->acl = ACL_new();
        if_null(server->acl, error);

        server->acl->enabled = server->config.acl_enable;
        server->acl->is_blacklist = server->config.acl_blacklist;

        if (ACL_load_acl_entries(server->acl, server->config.config_path) < 0) {
                goto error;
        }

        return 0;
error:
        cclog(LOG_CRITICAL, NULL, "Failed to initialise acl");
        return -1;
}


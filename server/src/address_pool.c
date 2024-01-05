#include "address_pool.h"
#include "RFC/RFC-2132.h"
#include "logging.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include "dhcp_options.h"
#include <cclog.h>
#include <cclog_macros.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void log_with_pool_info(int log_level, const char *format, 
                uint32_t start, uint32_t end, uint32_t mask)
{
        char *start_addr = strdup(uint32_to_ipv4_address(start));
        char *end_addr = strdup(uint32_to_ipv4_address(end));
        char *subnet_mask = strdup(uint32_to_ipv4_address(mask));

        cclog(log_level, NULL, format, start_addr, end_addr, subnet_mask);

        free(start_addr);
        free(end_addr);
        free(subnet_mask);
}

static bool can_range_be_on_subnet(uint32_t start, uint32_t end, uint32_t mask)
{
        uint32_t subnet_start = start & mask;
        uint32_t subnet_end = subnet_start + (~mask);

        /* +1 and -1 to accomodate network address and broacast address respectively */
        bool result = (start >= subnet_start + 1 && end <= subnet_end - 1);
        if_false(result, error);

        return true;

error:
        log_with_pool_info(LOG_ERROR, "Invalid start and end addresses in pool %s - %s on subnet %s",
                        start, end, mask);
        return false;
}

address_pool_t* address_pool_new(const char *name, uint32_t start_address,
        uint32_t end_address, uint32_t subnet_mask)
{
        if_null(name, error);
        if_false(can_range_be_on_subnet(start_address, end_address, subnet_mask), error);

        address_pool_t *pool = calloc(1, sizeof(address_pool_t));
        if_null(pool, error);

        pool->start_address = start_address;
        pool->end_address = end_address;
        pool->name = name;
        pool->mask = subnet_mask;
        pool->dhcp_option_override = llist_new();

        dhcp_option_t *opt_subnet_mask = dhcp_option_new();
        if (!opt_subnet_mask) {
                free(pool);
                goto error;
        }

        opt_subnet_mask->tag = DHCP_OPTION_SUBNET_MASK;
        opt_subnet_mask->type = DHCP_OPTION_IP;
        opt_subnet_mask->lenght = 4;
        opt_subnet_mask->value.ip = subnet_mask;

        if_failed(dhcp_option_add(pool->dhcp_option_override, opt_subnet_mask), error);

        log_with_pool_info(LOG_MSG, "Created new address pool from %s to %s on subnet %s",
                        start_address, end_address, subnet_mask);
        return pool;
error:
        return NULL;
}

address_pool_t* address_pool_new_str(const char *name, const char *start_address,
        const char *end_address, const char *subnet_mask)
{
        return address_pool_new(name, ipv4_address_to_uint32(start_address), 
                                      ipv4_address_to_uint32(end_address), 
                                      ipv4_address_to_uint32(subnet_mask));
}

void address_pool_destroy(address_pool_t **pool)
{
        if (!pool || !(*pool))
                return;

        free(*pool);
        *pool = NULL;
}

bool address_belongs_to_pool(address_pool_t *pool, uint32_t address)
{
        return pool->start_address <= address && address <= pool->end_address;
}

bool address_belongs_to_pool_str(address_pool_t *pool, const char *address)
{
        return address_belongs_to_pool(pool, ipv4_address_to_uint32(address));
}


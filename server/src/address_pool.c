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

static uint32_t pool_range(uint32_t start_addr, uint32_t end_addr)
{
        return (end_addr - start_addr + 8) / 8;
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
        if_false((start_address < end_address), error);

        address_pool_t *pool = calloc(1, sizeof(address_pool_t));
        if_null(pool, error);

        pool->start_address = start_address;
        pool->end_address = end_address;
        pool->name = name;
        pool->mask = subnet_mask;
        pool->dhcp_option_override = llist_new();
        if_null(pool->dhcp_option_override, error_options);

        dhcp_option_t *opt_subnet_mask = dhcp_option_new();
        if_null(opt_subnet_mask, error_options);

        opt_subnet_mask->tag = DHCP_OPTION_SUBNET_MASK;
        opt_subnet_mask->type = DHCP_OPTION_IP;
        opt_subnet_mask->lenght = 4;
        opt_subnet_mask->value.ip = subnet_mask;

        if_failed(dhcp_option_add(pool->dhcp_option_override, opt_subnet_mask), error);

        pool->leases_bm = calloc(pool_range(start_address, end_address), sizeof(uint8_t));
        if_null(pool->leases_bm, error_leases);
        pool->available_addresses = end_address - start_address + 1;

        log_with_pool_info(LOG_MSG, "Created new address pool from %s to %s on subnet %s",
                        start_address, end_address, subnet_mask);
        return pool;

error_leases:
        llist_destroy(&pool->dhcp_option_override);
error_options:
        free(pool);
error:
        log_with_pool_info(LOG_ERROR, "Cannot create pool with range %s to %s on subnet %s",
                        start_address, end_address, subnet_mask);
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

        llist_destroy(&(*pool)->dhcp_option_override);
        free((*pool)->leases_bm);
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

int address_pool_address_allocation_ctl(address_pool_t *pool, uint32_t address, int action)
{
        int rv = -1;
        if_null(pool, exit);
        if_false(address_belongs_to_pool(pool, address), exit);

        uint32_t address_number = address - pool->start_address;
        uint32_t index = address_number / 8;
        uint8_t bit = address_number % 8;
        uint8_t address_bit = (pool->leases_bm[index] & (uint8_t)(1 << bit)) != 0;

        switch (action) {
        case 's':   // set address alocation
                if_true(address_bit, exit);

                pool->leases_bm[index] |= (uint8_t)(1 << bit);
                pool->available_addresses -= 1;
                rv = 0;
                break;
        case 'c':   // clear address allocation
                if_false(address_bit, exit);

                pool->leases_bm[index] -= (uint8_t)(1 << bit);
                pool->available_addresses += 1;
                rv = 0;
                break;
        case 'g':   // get address allocation
                rv = address_bit;
                break;
        default:
                cclog(LOG_ERROR, NULL, "Invalid action \'%c\' for address allocation ctl", action);
        }

exit:
        return rv;
}

int address_pool_set_address_allocation(address_pool_t *pool, uint32_t address)
{
        return address_pool_address_allocation_ctl(pool, address, 's');
}

int address_pool_set_address_allocation_str(address_pool_t *pool, const char *address)
{
        return address_pool_address_allocation_ctl(pool, ipv4_address_to_uint32(address), 's');
}

int address_pool_get_address_allocation(address_pool_t *pool, uint32_t address)
{
        return address_pool_address_allocation_ctl(pool, address, 'g');
}

int address_pool_get_address_allocation_str(address_pool_t *pool, const char *address)
{
        return address_pool_address_allocation_ctl(pool, ipv4_address_to_uint32(address), 'g');
}

int address_pool_clear_address_allocation(address_pool_t *pool, uint32_t address)
{
        return address_pool_address_allocation_ctl(pool, address, 'c');
}

int address_pool_clear_address_allocation_str(address_pool_t *pool, const char *address)
{
        return address_pool_address_allocation_ctl(pool, ipv4_address_to_uint32(address), 'c');
}


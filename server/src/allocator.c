#include "allocator.h"
#include "address_pool.h"
#include "dhcp_options.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include "logging.h"
#include <alloca.h>
#include <cclog.h>
#include <cclog_macros.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char* allocator_strerror(enum allocator_status s)
{
        switch (s) {
                case ALLOCATOR_OK: return "OK";
                case ALLOCATOR_ERROR: return "Error";
                case ALLOCATOR_POOL_DUPLICITE: return "Pool already exists";
                case ALLOCATOR_OPTION_DUPLICITE: return "Option already exists";
                case ALLOCATOR_ADDR_IN_USE: return "Address in use";
                case ALLOCATOR_ADDR_NOT_IN_USE: return "Address not in use";
                case ALLOCATOR_ADDR_RESERVED: return "Address reserved";
                case ALLOCATOR_OPTION_INVALID: return "Option invalid";
                default: return "Uknown error code";
        }
}

address_allocator_t* address_allocator_new()
{
        address_allocator_t *a = calloc(1, sizeof(address_allocator_t));
        if_null(a, error_all);

        a->address_pools = llist_new();
        if_null(a->address_pools, error_pools);
        a->default_options = llist_new();
        if_null(a->default_options, error_options);

        return a;

error_options:
        llist_destroy(&(*a).address_pools);
error_pools:
        free(a);
error_all:
        return NULL;
}

void allocator_destroy(address_allocator_t **a)
{
        if (!(*a))
                return;

        dhcp_option_destroy_list(&(*a)->default_options);
        
        address_pool_t *ap;
        llist_foreach((*a)->address_pools, {
                ap = (address_pool_t*) node->data;
                address_pool_destroy(&ap);
        })
        llist_destroy(&(*a)->address_pools);

        free(*a);
        *a = NULL;
}

address_pool_t* allocator_get_pool_by_address(address_allocator_t *a, uint32_t addr)
{
        if_null(a, exit);

        address_pool_t *pool = NULL;
        llist_foreach(a->address_pools, {
                pool = (address_pool_t*)node->data;

                if (address_belongs_to_pool(pool, addr))
                        return pool;
        })

exit:
        return NULL;
}

/* Returns pool from list of pools by name. If the pool doesnt exist, returns NULL */
address_pool_t* allocator_get_pool_by_name(address_allocator_t* a, const char* name)
{
        if_null(a, exit);
        if_null(name, exit);

        address_pool_t *pool;

        llist_foreach(a->address_pools, {
                pool = (address_pool_t*)node->data;
                if (!strcmp(pool->name, name))
                        return node->data;
        })

exit:
        return NULL;
}

int allocator_add_pool(address_allocator_t *allocator, address_pool_t *pool)
{
        if (!allocator || !pool)
                return ALLOCATOR_ERROR;

        int rv = ALLOCATOR_ERROR;
        
        /* Check for pool duplicite */
        if (allocator_get_pool_by_name(allocator, pool->name)) {
                rv = ALLOCATOR_POOL_DUPLICITE;
                goto error;
        }

        if_failed(llist_append(allocator->address_pools, pool, false), error);

        rv = ALLOCATOR_OK;
error:
        if_failed_log_ng(rv, LOG_ERROR, NULL, "Error creating pool %s:(%d) %s", 
                        pool->name, rv, allocator_strerror(rv));
        return rv;
}


static int allocator_assign_first_from_pool(address_pool_t *pool, uint32_t *addr_buf)
{
        for (uint32_t a = pool->start_address; a <= pool->end_address; a++) {
                if (address_pool_get_address_allocation(pool, a) == 0) {
                        address_pool_set_address_allocation(pool, a);
                        *addr_buf = a;
                        return ALLOCATOR_OK;
                }
        }

        return ALLOCATOR_POOL_DEPLETED;
}

int allocator_request_any_address(address_allocator_t *allocator, uint32_t *addr_buf)
{
        int rv = ALLOCATOR_ERROR;
        if_null(allocator, exit);
        if_null(addr_buf, exit);

        address_pool_t *pool = NULL;

        llist_foreach(allocator->address_pools, {
                pool = (address_pool_t*)node->data;

                if (pool->available_addresses == 0) {
                        if (node == allocator->address_pools->last) {
                                rv = ALLOCATOR_POOL_DEPLETED;
                                break;
                        }
                        continue;
                }

                if(allocator_assign_first_from_pool(pool, addr_buf) == 0) {
                        break;
                }
        })
        
        rv = ALLOCATOR_OK;
exit:
        return rv;
}

int allocator_request_address_from_pool(address_allocator_t *allocator,
        const char *pool_name, uint32_t *addr_buf)
{
        int rv = ALLOCATOR_ERROR;
        if_null(allocator, exit);
        if_null(pool_name, exit);
        if_null(addr_buf, exit);

        address_pool_t *p = allocator_get_pool_by_name(allocator, pool_name);
        if_null_log(p, exit, LOG_WARN, NULL, 
                        "Pool named %s wasnt found, make sure it exists", pool_name);

        rv = allocator_assign_first_from_pool(p, addr_buf);

exit:
        return rv;
}

int allocator_request_this_address(address_allocator_t *allocator,
        uint32_t requested_addres, uint32_t *addr_buf)
{
        int rv = ALLOCATOR_ERROR;
        if_null(allocator, exit);
        if_null(addr_buf, exit);

        address_pool_t *p = allocator_get_pool_by_address(allocator, requested_addres);
        if_null_log(p, exit, LOG_WARN, NULL, 
                        "Pool containing address %s was not found, make sure it exists",
                        uint32_to_ipv4_address(requested_addres));

        if (address_pool_get_address_allocation(p, requested_addres) == 1) {
                rv = ALLOCATOR_ADDR_IN_USE;
                goto exit;
        }

        if_failed_log(address_pool_set_address_allocation(p, requested_addres), 
                exit, LOG_ERROR, NULL, "Failed to set address allocation for address %s",
                uint32_to_ipv4_address(requested_addres));

        *addr_buf = requested_addres;
        rv = ALLOCATOR_OK;
exit:
        return rv;
}

int allocator_request_this_address_str(address_allocator_t *allocator,
        const char *requested_addres, uint32_t *addr_buf)
{
        return allocator_request_this_address(allocator, 
                        ipv4_address_to_uint32(requested_addres), addr_buf);
}

int allocator_release_address(address_allocator_t *allocator, uint32_t address)
{
        int rv = ALLOCATOR_ERROR;
        if_null(allocator, exit);

        address_pool_t *p = allocator_get_pool_by_address(allocator, address);
        if_null_log(p, exit, LOG_WARN, NULL, 
                        "Pool containing address %s was not found, make sure it exists",
                        uint32_to_ipv4_address(address));

        if (address_pool_get_address_allocation(p, address) == 0) {
                rv = ALLOCATOR_ADDR_NOT_IN_USE;
                goto exit;
        }

        if_failed_log(address_pool_clear_address_allocation(p, address), 
                exit, LOG_ERROR, NULL, "Failed to clear address allocation for address %s",
                uint32_to_ipv4_address(address));

        rv = ALLOCATOR_OK;
exit:
        return rv;
}

int allocator_release_address_str(address_allocator_t *allocator, const char *address)
{
        return allocator_release_address(allocator, ipv4_address_to_uint32(address));
}

int allocator_add_dhcp_option(address_allocator_t *allocator, dhcp_option_t *option)
{
        int rv = ALLOCATOR_ERROR;
        if_null(allocator, exit);
        if_null(option, exit);

        if (dhcp_option_retrieve(allocator->default_options, option->tag)) {
                rv = ALLOCATOR_OPTION_DUPLICITE;
                goto exit;
        }

        if_failed_log(llist_append(allocator->default_options, option, false), exit,
                        LOG_ERROR, NULL,"Failed to add dhcp option to allocator");

        rv = ALLOCATOR_OK;
exit:
        return rv;
}

int allocator_change_dhcp_option(address_allocator_t *allocator, uint32_t tag, 
                void *new_value, uint8_t new_length)
{
        int rv = ALLOCATOR_ERROR;
        if_null(allocator, exit);
        if_null(new_value, exit);

        dhcp_option_t *option = dhcp_option_retrieve(allocator->default_options, tag);
        rv = ALLOCATOR_OPTION_INVALID;
        if_null_log(option, exit, LOG_ERROR, NULL, "Option with tag %lu doesnt exist", tag);

        memcpy(option->value.binary_data, new_value, new_length);
        option->lenght = new_length;

        rv = ALLOCATOR_OK;
exit:
        return rv;
}

bool allocator_is_address_available(address_allocator_t *allocator, uint32_t address)
{
        if_null(allocator, exit);

        address_pool_t *p = allocator_get_pool_by_address(allocator, address);
        if_null_log(p, exit, LOG_WARN, NULL, 
                        "Pool containing address %s was not found, make sure it exists",
                        uint32_to_ipv4_address(address));

        return !address_pool_get_address_allocation(p, address);

exit:
        /* To be safe, return false so we dont try to use the address */
        return false;
}

bool allocator_is_address_available_str(address_allocator_t *allocator, const char *address)
{
        return allocator_is_address_available(allocator, ipv4_address_to_uint32(address));
}


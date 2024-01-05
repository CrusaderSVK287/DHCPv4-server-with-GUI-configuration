#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include "dhcp_options.h"
#include "utils/llist.h"
#include "address_pool.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct allocaor {
    llist_t *default_options;
    llist_t *address_pools;
} address_allocator_t;

/**
 * Create new address_allocator_t. Returns pointer to the new structure on 
 * success, NULL on failure
 */
address_allocator_t* address_allocator_new();

/**
 * Destroys allocator and all its members, sets the pointer to NULL
 */
void allocator_destroy(address_allocator_t **a);

int allocator_add_pool(address_allocator_t *allocator, address_pool_t *pool);

int allocator_request_any_address(address_allocator_t *allocator, uint32_t *addr_buf);

int allocator_request_address_from_pool(address_allocator_t *allocator,
        const char *pool, uint32_t *addr_buf);

int allocator_request_this_address(address_allocator_t *allocator,
        uint32_t requested_addres, uint32_t *addr_buf);

int allocator_request_this_address_str(address_allocator_t *allocator,
        const char *requested_addres, uint32_t *addr_buf);

int allocator_release_address(address_allocator_t *allocator, uint32_t address);

int allocator_release_address_str(address_allocator_t *allocator, const char *address);

int allocator_add_dhcp_option(address_allocator_t *allocator, dhcp_option_t *option);

int allocator_change_dhcp_option(address_allocator_t *allocator, uint32_t tag, void *value);

bool allocator_is_address_available(address_allocator_t *allocator, uint32_t address);

bool allocator_is_address_available_str(address_allocator_t *allocator, const char *address);

#endif /* __ALLOCATOR_H__ */

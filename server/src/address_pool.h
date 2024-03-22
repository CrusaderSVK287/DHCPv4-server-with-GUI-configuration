#ifndef __ADDRESS_POOL_H__
#define __ADDRESS_POOL_H__

#include "utils/llist.h"
#include <stdbool.h>
#include <stdint.h>

#define ADDRESS_POOL_NAME_MAX_LENGHT 64

typedef struct pool {
    char *name;

    uint32_t start_address;
    uint32_t end_address;
    uint32_t mask;

    /* 
     * Bitmask storing information about which IP address in pool is leased and 
     * which is available. 0 Means it is available, 1 means it is in use.
     * NOTE: Even if address is reserved, if the client is not using it it should 
     * be marked as available.
     */
    uint8_t *leases_bm;
    /* 
     * Count of curretnly not assigned addresses from this pool. 
     * Or number of 0 valued bits in leases_bm
     */
    uint32_t available_addresses;

    llist_t *dhcp_option_override;
} address_pool_t;

// TODO: Add something to autmatically create subnet mask
address_pool_t* address_pool_new(const char *name, uint32_t start_address,
        uint32_t end_address, uint32_t subnet_mask);

address_pool_t* address_pool_new_str(const char *name, const char *start_address,
        const char *end_address, const char *subnet_mask);

void address_pool_destroy(address_pool_t **pool);

bool address_belongs_to_pool(address_pool_t *pool, uint32_t address);

bool address_belongs_to_pool_str(address_pool_t *pool, const char *address);

/**
 * Function controlls pools allocation tracking bitmask. sets or gets appropriate 
 * bit belonging to address in pools leases_bm bitmask.
 * Actions:
 * 's' : set address allocation to 1
 * 'c' : set address allocation to 0
 * 'g' : return address allocation 
 * In action s and c, the function returns 0 on success, -1 on error.
 * Below are declared helper functions for this function
 */
int address_pool_address_allocation_ctl(address_pool_t *pool, uint32_t address, int action);

int address_pool_set_address_allocation(address_pool_t *pool, uint32_t address);
int address_pool_set_address_allocation_str(address_pool_t *pool, const char *address);

int address_pool_get_address_allocation(address_pool_t *pool, uint32_t address);
int address_pool_get_address_allocation_str(address_pool_t *pool, const char *address);

int address_pool_clear_address_allocation(address_pool_t *pool, uint32_t address);
int address_pool_clear_address_allocation_str(address_pool_t *pool, const char *address);
#endif // !__ADDRESS_POOL_H__

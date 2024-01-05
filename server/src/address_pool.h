#ifndef __ADDRESS_POOL_H__
#define __ADDRESS_POOL_H__

#include "utils/llist.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct pool {
    const char *name;

    uint32_t start_address;
    uint32_t end_address;
    uint32_t mask;

    llist_t *dhcp_option_override;
} address_pool_t;

address_pool_t* address_pool_new(const char *name, uint32_t start_address,
        uint32_t end_address, uint32_t subnet_mask);

address_pool_t* address_pool_new_str(const char *name, const char *start_address,
        const char *end_address, const char *subnet_mask);

void address_pool_destroy(address_pool_t **pool);

bool address_belongs_to_pool(address_pool_t *pool, uint32_t address);

bool address_belongs_to_pool_str(address_pool_t *pool, const char *address);


#endif // !__ADDRESS_POOL_H__

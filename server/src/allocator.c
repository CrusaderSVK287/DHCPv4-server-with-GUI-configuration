#include "allocator.h"
#include "utils/xtoy.h"
#include "logging.h"

address_allocator_t* address_allocator_new()
{
        return NULL;
}

void allocator_destroy(address_allocator_t **a)
{

}

int allocator_add_pool(address_allocator_t *allocator, address_pool_t *pool)
{
        return 0;
}

int allocator_request_any_address(address_allocator_t *allocator, uint32_t *addr_buf)
{
        return 0;
}

int allocator_request_address_from_pool(address_allocator_t *allocator,
        const char *pool, uint32_t *addr_buf)
{
        return 0;
}

int allocator_request_this_address(address_allocator_t *allocator,
        uint32_t requested_addres, uint32_t *addr_buf)
{
        return 0;
}

int allocator_request_this_address_str(address_allocator_t *allocator,
        const char *requested_addres, uint32_t *addr_buf)
{
        return allocator_request_this_address(allocator, 
                        ipv4_address_to_uint32(requested_addres), addr_buf);
}

int allocator_release_address(address_allocator_t *allocator, uint32_t address)
{
        return 0;
}

int allocator_release_address_str(address_allocator_t *allocator, const char *address)
{
        return allocator_release_address(allocator, ipv4_address_to_uint32(address));
}

int allocator_add_dhcp_option(address_allocator_t *allocator, dhcp_option_t *option)
{
        return 0;
}

int allocator_change_dhcp_option(address_allocator_t *allocator, uint32_t tag, void *value)
{
        return 0;
}

bool allocator_is_address_available(address_allocator_t *allocator, uint32_t address)
{
        return false;
}

bool allocator_is_address_available_str(address_allocator_t *allocator, const char *address)
{
        return allocator_is_address_available(allocator, ipv4_address_to_uint32(address));
}


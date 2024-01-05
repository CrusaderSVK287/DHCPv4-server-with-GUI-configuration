#ifndef __XTOY_H__
#define __XTOY_H__

#include <stdint.h>

/**
 * Convert uint32_t to string representation of ipv4 address.
 * Uses static buffer, therefore if you need to use the value, use strdup()
 * or make sure you are not using this function before using the value.
 */
const char* uint32_to_ipv4_address(uint32_t a);

uint32_t ipv4_address_to_uint32(const char *s);

#endif // !__XTOY_H__


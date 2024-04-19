#ifndef __XTOY_HPP__
#define __XTOY_HPP__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/**
 * Convert uint32_t to string representation of ipv4 address.
 * Uses static buffer, therefore if you need to use the value, use strdup()
 * or make sure you are not using this function before using the value.
 */
const char* uint32_to_ipv4_address(uint32_t a);

uint32_t ipv4_address_to_uint32(const char *s);

/* Convert uint8 array of size 6 to string mac and vice versa */
void mac_to_uint8_array(const char *mac, uint8_t array[]);

const char *uint8_array_to_mac(uint8_t mac[]);

#ifdef __cplusplus
}
#endif
#endif // !__XTOY_HPP__


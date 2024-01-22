#include "xtoy.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char* uint32_to_ipv4_address(uint32_t a)
{
        static char buf[16];
        snprintf(buf, 16, "%u.%u.%u.%u",
                        (a >> 24) & 0xff,
                        (a >> 16) &0xff,
                        (a >> 8) & 0xff,
                        a & 0xff);
        return buf;
}

uint32_t ipv4_address_to_uint32(const char *s)
{
        uint32_t a, b, c, d;
        if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4)
                return 0;

        return (a << 24) | (b << 16) | (c << 8) | d;
}

const char *uint8_array_to_mac(uint8_t mac[])
{
        static char buf[18];
        snprintf(buf, 18, "%02x:%02x:%02x:%02x:%02x:%02x", 
                        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

        return buf;
}

void mac_to_uint8_array(const char *mac, uint8_t array[])
{
        sscanf(mac, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                &array[0], &array[1], &array[2], &array[3], &array[4], &array[5]);
}


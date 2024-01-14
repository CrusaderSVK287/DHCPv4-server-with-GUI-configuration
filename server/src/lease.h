#ifndef __LEASE_H__
#define __LEASE_H__

#include <stdint.h>
#define LEASE_PATH_PREFIX "/etc/dhcp"

typedef struct ip_lease {
    uint32_t address;
    uint32_t subnet;
    char *pool_name;

    uint32_t lease_start;
    uint32_t lease_expire;

    uint32_t xid;
    uint8_t client_mac_address[6];
} lease_t;

#endif // !__LEASE_H__


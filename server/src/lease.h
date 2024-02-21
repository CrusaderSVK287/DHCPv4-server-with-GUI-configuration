#ifndef __LEASE_H__
#define __LEASE_H__

#include "utils/llist.h"
#include <stdint.h>

/* COMMENT OUT FOR RELEASE BUILD */
// #define LEASES_TEST_BUILD

#ifdef LEASES_TEST_BUILD
#define LEASE_PATH_PREFIX "./test/test_leases/"
#else
#define LEASE_PATH_PREFIX "/etc/dhcp/lease/"
#endif // LEASES_TEST_BUILD

enum lease_flags {
    LEASE_FLAG_STATIC_ALLOCATION = (1 << 0),
};

enum lease_status {
    LEASE_OK = 0,
    LEASE_EXISTS = -1,
    LEASE_DOESNT_EXITS = -2,
    LEASE_INVALID_JSON = -3,
    LEASE_ERROR = -4,
};

/**
 * Structured represenation of a lease. This structure is used when we want to 
 * store/load information about particular lease to a .lease file
 *
 * address: ipv4 address assosiated with the lease 
 * subnet: subnet mask assosiated with the lease/pool 
 * pool_name: name of pool to which the address belongs, used to locate the right file
 * lease_start: UNIX timestamp for when the lease was assigned
 * lease_expire: UNIX timestamp for when the lease will expire 
 * xid: xid of transaction in which the lease was created
 * flags: flags for allocated address, for now, only one flag exists, 
 *        rest is reserved for future use
 * client_mac_address: mac address of client to which the lease belongs.
 */
typedef struct ip_lease {
    uint32_t address;
    uint32_t subnet;
    char *pool_name;

    uint32_t lease_start;
    uint32_t lease_expire;

    uint32_t xid;
    uint8_t flags;
    uint8_t client_mac_address[6];
} lease_t;

/* Allocate space for new lease */
lease_t *lease_new();

/* destroy allocated lease */
void lease_destroy(lease_t **l);

/* 
 * Retrieves information on lease of address in addr.
 * Since this function doesnt know pool_name, it needs to go through all pools 
 * to find one to which the address belong. Use lease_retrieve_pool if possible 
 */
int lease_retrieve_address(lease_t *result, uint32_t addr, llist_t *pools);

/* Retrieves information on lease of address in addr from pool_name */
int lease_retrieve(lease_t *result, uint32_t addr, char *pool_name);

/* Adds lease information to the lease->pool_name's .lease file */
int lease_add(lease_t *lease);

/* 
 * Removes lease information from lease file.
 * NOTE: lease MUST have configured valid lease->address at lease. 
 * Preferably also configured lease->pool_name if possible 
 */
int lease_remove(lease_t *lease);

/*
 * Helper function. acts same as lease_remove but creates the required lease_t 
 * itself
 */
int lease_remove_address_pool(uint32_t address, char *pool_name);

#endif // !__LEASE_H__


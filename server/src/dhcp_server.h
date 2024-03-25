#ifndef __DHCP_SERVER_H__
#define __DHCP_SERVER_H__

#include "allocator.h"
#include "transaction_cache.h"
#include "timer.h"
#include "utils/llist.h"
#include "security/acl.h"
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct dhcp_server {
    int sock_fd;
    address_allocator_t *allocator;
    transaction_cache_t *trans_cache;

    /* Wrapper structure to hold all timers used by server */
    struct {
        struct timer *lease_expiration_check;
    } timers;

    struct {
        char        config_path[PATH_MAX];
        char        interface[256];         // name of bound interface. Can be empty if ip address is specified
        uint32_t    bound_ip;               // ip address of server. Is retrieved using the interace name
        uint32_t    broadcast_addr;         // broadcast domain of the server. Is determined from interface name
        uint32_t    tick_delay;             // delay in miliseconds between server ticks.
        uint32_t    cache_size;             // number of max transactions held in a cache 
        uint32_t    trans_duration;         // duration in seconds for which the transactions are stored in cache
        uint32_t    lease_expiration_check; // period in seconds after which server checks lease database for expired leases and removes them.
        uint32_t    lease_time;
        uint8_t     log_verbosity;          // verbosity of logger messages
        
        uint8_t     acl_enable;             // enable ACL security feature (default true)
        uint8_t     acl_blacklist;          // is ACL a blacklist? (default true)
    } config;

    ACL_t *acl;
} dhcp_server_t;

/**
 * initialises dhcp server but does not run it yet. 
 * Only fills dhcp_server_t struct with needed data
 */
int init_dhcp_server(dhcp_server_t *server);

/* Initialise timers used by dhcp server */
int init_dhcp_server_timers(dhcp_server_t *server);

/**
 * Stops and deinitialises dhcp server
 */
int uninit_dhcp_server(dhcp_server_t *server);

/*
 * Start DHCP server with structure initialised with dhcp_server_init().
 * Server keeps being active until it receives interupt signal
 */
int dhcp_server_serve(dhcp_server_t *server);

#endif /* __DHC_SERVER_H__ */

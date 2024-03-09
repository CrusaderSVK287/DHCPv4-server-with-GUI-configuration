#ifndef __DHCP_SERVER_H__
#define __DHCP_SERVER_H__

#include "allocator.h"
#include "transaction.h"
#include "transaction_cache.h"
#include "timer.h"

typedef struct dhcp_server {
    int sock_fd;
    address_allocator_t *allocator;
    transaction_cache_t *trans_cache;

    /* Wrapper structure to hold all timers used by server */
    struct {
        struct timer *lease_expiration_check;
    } timers;
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

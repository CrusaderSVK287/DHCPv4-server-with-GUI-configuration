#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "dhcp_server.h"
int init_allocator(dhcp_server_t *server);
int init_address_pools(dhcp_server_t *server);
int init_dhcp_options(dhcp_server_t *server);
int init_cache(dhcp_server_t *server);

#endif // !__CONFIGURATION_H__


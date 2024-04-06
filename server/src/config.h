#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "dhcp_server.h"

/* Return value for when program called with --version or --help flag */
#define CONFIG_EXIT_PROGRAM 1
/* Since we need to keep track of untouched parts of config, we cannot use proper booleans */
#define CONFIG_UNTOUCHED  0 
#define CONFIG_BOOL_FALSE 1
#define CONFIG_BOOL_TRUE  2 

#define CONFIG_DEFAULT_PATH "/etc/dhcp/config.json"
#define CONFIG_DEFAULT_TICK_DELAY 1000
#define CONFIG_DEFAULT_CACHE_SIZE 25
#define CONFIG_DEFAULT_TRANS_DURATION 60
#define CONFIG_DEFAULT_LEASE_EXPIRATION_CHECK 60
#define CONFIG_DEFAULT_LOG_VERBOSITY 4

#define CONFIG_DEFAULT_LEASE_TIME 43200
#define CONFIG_DEFAULT_POOL_NAME "Pool"
#define CONFIG_DEFAULT_POOL_START "192.168.1.1"
#define CONFIG_DEFAULT_POOL_END "192.168.1.100"
#define CONFIG_DEFAULT_POOL_MASK "255.255.255.0"

#define CONFIG_DEFAULT_ACL_ENABLE CONFIG_BOOL_TRUE
#define CONFIG_DEFAULT_ACL_BLACKLIST CONFIG_BOOL_TRUE
//TODO: Make a toggle to enable/disable database storing

int config_parse_arguments(dhcp_server_t *server, int argc, char **argv);

void print_usage(const char *prog_name);

int config_get_interface_info(dhcp_server_t *server);

/* Finishes up confiugring server by fetching missing data from config file in path, or from default path when n ocustom path is set*/
int config_load_configuration(dhcp_server_t *server);

#endif // !__CONFIG_H__


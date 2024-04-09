#include "config.h"
#include <bits/getopt_core.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "RFC/RFC-2132.h"
#include "address_pool.h"
#include "allocator.h"
#include "cclog_macros.h"
#include "logging.h"
#include "dhcp_options.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include <cJSON.h>

void print_usage(const char *prog_name)
{
        printf("Usage: %s TODO\n", prog_name);
}

static void print_version()
{
        printf("Version: 1.0 TODO\n");
}

int config_get_interface_info(dhcp_server_t *server)
{
        if (!server)
                return -1;

        struct ifaddrs *ifap, *ifa;

        /* Get list of all interfaces and itterate through them to find out interface */
        if (getifaddrs(&ifap) != 0) {
                fprintf(stderr, "Failed to retrieve list of interfaces, exiting\n");
        }

        for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
                if (strcmp(ifa->ifa_name, server->config.interface) || !ifa->ifa_addr)
                        continue;

                if (ifa->ifa_addr->sa_family == AF_INET) {
                        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
                        server->config.bound_ip = addr->sin_addr.s_addr;
                }

                if (ifa->ifa_broadaddr != NULL && ifa->ifa_addr->sa_family == AF_INET) {
                        struct sockaddr_in *broadcast = (struct sockaddr_in *)ifa->ifa_broadaddr;
                        server->config.broadcast_addr = broadcast->sin_addr.s_addr;
                }

                /* If we have both interface ip and broadcast addresses, we can exit the loop */
                if (server->config.bound_ip && server->config.broadcast_addr)
                        break;
        }

        freeifaddrs(ifap);
        
        if (!server->config.bound_ip || !server->config.broadcast_addr) {
                fprintf(stderr, "Failed to retrieve information on interface '%s'. Check if the "
                                "interface name is correct", server->config.interface);
                return -1;
        }

        return 0;
}

static cJSON *config_load_json_file(const char *path)
{
        struct stat st = {0};
        if (stat(path, &st) < 0) {
                fprintf(stderr, "Cannot stat file %s", path);
                goto error;
        }

        int fd = open(path, O_RDONLY);
        if (fd < 0) {
                fprintf(stderr, "Failed to open configuration file %s: %s\n", path, strerror(errno));
                goto error;
        }

        char *config = calloc(1, st.st_size + 32);
        
        /* Get entire json from the file */
        if (read(fd, config, st.st_size + 32) < 0) {
                fprintf(stderr, "Failed reading from %s: %s\n", path, strerror(errno));
                goto error;
        }

        cJSON *result = cJSON_Parse(config);

        free(config);
        return result;
error:
        return NULL;
}

static int config_load_security_config(dhcp_server_t *server, cJSON *security_config)
{
        if (!server)
                return -1;

        int rv = -1;

        cJSON *object = NULL;

        if (server->config.acl_enable == CONFIG_UNTOUCHED) {
                object = cJSON_GetObjectItem(security_config, "acl_enable");
                server->config.acl_enable = (object) ? cJSON_IsTrue(object) : CONFIG_DEFAULT_ACL_ENABLE;
        }
        
        if (server->config.acl_blacklist == CONFIG_UNTOUCHED) {
                object = cJSON_GetObjectItem(security_config, "acl_blacklist");
                server->config.acl_blacklist = (object) ? cJSON_IsTrue(object) : CONFIG_DEFAULT_ACL_BLACKLIST;
        }

        rv = 0;

        return rv;
}

static int config_load_server_config(dhcp_server_t *server, cJSON *server_config)
{
        if (!server)
                return -1;

        int rv = -1;

        cJSON *object = NULL;

        if (!strlen(server->config.interface)) {
                object = cJSON_GetObjectItem(server_config, "interface");
                if (!object) {
                        fprintf(stderr, "Error, interface not specified. Interface MUST be specified "
                                        "eighter in a configuration file or using --interface flag\n");
                        goto exit;
                }
                
                strncpy(server->config.interface, cJSON_GetStringValue(object), 255);
                if (config_get_interface_info(server) < 0) {
                        fprintf(stderr, "Error getting interface information, exiting\n");
                        goto exit;
                }
        }

        if (!server->config.tick_delay) {
                object = cJSON_GetObjectItem(server_config, "tick_delay");
                server->config.tick_delay = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_TICK_DELAY;
        }

        if (!server->config.cache_size) {
                object = cJSON_GetObjectItem(server_config, "cache_size");
                server->config.cache_size = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_CACHE_SIZE;
        }

        if (!server->config.trans_duration) {
                object = cJSON_GetObjectItem(server_config, "trans_duration");
                server->config.trans_duration = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_TRANS_DURATION;
        }
        
        if (!server->config.lease_expiration_check) {
                object = cJSON_GetObjectItem(server_config, "lease_expiration_check");
                server->config.lease_expiration_check = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_LEASE_EXPIRATION_CHECK;
        }

        if (!server->config.log_verbosity) {
                object = cJSON_GetObjectItem(server_config, "log_verbosity");
                server->config.log_verbosity = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_LOG_VERBOSITY;
        }

        if (!server->config.lease_time) {
                object = cJSON_GetObjectItem(server_config, "lease_time");
                server->config.lease_time = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_LEASE_TIME;
        }

        if (!server->config.db_enable) {
                object = cJSON_GetObjectItem(server_config, "db_enable");
                server->config.db_enable = (object) ? cJSON_GetNumberValue(object) : CONFIG_DEFAULT_DB_ENABLE;
        }

        rv = 0;
exit:
        return rv;
}

static int config_load_dhcp_options(llist_t *dest, cJSON *options)
{
        if (!dest)
                return -1;
        if (!options)
                return 0;

        int rv = -1;

        cJSON *opt = NULL;
        cJSON *value = NULL;
        uint8_t tag;
        uint8_t len;
        dhcp_option_t *new_opt = NULL;

        uint32_t val_num;
        char *val_string;
        bool val_bool;

        cJSON_ArrayForEach(opt, options) {
                tag = cJSON_GetNumberValue(cJSON_GetObjectItem(opt, "tag"));
                len = cJSON_GetNumberValue(cJSON_GetObjectItem(opt, "lenght"));
                value = cJSON_GetObjectItem(opt, "value");

                /* Configuration format error */
                if (!tag || !len || !value) {
                        fprintf(stderr, "Configuration error: Missing %s%s%s\n", 
                                        (tag) ? "" : "tag ", 
                                        (len) ? "" : "lenght ",
                                        (value) ? "" : "value ");
                        continue;
                }

                switch (dhcp_option_tag_to_type(tag)) {
                case DHCP_OPTION_IP:
                        if_false(cJSON_IsString(value), exit);
                        val_num = ipv4_address_to_uint32(cJSON_GetStringValue(value));
                        if_false(val_num, exit);
                        new_opt = dhcp_option_new_values(tag, len, &val_num);
                        break;
                        
                case DHCP_OPTION_NUMERIC:
                        if_false(cJSON_IsNumber(value), exit);
                        val_num = cJSON_GetNumberValue(value);
                        new_opt = dhcp_option_new_values(tag, len, &val_num);
                        break;
                /* 
                 * JSON cannot store binary formats in particular, but technically
                 * no text format can. We can use the same logic as for string type 
                 */
                case DHCP_OPTION_BIN:
                case DHCP_OPTION_STRING:
                        if_false(cJSON_IsString(value), exit);
                        val_string = cJSON_GetStringValue(value);
                        if_null(val_string, exit);
                        new_opt = dhcp_option_new_values(tag, len, val_string);
                        break;

                case DHCP_OPTION_BOOL:
                        if_false(cJSON_IsBool(value), exit);
                        val_bool = cJSON_IsTrue(value);
                        new_opt = dhcp_option_new_values(tag, len, &val_bool);
                        break;

                default:
                        fprintf(stderr, "Unknown tag or type: %d\n", tag);
                        goto exit;
                }

                /* 
                 * We dont want to raise an error if this fails. If the option is 
                 * already configured via cli arguments, this function will still 
                 * return 0. This IS expected behaviour.
                 */
                if (dhcp_option_add(dest, new_opt) < 0) {
                        fprintf(stderr, "Failed to add dhcp option %d from configuration file. "
                                        "Server will continue to function, but will not use this option.\n", tag);           
                }
        }

        rv = 0;
exit:
        return rv;
}

static int config_load_pools_config(dhcp_server_t *server, cJSON *pools_config)
{
        if (!server)
                return -1;
        if (!pools_config)
                return 0;

        int rv = -1;

        char *pool_name = NULL;
        uint32_t pool_start = 0;
        uint32_t pool_end   = 0;
        uint32_t pool_mask  = 0;
        cJSON *object = NULL;

        address_pool_t *new_pool = NULL;
        cJSON *pool = NULL;
        cJSON_ArrayForEach(pool, pools_config) {
                object = cJSON_GetObjectItem(pool, "name");
                if (object)
                        pool_name = cJSON_GetStringValue(object);
                if (!pool_name) {
                        fprintf(stderr, "Error creating pool. Missing attribute: name\n");
                        continue;
                }

                object = cJSON_GetObjectItem(pool, "start");
                if (object)
                        pool_start = ipv4_address_to_uint32(cJSON_GetStringValue(object));
                if (!pool_start) {
                        fprintf(stderr, "Error creating pool %s. Missing attribute: start\n", pool_name);
                        continue;
                }

                object = cJSON_GetObjectItem(pool, "end");
                if (object)
                        pool_end = ipv4_address_to_uint32(cJSON_GetStringValue(object));
                if (!pool_end) {
                        fprintf(stderr, "Error creating pool %s. Missing attribute: end\n", pool_name);
                        continue;
                }

                object = cJSON_GetObjectItem(pool, "subnet");
                if (object)
                        pool_mask = ipv4_address_to_uint32(cJSON_GetStringValue(object));
                if (!pool_mask) {
                        fprintf(stderr, "Error creating pool %s. Missing attribute: subnet\n", pool_name);
                        continue;
                }

                /* If pool creation fails, continue server running but alert user */
                new_pool = address_pool_new(pool_name, pool_start, pool_end, pool_mask);
                if (!new_pool) {
                        fprintf(stderr, "Failed to create pool %s. Server will keep running, but "
                                        "this pool will not be used\n", pool_name);
                        continue;
                }

                /* Add dhcp options to pool, options can be null */
                object = cJSON_GetObjectItem(pool, "options");
                if (object && config_load_dhcp_options(new_pool->dhcp_option_override, object) < 0) {
                        fprintf(stderr, "Error configuring dhcp options for pool %s\n", pool_name);
                        continue;
                }

                rv = allocator_add_pool(server->allocator, new_pool);
                if (rv == ALLOCATOR_POOL_DUPLICITE) {
                        fprintf(stderr, "Misconfiguration: Pool named %s already exists, "
                                        "check your configuration file", pool_name);
                } else if (rv != ALLOCATOR_OK) {
                        fprintf(stderr, "Error adding pool %s to allocator: %s\n", 
                                        pool_name, allocator_strerror(rv));
                }
        }

        rv = 0;
// exit:
        return rv;
}
int config_load_configuration(dhcp_server_t *server)
{
        if (!server)
                return -1;

        int rv = -1;

        cJSON *config = config_load_json_file((strlen(server->config.config_path)) ? 
                                                server->config.config_path : CONFIG_DEFAULT_PATH);

        if (!config && !strlen(server->config.interface)) {
                fprintf(stderr, "Erorr. Failed to retrieve configuration from file and "
                                "no interface provided, please provide interface\n");
                goto exit;
        }

        if (config_load_server_config(server, cJSON_GetObjectItem(config, "server")) < 0) {
                fprintf(stderr, "Error configuring server. Check configuration "
                                "file for syntax errors or asses the manual\n");
                goto exit;
        }

        if (config_load_pools_config(server, cJSON_GetObjectItem(config, "pools")) < 0) {
                fprintf(stderr, "Error configuring address pools. Check configuration "
                                "file for syntax errors or asses the manual\n");
                goto exit;
        }

        if (config_load_dhcp_options(server->allocator->default_options, cJSON_GetObjectItem(config, "options")) < 0) {
                fprintf(stderr, "Error configuring global dhcp options. Check configuration "
                                "file for syntax errors or asses the manual\n");
                goto exit;
        }

        if (config_load_security_config(server, cJSON_GetObjectItem(config, "security")) < 0) {
                fprintf(stderr, "Error configuring security options. Check configuration "
                                "file for syntax errors or asses the manual\n");
                goto exit;
        }

        rv = 0;
exit:
        return rv;
}

static int config_load_defaults(dhcp_server_t *server)
{
        if (!server || !server->allocator || !server->allocator->default_options || !server->allocator->address_pools)
                return -1;
        
        int rv = -1;

        strcpy(server->config.config_path, CONFIG_DEFAULT_PATH);
        strcpy(server->config.interface, optarg);
        server->config.tick_delay = CONFIG_DEFAULT_TICK_DELAY;
        server->config.cache_size = CONFIG_DEFAULT_CACHE_SIZE;
        server->config.trans_duration = CONFIG_DEFAULT_TRANS_DURATION;
        server->config.lease_expiration_check = CONFIG_DEFAULT_LEASE_EXPIRATION_CHECK;
        server->config.log_verbosity = CONFIG_DEFAULT_LOG_VERBOSITY;
        server->config.lease_time = CONFIG_DEFAULT_LEASE_TIME;
        
        /* Default config doesnt have acl at all */
        server->config.acl_enable = CONFIG_BOOL_FALSE;
        server->config.acl_blacklist = CONFIG_BOOL_FALSE;
        server->config.db_enable = CONFIG_DEFAULT_DB_ENABLE;
        
        uint32_t lease_time_value = CONFIG_DEFAULT_LEASE_TIME;
        if (dhcp_option_add(server->allocator->default_options, dhcp_option_new_values(
                                        DHCP_OPTION_IP_ADDRESS_LEASE_TIME, 4, &lease_time_value
                                        )) < 0) {
                fprintf(stderr, "Error adding default dhcp option, exiting\n");
                goto exit;
        }

        if (allocator_add_pool(server->allocator, address_pool_new_str(
                                        CONFIG_DEFAULT_POOL_NAME,
                                        CONFIG_DEFAULT_POOL_START,
                                        CONFIG_DEFAULT_POOL_END,
                                        CONFIG_DEFAULT_POOL_MASK
                                        )) != ALLOCATOR_OK) {
                fprintf(stderr, "Error adding default pool, exiting\n");
                goto exit;
        }

        if (config_get_interface_info(server) < 0) {
                fprintf(stderr, "Error getting interface information, exiting\n");
                goto exit;
        }

        rv = 0;
exit:
        return rv;
}

static int config_add_dhcp_option(dhcp_server_t *server)
{
        if (!server || !optarg || !strlen(optarg))
                return -1;

        int rv = -1;

        char value[256];

        dhcp_option_t *o = dhcp_option_new();
        if (!o) {
                fprintf(stderr, "Failed to allocate memory for dhcp option, exiting\n");
                goto exit;
        }

        if (sscanf(optarg, "%hhu:%hhu:%[^\n]", &o->tag, &o->lenght, value) != 3) {
                fprintf(stderr, "Format error with --option. Usage: --option tag:lenght:value\n");
                goto exit;
        }

        o->type = dhcp_option_tag_to_type(o->tag);

        switch (dhcp_option_tag_to_type(o->tag)) {
                case DHCP_OPTION_NUMERIC:
                        if (sscanf(value, "%u", &o->value.number) != 1) {
                                fprintf(stderr, "Bad format for option %d, exiting\n", o->tag);
                                goto exit;
                        }
                        break;
                case DHCP_OPTION_IP:
                        o->value.ip = ipv4_address_to_uint32(value);
                        if (o->value.ip == 0) {
                                fprintf(stderr, "Option %d required ip address argument in this format: xxx.xxx.xxx.xxx\n", o->tag);
                                goto exit;
                        }
                        break;
                case DHCP_OPTION_BOOL:
                        if (sscanf(value, "%hhu", &o->value.boolean) != 1) {
                                fprintf(stderr, "Bad format for option %d, exiting\n", o->tag);
                                goto exit;
                        }
                        if (o->value.boolean != 0 && o->value.boolean != 1) {
                                fprintf(stderr, "Bad format for option %d, boolean options require value in format '0' or '1'\n", o->tag);
                                goto exit;
                        }
                        break;
                case DHCP_OPTION_STRING:
                        strncpy(o->value.string, value, 256);
                        break;
                case DHCP_OPTION_BIN:
                        fprintf(stderr, "Option %d has a binary value, it cannot be set via cli arguments\n", o->tag);
                        goto exit;
                default:
                        fprintf(stderr, "Unknown type of dhcp option tag %d\n", o->tag);
                        goto exit;
        }

        if (dhcp_option_add(server->allocator->default_options, o) < 0) {
                fprintf(stderr, "Failed to add dhcp option %d\n", o->tag);
                goto exit;
        }

        rv = 0;
exit:
        return rv;
}

static int config_add_pool(dhcp_server_t *server)
{
        if (!server || !optarg || !strlen(optarg))
               return -1;

        int rv = -1;

        char start[16], end[16], mask[16];
        if (sscanf(optarg, "%15[^:]:%15[^:]:%15s", start, end, mask) != 3) {
                fprintf(stderr, "Format error. Pool requires arguments in this format: "
                                "xxx.xxx.xxx.xxx:xxx.xxx.xxx.xxx:xxx.xxx.xxx.xxx\n"
                                "Where the individual arguments mean start_address:end_address:subnet_mask\n");
                goto exit;
        } 

        address_pool_t *pool = address_pool_new_str("pool", start, end, mask);
        if (!pool) {
                fprintf(stderr, "Failed to create custom pool, check your arguments\n");
                goto exit;
        }

        if ((rv = allocator_add_pool(server->allocator, pool)) != ALLOCATOR_OK) {
                fprintf(stderr, "Failed to add custom pool to allocator: %s\n", allocator_strerror(rv));
                goto exit;
        }

        rv = 0;
exit:
        return rv;
}

int config_parse_arguments(dhcp_server_t *server, int argc, char **argv)
{
        int rv = -1;

        if (!server) {
                fprintf(stderr, "Server is not initialised, cannot proceed");
                goto exit;
        }

        if (!server->allocator || !server->allocator->address_pools || !server->allocator->default_options) {
                fprintf(stderr, "Address allocator is not initialised, cannot proceed");
                goto exit;
        }

        static struct option long_options[] = {
                {"version",                 no_argument,        0, 'v'},
                {"help",                    no_argument,        0, 'h'},
                {"config",                  required_argument,  0, 'c'},
                {"default-configuration",   required_argument,  0,  1 },

                {"interface",               required_argument, 0, 'i'},
                {"tick-delay",              required_argument, 0, 'd'},
                {"cache-size",              required_argument, 0, 's'},
                {"transaction-duration",    required_argument, 0, 't'},
                {"lease-expiration-check",  required_argument, 0, 'e'},
                {"lease-time",              required_argument, 0, 'l'},
                {"log",                     required_argument, 0,  2 },

                {"acl-disable",             no_argument,       0,  3 },
                {"acl-whitelist-mode",      no_argument,       0,  4 },

                {"db-disable",              no_argument,       0,  5 },

                {"pool",    required_argument, 0, 'p'},
                {"option",  required_argument, 0, 'o'},
                {0, 0, 0, 0}
        };

        int opt;
        int option_index = 0;

        rv = 0;
        while ((opt = getopt_long(argc, argv, "vhci:d:s:t:e:l:p:o:", long_options, &option_index)) != -1 && rv == 0) {
                switch (opt) {
                case 'v':
                        print_version();
                        rv = CONFIG_EXIT_PROGRAM;
                        goto exit;
                case 'h':
                        print_usage(argv[0]);
                        rv = CONFIG_EXIT_PROGRAM;
                        goto exit;
                case 'c':
                        strncpy(server->config.config_path, optarg, PATH_MAX);
                        break;
                case 'i':
                        strncpy(server->config.interface, optarg, 256);
                        break;
                case 'd':
                        if (sscanf(optarg, "%u", &server->config.tick_delay) != 1)
                                rv = -1;
                        break;
                case 's':
                        if (sscanf(optarg, "%u", &server->config.cache_size) != 1)
                                rv = -1;
                        break;
                case 't':
                        if (sscanf(optarg, "%u", &server->config.trans_duration) != 1)
                                rv = -1;
                        break;
                case 'e':
                        if (sscanf(optarg, "%u", &server->config.lease_expiration_check) != 1)
                                rv = -1;
                        break;
                case 'l':
                        if (sscanf(optarg, "%u", &server->config.lease_time) != 1)
                                rv = -1;
                        break;
                case 'p':
                        rv = config_add_pool(server);
                        break;
                case 'o':
                        rv = config_add_dhcp_option(server);
                        break;
                case 1:
                        if (argc != 3) {
                                fprintf(stderr, "Flag --default-configuration is not compatible with any other flags, please use it by itself\n");
                                rv = -1;
                                goto exit;
                        }
                        config_load_defaults(server);
                        break;
                case 2: 
                        if (sscanf(optarg, "%hhu", &server->config.log_verbosity) != 1)
                                rv = -1;
                        break;
                case 3:
                        server->config.acl_enable = CONFIG_BOOL_FALSE;
                        break;
                case 4: 
                        server->config.acl_blacklist = CONFIG_BOOL_FALSE;
                        break;
                case 5:
                        server->config.db_enable = CONFIG_BOOL_FALSE;
                        break;
                default:
                        if (optopt == 0) {
                                fprintf(stderr, "Unknown option '%s' use --help for usage\n", argv[optind - 1]);
                        } else {
                                fprintf(stderr, "Unknown option '%c' use --help for usage\n", optopt);
                        }
                        rv = -1;
                        break;
                }
        }

exit:
        return rv;
}


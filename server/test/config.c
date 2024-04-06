#include "address_pool.h"
#include "allocator.h"
#include "dhcp_options.h"
#include <bits/getopt_core.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h> 
#include "dhcp_server.h"
#include "init.h"
#include "tests.h"
#include "greatest.h"
#include "utils/xtoy.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define ARGC (sizeof(arguments) / sizeof(arguments[0]) - 1)
static int original_opterr = 0;
//
// static int get_configured_interface(char *result) {
//     struct ifaddrs *ifaddr, *ifa;
//     int family, s;
//     char host[NI_MAXHOST];
//
//     int rv = -1;
//     if (getifaddrs(&ifaddr) == -1) {
//         perror("getifaddrs");
//         exit(EXIT_FAILURE);
//     }
//
//     // Walk through linked list, maintaining head pointer so we can free list later
//     for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
//         if (ifa->ifa_addr == NULL)
//             continue;
//
//         family = ifa->ifa_addr->sa_family;
//
//         // Display interface name if it has an IP address
//         if (family == AF_INET || family == AF_INET6) {
//             s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
//                             host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
//             if (s != 0 || strcmp(ifa->ifa_name, "lo") == 0) {
//                 continue;
//             }
//             strcpy(result, ifa->ifa_name);
//             rv = 0;
//             break;
//         }
//     }
//
//     freeifaddrs(ifaddr);
//     return rv;
// }

static void reset_getopt()
{
        optind = 1; // Reset optind to 1
        opterr = original_opterr; // Reset opterr to its original state

        // Reset getopt_long variables
        optopt = 0; // Reset optopt to 0
}

TEST test_config_parse_valid_arguments()
{
        SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        char *arguments[] = {
                "./dhcps",
                "--interface", INTERFACE,
                "--tick-delay", "10",
                "--cache-size", "12",
                "--transaction-duration", "32",
                "--lease-expiration-check", "50",
                "--log", "3",
                "--pool", "192.168.0.5:192.168.0.10:255.255.255.0",
                "--option", "12:5:Hello",
                "--db-disable",
                NULL // Null-terminate the array
        };

        ASSERT_EQ(0, config_parse_arguments(&server, ARGC, arguments));
        ASSERT_EQ(10, server.config.tick_delay);
        ASSERT_EQ(12, server.config.cache_size);
        ASSERT_EQ(32, server.config.trans_duration);
        ASSERT_EQ(50, server.config.lease_expiration_check);
        ASSERT_EQ(3,  server.config.log_verbosity);
        ASSERT_EQ(false, server.config.db_enable);
        address_pool_t *pool = allocator_get_pool_by_name(server.allocator, "pool");
        ASSERT_NEQ(NULL, pool);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.5"), pool->start_address);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.10"), pool->end_address);
        ASSERT_EQ(ipv4_address_to_uint32("255.255.255.0"), pool->mask);
        dhcp_option_t *option = dhcp_option_retrieve(server.allocator->default_options, 12);
        ASSERT_NEQ(NULL, option);
        ASSERT_EQ(12, option->tag);
        ASSERT_EQ(5, option->lenght);
        ASSERT_STR_EQ("Hello", option->value.string);

        PASS();
}

TEST test_parse_valid_config_file()
{
        SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        strcpy(server.config.config_path, "./test/config_sample.json");

        ASSERT_EQ(0, config_load_configuration(&server));

        ASSERT_EQ(1000, server.config.tick_delay);
        ASSERT_EQ(25, server.config.cache_size);
        ASSERT_EQ(60, server.config.trans_duration);
        ASSERT_EQ(60, server.config.lease_expiration_check);
        ASSERT_EQ(4,  server.config.log_verbosity);
        ASSERT_EQ(43200,  server.config.lease_time);
        address_pool_t *pool = allocator_get_pool_by_name(server.allocator, "LAN");
        ASSERT_NEQ(NULL, pool);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.2"), pool->start_address);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.100"), pool->end_address);
        ASSERT_EQ(ipv4_address_to_uint32("255.255.255.0"), pool->mask);
        dhcp_option_t *option = dhcp_option_retrieve(pool->dhcp_option_override, 51);
        ASSERT_NEQ(NULL, option);
        ASSERT_EQ(51, option->tag);
        ASSERT_EQ(4, option->lenght);
        ASSERT_EQ(600, option->value.number);
        
        option = dhcp_option_retrieve(server.allocator->default_options, 51);
        ASSERT_NEQ(NULL, option);
        ASSERT_EQ(51, option->tag);
        ASSERT_EQ(4, option->lenght);
        ASSERT_EQ(86400, option->value.number);
        option = dhcp_option_retrieve(server.allocator->default_options, 3);
        ASSERT_NEQ(NULL, option);
        ASSERT_EQ(3, option->tag);
        ASSERT_EQ(4, option->lenght);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.1"), option->value.ip);

        PASS();
}

TEST test_load_config_override_with_arguments()
{
        SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        reset_getopt();
        char *arguments[] = {
                "./dhcps",
                "--cache-size", "200",
                "--transaction-duration", "300",
                NULL // Null-terminate the array
        };

        strcpy(server.config.config_path, "./test/config_sample.json");
        ASSERT_EQ(0 , config_parse_arguments(&server, ARGC, arguments));
        ASSERT_EQ(0 , config_load_configuration(&server));
        ASSERT_EQ(1000, server.config.tick_delay);
        ASSERT_EQ(200, server.config.cache_size);
        ASSERT_EQ(300, server.config.trans_duration);
        ASSERT_EQ(60, server.config.lease_expiration_check);
        ASSERT_EQ(4 , server.config.log_verbosity);
        address_pool_t *pool = allocator_get_pool_by_name(server.allocator, "LAN");
        ASSERT_NEQ(NULL, pool);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.2"), pool->start_address);
        ASSERT_EQ(ipv4_address_to_uint32("192.168.0.100"), pool->end_address);
        ASSERT_EQ(ipv4_address_to_uint32("255.255.255.0"), pool->mask);
        dhcp_option_t *option = dhcp_option_retrieve(server.allocator->default_options, 51);
        ASSERT_NEQ(NULL, option);
        ASSERT_EQ(51, option->tag);
        ASSERT_EQ(4, option->lenght);
        ASSERT_EQ(86400, option->value.number);

        PASS();

}

TEST test_if_no_config_load_default_value()
{
        SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        strcpy(server.config.config_path, "./test/config_sample_incomplete.json");

        ASSERT_EQ(0, config_load_configuration(&server));

        ASSERT_EQ(555, server.config.tick_delay);
        ASSERT_EQ(CONFIG_DEFAULT_CACHE_SIZE, server.config.cache_size);
        ASSERT_EQ(CONFIG_DEFAULT_TRANS_DURATION, server.config.trans_duration);
        ASSERT_EQ(444, server.config.lease_expiration_check);
        ASSERT_EQ(CONFIG_DEFAULT_LOG_VERBOSITY,  server.config.log_verbosity);

        PASS();

}

TEST test_config_file_ok_but_pools_and_options_bad()
{
        SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        strcpy(server.config.config_path, "./test/config_sample_bad_pool_and_options.json");

        ASSERT_EQ(0, config_load_configuration(&server));
        ASSERT_EQ(NULL, allocator_get_pool_by_name(server.allocator, "LAN"));
        ASSERT_EQ(NULL, dhcp_option_retrieve(server.allocator->default_options, 51));

        PASS();
}

TEST test_config_missing_interface()
{
        // SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        strcpy(server.config.config_path, "./test/config_sample_empty.json");

        ASSERT_EQ(-1, config_load_configuration(&server));

        PASS();
}

TEST test_config_interface_in_cli_arguments()
{
        SKIP_IF_PIPELINE_BUILD;

        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        reset_getopt();
        char *arguments[] = {
                "./dhcps",
                "--interface", INTERFACE,
                NULL // Null-terminate the array
        };
        strcpy(server.config.config_path, "./test/config_sample_empty.json");

        ASSERT_EQ(0 , config_parse_arguments(&server, ARGC, arguments));
        ASSERT_EQ(0, config_load_configuration(&server));

        PASS();
}

TEST test_config_default_configuration_only_flag()
{
        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        reset_getopt();
        char *arguments[] = {
                "./dhcps",
                "--default-configuration", INTERFACE,
                "--cache_size", "500",
                NULL // Null-terminate the array
        };
        strcpy(server.config.config_path, "./test/config_sample_empty.json");

        ASSERT_EQ(-1 , config_parse_arguments(&server, ARGC, arguments));

        PASS();

}

TEST test_config_default_configuration()
{
        dhcp_server_t server = {0};
        ASSERT_EQ(0, init_allocator(&server));

        reset_getopt();
        char *arguments[] = {
                "./dhcps",
                "--default-configuration", INTERFACE,
                NULL // Null-terminate the array
        };

        ASSERT_EQ(0 , config_parse_arguments(&server, ARGC, arguments));
        ASSERT_EQ(0, config_load_configuration(&server));

        ASSERT_EQ(CONFIG_DEFAULT_TICK_DELAY, server.config.tick_delay);
        ASSERT_EQ(CONFIG_DEFAULT_LEASE_EXPIRATION_CHECK, server.config.lease_expiration_check);
        ASSERT_EQ(CONFIG_DEFAULT_CACHE_SIZE, server.config.cache_size);
        ASSERT_EQ(CONFIG_DEFAULT_TRANS_DURATION, server.config.trans_duration);
        ASSERT_EQ(CONFIG_DEFAULT_LOG_VERBOSITY, server.config.log_verbosity);
        address_pool_t *pool = allocator_get_pool_by_name(server.allocator, CONFIG_DEFAULT_POOL_NAME);
        ASSERT_NEQ(NULL, pool);
        ASSERT_EQ(ipv4_address_to_uint32(CONFIG_DEFAULT_POOL_START), pool->start_address);
        ASSERT_EQ(ipv4_address_to_uint32(CONFIG_DEFAULT_POOL_END), pool->end_address);
        ASSERT_EQ(ipv4_address_to_uint32(CONFIG_DEFAULT_POOL_MASK), pool->mask);

        PASS();
}

SUITE(config)
{
        original_opterr = opterr;
        RUN_TEST(test_config_parse_valid_arguments);
        RUN_TEST(test_parse_valid_config_file);
        RUN_TEST(test_load_config_override_with_arguments);
        RUN_TEST(test_if_no_config_load_default_value);
        RUN_TEST(test_config_file_ok_but_pools_and_options_bad);
        RUN_TEST(test_config_missing_interface);
        RUN_TEST(test_config_interface_in_cli_arguments);
        RUN_TEST(test_config_default_configuration_only_flag);
        RUN_TEST(test_config_default_configuration);
}


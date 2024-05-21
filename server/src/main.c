#include <cclog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "cclog_macros.h"
#include "lease.h"
#include "logging.h"
#include "dhcp_server.h"
#include "unix_server.h"
#include "init.h"
#include "config.h"
#include "utils/xtoy.h"

#define SERVER_VERSION "v1.0.0"
static void print_program_version()
{
        printf("Simple DHCPv4 server.\n" 
               "Version " SERVER_VERSION "\n"
               "Original creator: Lukáš Belán. Published under GPLv3 licence\n");
}

static void print_program_usage(const char *proc_name)
{
        printf("Simple DHCPv4 server. Used to configure DHCPv4 clients with IP addresses "
               "on small to medium networks. Configuration file is by default located "
               "at /etc/dhcp/config.json. Do mind that server required root access to be able to utilise sockets, "
               "read configuration and write logs\n"
               "Usage: %s [flags]\n\n"
               "Flags: Long, short, (argument): short description\n\t"
               "--version    -v: Shows version of the program and exit\n\t"
               "--help       -h: Shows usage of the program (this page) and exit\n\t"
               "--config     -c (path): Specify a path to different config file\n\t"
               "--default-configuration (interface): use default configuration for the server. Specify interface name as the argument\n\n\t"
               
               "--interface  -i (interface): Specify which interface the server should use\n\t"
               "--tick-delay -d (number): Time in milliseconds between each server tick\n\t"
               "--cache-size -s (number): Maximum number of transactions to handle at any given time\n\t"
               "--transaction-duration -t (number): Time in seconds for which a transaction will be stored in cache\n\t"
               "--lease-expiration-check -e (number): Interval in seconds to check for expired leases\n\t"
               "--lease-time -l (number): Default lease time in seconds for the clients\n\t"
               "--log           (number): Specify verbosity of log files (1 - 5)\n\n\t"

               "--pool       -p (range): Specify the IP address range to be used for the DHCP pool. \n\t\t\t"
                        "Format of argument is start_ip:end_ip:subnet_mask. Example: usage\n\t\t\t"
                        "--pool 192.168.0.10:192.168.0.100:255.255.255.0\n\t"
               "--option     -o (option): Add DHCP option to the global list of options.\n\t\t\t"
                        "Format of argument is tag:lenght:value. Example usage\n\t\t\t"
                        "--option 12:6:server this will set option 12 (hostname) to server\n\n\t"

               "--acl-disable            : Disable Access Control List\n\t"
               "--acl-whitelist-mode     : Switch to whitelist mode for Access Control List\n\t"
               "--dynamic-acl-disable    : Disable dynamic updates to Access Control List\n\t"
               "--db-disable             : Disable the use of database for storing detailed transaction information\n"
               , proc_name);
}

static int check_for_version_or_usage_flags(int argc, char *argv[])
{
        for (int i = 0; i < argc; i++) {
                if (!strcmp("--version", argv[i])) {
                        print_program_version();
                        return 1;
                }
                if (!strcmp("-v", argv[i])) {
                        print_program_version();
                        return 1;
                }
                if (!strcmp("--help", argv[i])) {
                        print_program_usage(argv[0]);
                        return 1;
                }
                if (!strcmp("-h", argv[i])) {
                        print_program_usage(argv[0]);
                        return 1;
                }
        }       
        
        return 0;
}

int main(int argc, char *argv[])
{
        /* If one of these flags are present, we want to end the program */
        if (check_for_version_or_usage_flags(argc, argv)) 
                return 0;

        int rv = 1;
        dhcp_server_t dhcp_server = {0};

        mkdir("/var/dhcp", 0744);
        mkdir("/var/dhcp/database", 0744);
        mkdir("/etc/dhcp", 0744);
        mkdir("/etc/dhcp/lease/", 0744);

        strcpy(dhcp_server.config.config_path, CONFIG_DEFAULT_PATH);
        if_failed(init_logging(), exit);
        if_failed(init_allocator(&dhcp_server), exit);
        rv = config_parse_arguments(&dhcp_server, argc, argv);
        if (rv == CONFIG_EXIT_PROGRAM) {
                rv = 0;
                goto exit;
        } else if (rv != 0) {
                rv = 1;
                goto exit;
        }
        if_failed(config_load_configuration(&dhcp_server), exit);
        cclogger_set_verbosity_level(dhcp_server.config.log_verbosity);
        if_failed(init_dhcp_server(&dhcp_server), exit);
        if_failed(init_dhcp_server_timers(&dhcp_server), exit);
        if_failed_n(unix_server_init(&dhcp_server.unix_server), exit);
        if_failed(init_unix_commands(&dhcp_server.unix_server), exit);
        if_failed(init_dhcp_options(&dhcp_server), exit);
        if_failed(init_ACL(&dhcp_server), exit);
        /* dynamic ACL requires cache */
        if_failed(init_cache(&dhcp_server), exit);
        if_failed(init_dynamic_ACL(&dhcp_server), exit);
        /* We need to have address pools and allocator initialised before loading leases */
        if_failed(init_load_persisten_leases(&dhcp_server), exit);

        // _config_dump(&dhcp_server);
        dhcp_server_serve(&dhcp_server);

        unix_server_clean(&dhcp_server.unix_server);
        uninit_dhcp_server(&dhcp_server);
        uninit_logging();

        rv = 0;
exit:
        return rv;
}


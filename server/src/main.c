#include <cclog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "cclog_macros.h"
#include "lease.h"
#include "logging.h"
#include "dhcp_server.h"
#include "unix_server.h"
#include "init.h"
#include "config.h"
#include "utils/xtoy.h"

int main(int argc, char *argv[])
{
        int rv = 1;
        dhcp_server_t dhcp_server = {0};

        mkdir("/var/dhcp", 0744);
        mkdir("/var/dhcp/database", 0744);
        mkdir("/etc/dhcp", 0744);
        mkdir("/etc/dhcp/lease/", 0744);

        // TODO: change way for --version and --help to not require sudo and be handled here 
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
        if_failed(init_dhcp_options(&dhcp_server), exit);
        if_failed(init_ACL(&dhcp_server), exit);
        /* We need to have address pools and allocator initialised before loading leases */
        if_failed(init_load_persisten_leases(&dhcp_server), exit);
        if_failed(init_cache(&dhcp_server), exit);

        dhcp_server_serve(&dhcp_server);

        unix_server_clean(&dhcp_server.unix_server);
        uninit_dhcp_server(&dhcp_server);
        uninit_logging();

        rv = 0;
exit:
        return rv;
}


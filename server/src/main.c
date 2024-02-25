#include <stdio.h>
#include <stdlib.h>

#include "lease.h"
#include "logging.h"
#include "dhcp_server.h"
#include "configuration.h"

int main(int argc, char *argv[])
{
        int rv = 1;
        dhcp_server_t dhcp_server = {0};

        if_failed(init_logging(), exit);
        if_failed(init_dhcp_server(&dhcp_server), exit);
        if_failed(init_dhcp_server_timers(&dhcp_server), exit);
        if_failed(init_allocator(&dhcp_server), exit);
        if_failed(init_address_pools(&dhcp_server), exit);
        if_failed(init_dhcp_options(&dhcp_server), exit);
        /* We need to have address pools and allocator initialised before loading leases */
        if_failed(init_load_persisten_leases(&dhcp_server), exit);
        if_failed(init_cache(&dhcp_server), exit);

        dhcp_server_serve(&dhcp_server);

        uninit_dhcp_server(&dhcp_server);
        uninit_logging();

        rv = 0;
exit:
        return rv;
}

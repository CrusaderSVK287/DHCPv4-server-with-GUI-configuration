#include "dhcp_server.h"
#include "tests.h"
#include "utils/xtoy.h"
#include <netinet/in.h>
#include <security/dhcp_snooping/dhcp_snoop.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utils/llist.h>
#include <cclog_macros.h>

void test_dhcp_snooper()
{
        if (geteuid() != 0) {
                printf("Use sudo to test dhcp snooper\n");
                return;
        }


        dhcp_server_t server = {0};
        strcpy(server.config.interface, "eno1");
        // if_failed(init_dhcp_server(&server), error);
        
        char *msg = NULL;

        llist_t *whitelist = llist_new();
        if_null(whitelist, error);
        // uint32_t address = ipv4_address_to_uint32("192.168.0.1");
        // llist_append(whitelist, &address, false);

        int rv = dhcp_snooper_perform_scan(&server, "a8:a5:fb:b8:61:3d", whitelist, &msg);

        printf("RV = %d\nMSG = %s\n", rv, msg);
        return;
error:
        printf("erorr\n");
}


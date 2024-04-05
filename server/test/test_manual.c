#include "RFC/RFC-2131.h"
#include "dhcp_options.h"
#include "dhcp_packet.h"
#include "dhcp_server.h"
#include "tests.h"
#include "transaction.h"
#include "utils/xtoy.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <security/dhcp_snooping/dhcp_snoop.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/llist.h>
#include <cclog_macros.h>
#include "database.h"

void test_dhcp_snooper();
void test_database();

void test_manual()
{
        // test_dhcp_snooper();
        test_database();
        exit(0);
}

void test_database()
{
        // system("sudo cp /home/lukas/Repositories/dhcp-server/server/test/packet_samples/c7149f6f_b827ebb884c7.dhcp /var/dhcp/database/c7149f6f_b827ebb884c7.dhcp");

        // transaction_t *t = database_load_transaction_str(0xc7149f6f, "b8:27:eb:b8:84:c7");
        // transaction_t *t = database_load_transaction_xid(0xc7149f6f);
        transaction_t *t = database_load_transaction_mac_str("b8:27:eb:b8:84:c7");
        if (!t) {
                puts("t is null");
                return;
        }

        dhcp_message_t *m = NULL;
        llist_foreach(t->messages_ll, {
                m = (dhcp_message_t*)node->data;
                // dhcp_packet_parse(m);

                printf("type: %s", rfc2131_dhcp_message_type_to_str(m->type));
                dhcp_packet_dump(&m->packet);
        })

        return;
}

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


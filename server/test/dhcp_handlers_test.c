#include "allocator.h"
#include "dhcp_packet.h"
#include "messages/dhcprelease.h"
#include "tests.h"
#include "greatest.h"
#include <fcntl.h>
#include <messages/dhcp_messages.h>
#include <lease.h>
#include <utils/xtoy.h>
#include <string.h>
#include <dhcp_server.h>
#include <address_pool.h>

static dhcp_server_t server = {0};


static void setup() {
        server.allocator = address_allocator_new();
        address_pool_t *p = address_pool_new_str("test", "192.168.1.1", "192.168.1.254", "255.255.255.0");
        allocator_add_pool(server.allocator, p);

        server.sock_fd = open("/dev/null", O_RDWR);
}

static void dhcprelease_setup() {
        lease_t *lease = lease_new();
        lease->address = ipv4_address_to_uint32("192.168.1.10");
        lease->pool_name = "test";
        lease->xid = 0x12345678;
        lease->flags = 0;
        lease->subnet = ipv4_address_to_uint32("255.255.255.0");
        lease->lease_start = 10;
        lease->lease_expire = 100;
        uint8_t mac[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        memcpy(lease->client_mac_address, mac, 6);

        uint32_t buf;
        allocator_request_this_address_str(server.allocator, "192.168.1.10", &buf);
        lease_add(lease);
        lease_destroy(&lease);
}

TEST dhcp_release_non_existent_lease() {
        dhcp_message_t m = {0};
        m.ciaddr = ipv4_address_to_uint32("192.168.1.111");

        ASSERT_EQ(-1, message_dhcprelease_handle(&server, &m));

        PASS();
}

TEST dhcp_release_test_valid() {
        
        dhcp_message_t m = {0};
        m.ciaddr = ipv4_address_to_uint32("192.168.1.10");


        address_pool_t *p = (address_pool_t*)(server.allocator->address_pools->first->data);
        ASSERT_EQ(253, p->available_addresses);
        ASSERT_EQ(0b00000010, p->leases_bm[1]);
        ASSERT_EQ(0, message_dhcprelease_handle(&server, &m));
        ASSERT_EQ(254, p->available_addresses);
        ASSERT_EQ(0, p->leases_bm[1]);

        PASS();
}

static void dhcprelease_cleanup() {
        // remove(LEASE_PATH_PREFIX "/test.lease");
}

static void cleanup()
{
        allocator_destroy(&server.allocator);
}

SUITE(dhcp_message_handlers) 
{
        setup();
        dhcprelease_setup();
        RUN_TEST(dhcp_release_non_existent_lease);
        RUN_TEST(dhcp_release_test_valid);
        dhcprelease_cleanup();
        cleanup();
}


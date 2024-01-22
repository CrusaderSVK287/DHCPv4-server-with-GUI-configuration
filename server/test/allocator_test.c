#include "greatest.h"
#include "tests.h"
#include <allocator.h>
#include <dhcp_options.h>
#include <address_pool.h>
#include <stdbool.h>
#include <stdint.h>
#include <utils/xtoy.h>
#include <RFC/RFC-2132.h>

static address_allocator_t *a;

TEST test_create_and_destroy_allocator()
{
        address_allocator_t *allocator = address_allocator_new();
        ASSERT_NEQ(allocator, NULL);

        allocator_destroy(&allocator);
        ASSERT_EQ(allocator, NULL);

        PASS();
}

TEST test_allocator_add_pool()
{
        address_allocator_t *allocator = address_allocator_new();
        ASSERT_NEQ(allocator, NULL);

        address_pool_t *p = address_pool_new_str("test", "192.168.1.1", "192.168.1.254", "255.255.255.0");
        ASSERT_NEQ(p, NULL);

        ASSERT_EQ (ALLOCATOR_OK ,allocator_add_pool(allocator, p));
        ASSERT_NEQ (NULL, allocator->address_pools->first);
        ASSERT_EQ (allocator->address_pools->first, allocator->address_pools->last);

        allocator_destroy(&allocator);
        PASS();
}

TEST test_alloator_request_any_address()
{
        if (!a)
                SKIP();

        uint32_t addr[10];
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+1));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+2));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+3));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+4));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+5));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+6));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+7));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+8));
       ASSERT_EQ(ALLOCATOR_OK, allocator_request_any_address(a, addr+9));

        uint32_t test_addr = ipv4_address_to_uint32("192.168.1.1");

        ASSERT_EQ(test_addr+0, addr[0]);
        ASSERT_EQ(test_addr+1, addr[1]);
        ASSERT_EQ(test_addr+2, addr[2]);
        ASSERT_EQ(test_addr+3, addr[3]);
        ASSERT_EQ(test_addr+4, addr[4]);
        ASSERT_EQ(test_addr+5, addr[5]);
        ASSERT_EQ(test_addr+6, addr[6]);
        ASSERT_EQ(test_addr+7, addr[7]);
        ASSERT_EQ(test_addr+8, addr[8]);
        ASSERT_EQ(test_addr+9, addr[9]);

        /* We know that the first pool will be used in scope of this test */
        ASSERT_EQ(((address_pool_t*)a->address_pools->first->data)->available_addresses, 244);

        PASS();
}

/* This test acts both as a test and as a clear of previous test */
TEST test_allocator_release_addreses()
{
        if (!a)
                SKIP();

        uint32_t addr = ipv4_address_to_uint32("192.168.1.1");

        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 0));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 1));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 2));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 3));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 4));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 5));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 6));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 7));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 8));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr + 9));

        ASSERT_EQ(((address_pool_t*)a->address_pools->first->data)->available_addresses, 254);
        ASSERT_EQ(((address_pool_t*)a->address_pools->first->data)->leases_bm[0], 0);

        PASS();
}

TEST test_allocator_request_this_address()
{
        if (!a)
                SKIP();

        uint32_t addr;
        ASSERT_EQ(ALLOCATOR_OK, allocator_request_this_address_str(a, "192.168.1.111", &addr));
        ASSERT_EQ(ipv4_address_to_uint32("192.168.1.111"), addr);
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr));
        
        PASS();
}

TEST test_allocator_requst_address_from_pool()
{
        if (!a)
                SKIP();

        uint32_t addr;
        ASSERT_EQ(ALLOCATOR_ERROR, allocator_request_address_from_pool(a, "nonexistentpool", &addr));
        ASSERT_EQ(ALLOCATOR_OK, allocator_request_address_from_pool(a, "pool", &addr));

        ASSERT_EQ(addr, ipv4_address_to_uint32("192.168.1.1"));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr));

        PASS();
}

TEST test_allocatore_add_dhcpoption()
{
        if (!a)
                SKIP();

        dhcp_option_t *o = dhcp_option_new();
        dhcp_option_t *o_copy = NULL;
        if (!o)
                SKIP();
        o->tag = DHCP_OPTION_HOST_NAME;
        o->type = DHCP_OPTION_STRING;
        o->lenght = 5;
        strcpy(o->value.string, "hello");

        ASSERT_EQ(ALLOCATOR_OK, allocator_add_dhcp_option(a, o));

        o_copy = dhcp_option_retrieve(a->default_options, DHCP_OPTION_HOST_NAME);
        ASSERT_EQ(o, o_copy);

        PASS();
}

TEST test_allocator_change_dhcp_option()
{
        if (!a)
                SKIP();

        ASSERT_EQ(ALLOCATOR_OPTION_INVALID, allocator_change_dhcp_option(a, 200, "test", 4));
        ASSERT_EQ(ALLOCATOR_OK, allocator_change_dhcp_option(a, DHCP_OPTION_HOST_NAME, "world!", 6));
        dhcp_option_t* o = dhcp_option_retrieve(a->default_options, DHCP_OPTION_HOST_NAME);
        ASSERT_EQ(DHCP_OPTION_HOST_NAME, o->tag);
        ASSERT_EQ(DHCP_OPTION_STRING, o->type);
        ASSERT_EQ(6, o->lenght);
        ASSERT_STR_EQ("world!", o->value.string);

        PASS();
}

TEST test_allocator_is_address_available()
{
        if (!a)
                SKIP();

        ASSERT_EQ(true, allocator_is_address_available_str(a, "192.168.1.1"));

        uint32_t addr;
        ASSERT_EQ(ALLOCATOR_OK, allocator_request_this_address_str(a, "192.168.1.1", &addr));
        ASSERT_EQ(false, allocator_is_address_available_str(a, "192.168.1.1"));
        ASSERT_EQ(addr, ipv4_address_to_uint32("192.168.1.1"));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr));
        ASSERT_EQ(true, allocator_is_address_available_str(a, "192.168.1.1"));
        
        PASS();
}

TEST test_request_already_assigned_address()
{
        if (!a)
                SKIP();

        uint32_t addr;
        ASSERT_EQ(ALLOCATOR_OK, allocator_request_this_address_str(a, "192.168.1.15", &addr));
        ASSERT_EQ(ALLOCATOR_ADDR_IN_USE, allocator_request_this_address_str(a, "192.168.1.15", &addr));
        ASSERT_EQ(ALLOCATOR_OK, allocator_release_address_str(a, "192.168.1.15"));

        PASS();
}

TEST test_release_address_not_in_use()
{
        if (!a)
                SKIP();

        if (!allocator_is_address_available_str(a, "192.168.1.15"))
                SKIP();

        ASSERT_EQ(ALLOCATOR_ADDR_NOT_IN_USE, allocator_release_address_str(a, "192.168.1.15"));

        PASS();
}

TEST test_allocator_exhaust_pool()
{
        if (!a)
                SKIP();

        /* Allocate 10 addresses from small_pool*/
        uint32_t addr[20];
        for (int i = 0; i < 10; i++) {
                ASSERT_EQ(ALLOCATOR_OK, allocator_request_address_from_pool(a, "small_pool", addr+i));
        }

        ASSERT_EQ(ALLOCATOR_POOL_DEPLETED, allocator_request_address_from_pool(a, "small_pool", addr+10));

        for (int i = 0; i < 10; i++) {
                ASSERT_EQ(ALLOCATOR_OK, allocator_release_address(a, addr[i]));
        }

        PASS();
}

TEST test_allocator_add_duplicite_pool()
{
        if (!a)
                SKIP();

        address_pool_t *p = address_pool_new_str("pool", "1.1.1.1", "1.1.1.2", "0.0.0.0");
        ASSERT_EQ(ALLOCATOR_POOL_DUPLICITE, allocator_add_pool(a, p));
        address_pool_destroy(&p);

        PASS();
}

TEST test_allocator_add_duplicite_option()
{
        if (!a)
                SKIP();

        /* We assume the hostname option is still in allocator */
        if (!dhcp_option_retrieve(a->default_options, DHCP_OPTION_HOST_NAME))
                SKIP();

        dhcp_option_t *o = dhcp_option_new();
        if (!o)
                SKIP();
        o->tag = DHCP_OPTION_HOST_NAME;
        o->type = DHCP_OPTION_STRING;
        o->lenght = 5;
        strcpy(o->value.string, "hello");

        ASSERT_EQ(ALLOCATOR_OPTION_DUPLICITE, allocator_add_dhcp_option(a, o));

        dhcp_option_destroy(&o);

        PASS();
}

TEST test_get_pool_by_name()
{
        if (!a)
                SKIP();

        ASSERT_NEQ(NULL, allocator_get_pool_by_name(a, "pool"));
        ASSERT_NEQ(NULL, allocator_get_pool_by_name(a, "small_pool"));
        ASSERT_EQ(NULL, allocator_get_pool_by_name(a, "nonexistentpool"));

        PASS();
}

TEST test_get_pool_by_address()
{
        if (!a)
                SKIP();

        address_pool_t *p = allocator_get_pool_by_address(a, ipv4_address_to_uint32("192.168.1.100"));
        ASSERT_NEQ(NULL, p);
        ASSERT_STR_EQ("pool", p->name);

        p = allocator_get_pool_by_address(a, ipv4_address_to_uint32("192.168.10.15"));
        ASSERT_NEQ(NULL, p);
        ASSERT_STR_EQ("small_pool", p->name);

        p = allocator_get_pool_by_address(a, ipv4_address_to_uint32("100.100.100.150"));
        ASSERT_EQ(NULL, p);
        
        PASS();
}

SUITE(allocator)
{
        a = address_allocator_new();
        address_pool_t *p =  NULL;
        if (a) {
                p = address_pool_new_str("pool", "192.168.1.1", "192.168.1.254", "255.255.255.0");
                if (allocator_add_pool(a, p) != ALLOCATOR_OK)
                        return;
                p = address_pool_new_str("small_pool", "192.168.10.10", "192.168.10.19", "255.255.255.224");
                if (allocator_add_pool(a, p) != ALLOCATOR_OK)
                        return;
        }

        RUN_TEST(test_create_and_destroy_allocator);
        RUN_TEST(test_allocator_add_pool);
        RUN_TEST(test_alloator_request_any_address);
        RUN_TEST(test_allocator_release_addreses);
        RUN_TEST(test_allocator_request_this_address);
        RUN_TEST(test_allocator_requst_address_from_pool);
        RUN_TEST(test_allocatore_add_dhcpoption);
        RUN_TEST(test_allocator_change_dhcp_option);
        RUN_TEST(test_allocator_is_address_available);
        RUN_TEST(test_allocator_exhaust_pool);
        RUN_TEST(test_allocator_add_duplicite_pool);
        RUN_TEST(test_allocator_add_duplicite_option);
        RUN_TEST(test_request_already_assigned_address);
        RUN_TEST(test_release_address_not_in_use);
        RUN_TEST(test_get_pool_by_name);
        RUN_TEST(test_get_pool_by_address);

        if (a)
                allocator_destroy(&a);
}


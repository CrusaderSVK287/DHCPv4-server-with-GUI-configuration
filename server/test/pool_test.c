#include "RFC/RFC-2132.h"
#include "dhcp_options.h"
#include "tests.h"
#include "greatest.h"
#include <address_pool.h>
#include <stdio.h>
#include <utils/xtoy.h>

TEST test_create_new_pool_and_destroy_it()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.1", "192.168.1.10", "255.255.255.0");
        ASSERT_STR_EQ("test", pool->name);

        address_pool_destroy(&pool);
        ASSERT_EQ(pool, NULL);

        PASS();
}

TEST test_create_pool_switched_addresses()
{
        // switch start and end address
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.254", "192.168.1.1", "255.255.255.0");
        ASSERT_EQ(NULL, pool);

        PASS();
}

TEST test_create_pool_valid_range_edge()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.1", "192.168.1.254", "255.255.255.0");
        ASSERT_STR_EQ("test", pool->name);

        address_pool_destroy(&pool);
        ASSERT_EQ(pool, NULL);

        PASS();
}

TEST test_create_pool_valid_range_dhcp_option_present()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.1", "192.168.1.254", "255.255.255.0");
        ASSERT_STR_EQ("test", pool->name);

        dhcp_option_t *option = dhcp_option_retrieve(pool->dhcp_option_override, DHCP_OPTION_SUBNET_MASK);
        ASSERT_NEQ(NULL, option);
        ASSERT_EQ(DHCP_OPTION_SUBNET_MASK, option->tag);
        ASSERT_EQ(ipv4_address_to_uint32("255.255.255.0"), option->value.ip);

        address_pool_destroy(&pool);
        ASSERT_EQ(pool, NULL);

        PASS();
}

TEST test_create_pool_invalid_range_edge()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.0", "192.168.1.255", "255.255.255.0");
        ASSERT_EQ(NULL, pool);

        address_pool_destroy(&pool);
        PASS();
}

TEST test_create_pool_invalid_range()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.5.130", "192.168.8.138", "255.255.255.0");
        ASSERT_EQ(NULL, pool);

        address_pool_destroy(&pool);
        PASS();
}

TEST test_pool_allocate_address()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.1", "192.168.1.100", "255.255.255.0");
        ASSERT_NEQ(NULL, pool);
        ASSERT_EQ(100, pool->available_addresses);

        int rv = address_pool_set_address_allocation_str(pool, "192.168.1.11"); // allocate 11th address (byte 2, bit 3)
        ASSERT_EQ(0, rv);
        ASSERT_EQ(99, pool->available_addresses);

        // we expect address to be used
        ASSERT_EQ(1, address_pool_get_address_allocation_str(pool, "192.168.1.11"));
        // we expect second byte of of the bitmask to have only 3rd bit set to 1
        ASSERT_EQ(0b00000100, pool->leases_bm[1]);

        rv = address_pool_clear_address_allocation_str(pool, "192.168.1.11");

        // we expect address to be free again
        ASSERT_EQ(0, address_pool_get_address_allocation_str(pool, "192.168.1.11"));
        // we expect second byte of of the bitmask to have only 3rd bit set to 1
        ASSERT_EQ(0b00000000, pool->leases_bm[1]);
        ASSERT_EQ(100, pool->available_addresses);
        
        address_pool_destroy(&pool);
        PASS();
}

TEST test_pool_allocate_address_edge_cases()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.1", "192.168.1.254", "255.255.255.0");
        ASSERT_NEQ(NULL, pool);
        ASSERT_EQ(254, pool->available_addresses);

        int rv = address_pool_set_address_allocation_str(pool, "192.168.1.1"); // allocate 11th address (byte 2, bit 3)
        ASSERT_EQ(0, rv);
        rv = address_pool_set_address_allocation_str(pool, "192.168.1.254"); // allocate 11th address (byte 2, bit 3)
        ASSERT_EQ(0, rv);
        ASSERT_EQ(252, pool->available_addresses);

        ASSERT_EQ(1, address_pool_get_address_allocation_str(pool, "192.168.1.1"));
        ASSERT_EQ(1, address_pool_get_address_allocation_str(pool, "192.168.1.254"));

        ASSERT_EQ(0b00000001, pool->leases_bm[0]);
        ASSERT_EQ(0b00100000, pool->leases_bm[31]);

        address_pool_destroy(&pool);
        PASS();
}

SUITE(pool)
{
        RUN_TEST(test_create_new_pool_and_destroy_it);
        RUN_TEST(test_create_pool_invalid_range);
        RUN_TEST(test_create_pool_invalid_range_edge);
        RUN_TEST(test_create_pool_valid_range_edge);
        RUN_TEST(test_pool_allocate_address);
        RUN_TEST(test_pool_allocate_address_edge_cases);
        RUN_TEST(test_create_pool_switched_addresses);
        RUN_TEST(test_create_pool_valid_range_dhcp_option_present);
}


#include "tests.h"
#include "greatest.h"
#include <address_pool.h>
#include <utils/xtoy.h>

TEST test_create_new_pool_and_destroy_it()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.1", "192.168.1.10", "255.255.255.0");
        ASSERT_STR_EQ("test", pool->name);

        address_pool_destroy(&pool);
        ASSERT_EQ(pool, NULL);

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

TEST test_create_pool_invalid_range_edge()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.1.0", "192.168.1.255", "255.255.255.0");
        ASSERT_EQ(NULL, pool);

        PASS();
}

TEST test_create_pool_invalid_range()
{
        address_pool_t *pool = address_pool_new_str("test", "192.168.5.130", "192.168.8.138", "255.255.255.0");
        ASSERT_EQ(NULL, pool);

        PASS();

}

SUITE(pool)
{
        RUN_TEST(test_create_new_pool_and_destroy_it);
        RUN_TEST(test_create_pool_invalid_range);
        RUN_TEST(test_create_pool_invalid_range_edge);
        RUN_TEST(test_create_pool_valid_range_edge);
}


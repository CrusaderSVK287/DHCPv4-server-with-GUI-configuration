#include "greatest.h"
#include "tests.h"
#include <stdio.h>
#include <utils/xtoy.h>
#include <stdint.h>

TEST test_util_uint32_to_ipv4_string()
{
        uint32_t a = 0x70605040;
        ASSERT_STR_EQ("112.96.80.64", uint32_to_ipv4_address(a));

        PASS();
}

TEST test_util_ipov4_string_to_uint32()
{
        uint32_t a = ipv4_address_to_uint32("112.96.80.64");
        ASSERT_EQ(a, 0x70605040);
        PASS();
}

SUITE(utils)
{
        RUN_TEST(test_util_ipov4_string_to_uint32);
        RUN_TEST(test_util_uint32_to_ipv4_string);
}


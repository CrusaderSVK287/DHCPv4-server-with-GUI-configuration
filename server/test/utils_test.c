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

TEST test_util_uint8_to_mac()
{
        uint8_t mac[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        ASSERT_STR_EQ("aa:bb:cc:dd:ee:ff", uint8_array_to_mac(mac));
        PASS();
}

TEST test_util_mac_to_uint8()
{
        uint8_t mac[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        uint8_t buf[6] = {0};
        mac_to_uint8_array("aa:bb:cc:dd:ee:ff", buf);

        ASSERT_MEM_EQ(mac, buf, 6);
        PASS();
}

SUITE(utils)
{
        RUN_TEST(test_util_ipov4_string_to_uint32);
        RUN_TEST(test_util_uint32_to_ipv4_string);
        RUN_TEST(test_util_uint8_to_mac);
        RUN_TEST(test_util_mac_to_uint8);
}


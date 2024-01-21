#include <fcntl.h>
#include <lease.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
/* tests.h include redefinition of LEASE_PATH_PREFIX */
#include "address_pool.h"
#include "cJSON.h"
#include "tests.h"
#include "greatest.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include <unistd.h>

static int lease_path_ok = -1;
TEST test_undef_lease_path_for_testing()
{
        if (strcmp("./test/test_leases/", LEASE_PATH_PREFIX) != 0) {
                PASS();
        }
        mkdir(LEASE_PATH_PREFIX, 0777);
        lease_path_ok = 0;
        PASS();
}

TEST test_lease_file_init()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        if (stat(LEASE_PATH_PREFIX "test_pool.lease", &st) >= 0) {
                remove(LEASE_PATH_PREFIX "test_pool.lease");
        }

        ASSERT_EQ(-1, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        lease_t l = {
                .address = ipv4_address_to_uint32("192.168.1.10"),
                .subnet  = ipv4_address_to_uint32("255.255.255.0"),
                .xid     = 0x80706050,
                .lease_start = 100,
                .lease_expire = 1000,
                .pool_name = "test_pool",
                .flags = 0,
        };
        uint8_t mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        memcpy(l.client_mac_address, mac, 6);

        ASSERT_EQ(LEASE_OK, lease_add(&l));
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        PASS();
}

// this test also has a role to setup the .lease file for future tests
TEST test_add_leases()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        lease_t l1 = {
                .address = ipv4_address_to_uint32("192.168.1.186"),
                .subnet  = ipv4_address_to_uint32("255.255.255.128"),
                .xid     = 0x616ab16f,
                .lease_start = 454148657,
                .lease_expire = 547784223,
                .pool_name = "test_pool",
                .flags = 0,
        };
        lease_t l2 = {
                .address = ipv4_address_to_uint32("192.168.1.33"),
                .subnet  = ipv4_address_to_uint32("255.255.255.128"),
                .xid     = 0xa156bf84,
                .lease_start = 60,
                .lease_expire = 60000,
                .pool_name = "test_pool",
                .flags = 0,
        };
        uint8_t mac1[] = {0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1};
        memcpy(l1.client_mac_address, mac1, 6);
        uint8_t mac2[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        memcpy(l2.client_mac_address, mac2, 6);

        ASSERT_EQ(LEASE_OK, lease_add(&l1));
        ASSERT_EQ(LEASE_OK, lease_add(&l2));

        PASS();
}

TEST test_are_3_leases_present()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        int fd = open(LEASE_PATH_PREFIX "test_pool.lease", O_RDONLY);
        if (fd < 0)
                FAIL();

        char buf[8000];
        memset(buf, 0, 8000);
        if (read(fd, buf, 8000) < 0)
                FAIL();

        close(fd);

        cJSON *root = cJSON_Parse(buf);
        ASSERT_NEQ(NULL, root);
        cJSON *array = cJSON_GetObjectItem(root, "leases");
        ASSERT_NEQ(NULL, array);
        ASSERT_EQ(3, cJSON_GetArraySize(array));

        cJSON_Delete(root);

        PASS();
}

// lease_t l1 = {
//         .address = ipv4_address_to_uint32("192.168.1.186"),
//         .subnet  = ipv4_address_to_uint32("255.255.255.128"),
//         .xid     = 0x616ab16f,
//         .lease_start = 454148657,
//         .lease_expire = 547784223,
//         .pool_name = "test_pool",
//         .flags = 0,
// };
TEST test_retrieve_lease()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        lease_t l = {0};
        ASSERT_EQ(LEASE_OK, lease_retrieve(&l, ipv4_address_to_uint32("192.168.1.186"), "test_pool"));

        ASSERT_EQ(l.address, ipv4_address_to_uint32("192.168.1.186"));
        ASSERT_EQ(l.subnet, ipv4_address_to_uint32("255.255.255.128"));
        ASSERT_EQ(l.xid, 0x616ab16f);
        ASSERT_EQ(l.lease_start, 454148657);
        ASSERT_EQ(l.lease_expire, 547784223);
        ASSERT_EQ(l.flags, 0);

        PASS();
}

TEST test_retrieve_lease_address()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        llist_t *pools = llist_new();
        address_pool_t *p = address_pool_new_str("test_pool", "192.168.1.1", "192.168.1.254", "255.255.255.0");
        ASSERT_EQ(0, llist_append(pools, p, false));



        lease_t l = {0};
        ASSERT_EQ(LEASE_OK, lease_retrieve_address(&l, ipv4_address_to_uint32("192.168.1.186"), pools));

        ASSERT_EQ(l.address, ipv4_address_to_uint32("192.168.1.186"));
        ASSERT_EQ(l.subnet, ipv4_address_to_uint32("255.255.255.128"));
        ASSERT_EQ(l.xid, 0x616ab16f);
        ASSERT_EQ(l.lease_start, 454148657);
        ASSERT_EQ(l.lease_expire, 547784223);
        ASSERT_EQ(l.flags, 0);

        llist_destroy(&pools);
        address_pool_destroy(&p);

        PASS();
}

TEST test_retrieve_non_existent()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        lease_t l = {0};
        ASSERT_EQ(LEASE_DOESNT_EXITS, lease_retrieve(&l, ipv4_address_to_uint32("192.168.1.222"), "test_pool"));

        PASS();
}

TEST test_remove_lease()
{
        if (lease_path_ok < 0)
                SKIP();

        struct stat st;
        ASSERT_EQ(0, stat(LEASE_PATH_PREFIX "test_pool.lease", &st));

        lease_t l = {0};
        ASSERT_EQ(LEASE_OK, lease_remove_address_pool(ipv4_address_to_uint32("192.168.1.186"), "test_pool"));
        ASSERT_EQ(LEASE_DOESNT_EXITS, lease_retrieve(&l, ipv4_address_to_uint32("192.168.1.186"), "test_pool"));

        PASS();
}

SUITE(lease)
{
        RUN_TEST(test_undef_lease_path_for_testing);
        if (lease_path_ok < 0) {
                printf("LEASE_PATH_PREFIX does not equal ./test/test_leases, skipping lease tests\n"
                                "Uncomment line 8 (#define LEASES_TEST_BUILD)"
                                "of file lease.h to enabled lease tests\n");
        }

        RUN_TEST(test_lease_file_init);
        RUN_TEST(test_add_leases);
        RUN_TEST(test_are_3_leases_present);
        RUN_TEST(test_retrieve_lease);
        RUN_TEST(test_retrieve_lease_address);
        RUN_TEST(test_retrieve_non_existent);
        RUN_TEST(test_remove_lease);
}


#include "tests.h"
#include "greatest.h"
#include "utils/llist.h"
#include <security/acl.h>
#include <stdint.h>
#include <stdio.h>

#define IF_ACL_NULL_SKIP if (!acl || !acl->entries) {SKIP();}

static ACL_t* acl;

TEST test_security_ACL_new()
{
        acl = ACL_new();
        ASSERT_NEQ(NULL, acl);
        ASSERT_NEQ(NULL, acl->entries);
        
        acl->enabled = true;
        acl->is_blacklist = true;

        PASS();
}

TEST test_security_ACL_load_acl_entries()
{
        IF_ACL_NULL_SKIP

        ASSERT_EQ(ACL_OK, ACL_load_acl_entries(acl, "./test/config_sample.json"));
        ASSERT_NEQ(NULL, acl->entries->first);
        ASSERT_NEQ(NULL, acl->entries->last);
        char *first = (char*)acl->entries->first->data;
        char *last = (char*)acl->entries->last->data;
        
        ASSERT_STR_EQ("aa:bb:cc:dd:ee:ff", first);
        ASSERT_STR_EQ("11:22:33:44:55:66", last);

        PASS();
}

TEST test_security_ACL_allow_and_deny_client()
{
        IF_ACL_NULL_SKIP

        uint8_t mac_deny1[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        uint8_t mac_deny2[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        uint8_t mac_allow[] = {0xaa, 0xbb, 0xcc, 0x11, 0x22, 0x33};

        ASSERT_EQ(ACL_DENY , ACL_check_client(acl, mac_deny1));
        ASSERT_EQ(ACL_DENY , ACL_check_client(acl, mac_deny2));
        ASSERT_EQ(ACL_ALLOW, ACL_check_client(acl, mac_allow));

        PASS();
}

TEST test_security_ACL_allow_and_deny_client_str()
{
        IF_ACL_NULL_SKIP

        ASSERT_EQ(ACL_DENY , ACL_check_client_str(acl, "aa:bb:cc:dd:ee:ff"));
        ASSERT_EQ(ACL_DENY , ACL_check_client_str(acl, "11:22:33:44:55:66"));
        ASSERT_EQ(ACL_ALLOW, ACL_check_client_str(acl, "aa:bb:cc:11:22:33"));
        
        PASS();
}

TEST test_security_ACL_allow_all_ACL_off()
{
        IF_ACL_NULL_SKIP

        acl->enabled = false;

        ASSERT_EQ(ACL_ALLOW, ACL_check_client_str(acl, "aa:bb:cc:dd:ee:ff"));
        ASSERT_EQ(ACL_ALLOW, ACL_check_client_str(acl, "11:22:33:44:55:66"));
        ASSERT_EQ(ACL_ALLOW, ACL_check_client_str(acl, "aa:bb:cc:11:22:33"));
        
        PASS();
}

TEST test_security_ACL_destroy()
{
        IF_ACL_NULL_SKIP
        ACL_destroy(&acl);
        ASSERT_EQ(NULL, acl); 

        PASS();
}

SUITE(security)
{
        RUN_TEST(test_security_ACL_new);
        RUN_TEST(test_security_ACL_load_acl_entries);
        RUN_TEST(test_security_ACL_allow_and_deny_client);
        RUN_TEST(test_security_ACL_allow_and_deny_client_str);
        RUN_TEST(test_security_ACL_allow_all_ACL_off);
        if (acl) acl->enabled = true;

        /* must be run AFTER all tests utilising acl strucutre */
        RUN_TEST(test_security_ACL_destroy);
}


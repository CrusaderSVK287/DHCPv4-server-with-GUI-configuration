#include <lease.h>
#include <sys/stat.h>
/* tests.h include redefinition of LEASE_PATH_PREFIX */
#include "tests.h"
#include "greatest.h"

static int lease_path_ok = -1;
TEST test_undef_lease_path_for_testing()
{
        ASSERT_STR_EQ("./test/test_leases", LEASE_PATH_PREFIX);
        mkdir(LEASE_PATH_PREFIX, 0777);
        lease_path_ok = 0;
        PASS();
}

SUITE(lease)
{
        RUN_TEST(test_undef_lease_path_for_testing);
        if (lease_path_ok < 0)
                return;
}


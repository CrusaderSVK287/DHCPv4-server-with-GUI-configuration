#include "greatest.h"
#include "tests.h"
#include <logging.h>

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[])
{

        GREATEST_MAIN_BEGIN();

        init_logging();

        RUN_SUITE(linked_list);
        RUN_SUITE(dhcp_options);
        RUN_SUITE(utils);
        RUN_SUITE(pool);

        uninit_logging();

        GREATEST_MAIN_END();
        return 0;
}

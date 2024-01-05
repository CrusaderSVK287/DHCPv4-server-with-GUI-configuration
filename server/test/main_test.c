#include "greatest.h"
#include "tests.h"

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[])
{

        GREATEST_MAIN_BEGIN();

        RUN_SUITE(linked_list);
        RUN_SUITE(dhcp_options);
        RUN_SUITE(utils);

        GREATEST_MAIN_END();
        return 0;
}

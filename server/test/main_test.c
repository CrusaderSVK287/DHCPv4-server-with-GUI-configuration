#include "greatest.h"
#include "tests.h"
#include <cclog.h>

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[])
{

        GREATEST_MAIN_BEGIN();

        /* 
         * Set up a dummy logger that doesnt log anything, also removes 
         * the need to compile tests as root
         */
        cclogger_init(LOGGING_SINGLE_FILE, "./test_log_file", "dhcps_test");
        cclogger_add_log_level(false, false, CCLOG_TTY_CLR_GRN, NULL, NULL, 100);
        cclogger_add_log_level(false, false, CCLOG_TTY_CLR_GRN, NULL, NULL, 100);
        cclogger_add_log_level(false, false, CCLOG_TTY_CLR_GRN, NULL, NULL, 100);
        cclogger_set_verbosity_level(-1000);
        
        // test_manual();

        RUN_SUITE(linked_list);
        RUN_SUITE(dhcp_options);
        RUN_SUITE(utils);
        RUN_SUITE(pool);
        RUN_SUITE(allocator);
        RUN_SUITE(packet_parser_builder);
        RUN_SUITE(lease);
        RUN_SUITE(dhcp_message_handlers);
        RUN_SUITE(transaction);
        RUN_SUITE(timer); // this suite takes some time to run, no need to run it always
        RUN_SUITE(config);
        RUN_SUITE(security);

        cclogger_uninit();

        GREATEST_MAIN_END();
        return 0;
}

#ifndef SERVER_TESTS
#define SERVER_TESTS

#include "greatest.h"

// #define __RUN_TIMER_TESTS__

SUITE(linked_list);
SUITE(dhcp_options);
SUITE(utils);
SUITE(pool);
SUITE(allocator);
SUITE(packet_parser_builder);
SUITE(lease);
SUITE(dhcp_message_handlers);
SUITE(transaction);
SUITE(timer);
SUITE(config);
SUITE(security);

#ifdef __PIPELINE_BUILD
#define SKIP_IF_PIPELINE_BUILD SKIP();
#else 
#define SKIP_IF_PIPELINE_BUILD
#endif

#ifndef  __RUN_TIMER_TESTS__
#define SKIP_TIMER_TESTS SKIP()
#else 
#define SKIP_TIMER_TESTS
#endif

#define INTERFACE "eno1"

#endif // !SERVER_TESTS

#ifndef SERVER_TESTS
#define SERVER_TESTS

#include "greatest.h"

#ifdef LEASE_PATH_PREFIX
#undef LEASE_PATH_PREFIX
#define LEASE_PATH_PREFIX "./test/test_leases"
#endif // LEASE_PATH_PREFIX

SUITE(linked_list);
SUITE(dhcp_options);
SUITE(utils);
SUITE(pool);
SUITE(allocator);
SUITE(packet_parser_builder);
SUITE(lease);

#endif // !SERVER_TESTS

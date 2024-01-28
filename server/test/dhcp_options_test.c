#include "greatest.h"
#include "tests.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include <cclog_macros.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <RFC/RFC-2132.h>
#include <dhcp_options.h>

static uint8_t raw_dhcp_options[DHCP_PACKET_OPTIONS_SIZE];
static llist_t *parsed_raw_options = NULL;

TEST test_new_and_destroy()
{
        dhcp_option_t *o = dhcp_option_new();
        o->tag = DHCP_OPTION_DHCP_MESSAGE_TYPE;
        dhcp_option_destroy(&o);

        ASSERT_EQ(o, NULL);
        PASS();
}

TEST test_new_with_parameters()
{
        uint32_t lease_time = 86400;
        dhcp_option_t *o = dhcp_option_new_values(DHCP_OPTION_IP_ADDRESS_LEASE_TIME,
                                                  4, &lease_time);
        ASSERT_NEQ(NULL, o);
        ASSERT_EQ(DHCP_OPTION_IP_ADDRESS_LEASE_TIME, o->tag);
        ASSERT_EQ(DHCP_OPTION_NUMERIC, o->type);
        ASSERT_EQ(4, o->lenght);
        ASSERT_EQ(lease_time, o->value.number);

        uint32_t ip = ipv4_address_to_uint32("192.168.5.20");
        dhcp_option_t *o_ip = dhcp_option_new_values(DHCP_OPTION_SERVER_IDENTIFIER,
                                                  4, &ip);
        ASSERT_NEQ(NULL, o_ip);
        ASSERT_EQ(DHCP_OPTION_SERVER_IDENTIFIER, o_ip->tag);
        ASSERT_EQ(DHCP_OPTION_IP, o_ip->type);
        ASSERT_EQ(4, o_ip->lenght);
        ASSERT_EQ(ip, o_ip->value.ip);

        dhcp_option_destroy(&o);
        ASSERT_EQ(NULL, o);
        dhcp_option_destroy(&o_ip);
        ASSERT_EQ(NULL, o_ip);
        PASS();
}

TEST test_parse_options()
{
        llist_t *options = llist_new();
        
        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x35);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->tag, DHCP_OPTION_DHCP_MESSAGE_TYPE);
        ASSERT_EQ(o->value.number, 3);

        o = dhcp_option_retrieve(options, 0x32);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->tag, DHCP_OPTION_REQUESTED_IP_ADDRESS);
        ASSERT_EQ(o->lenght, 4);
        ASSERT_EQ(o->value.ip, 0x70605040);
        ASSERT_STR_EQ("112.96.80.64", uint32_to_ipv4_address(o->value.ip));

        /* Used in another test, freed there */
        parsed_raw_options = options;

        PASS();
}

TEST test_serialize_options() 
{
        if_null(parsed_raw_options, skip);

        uint8_t serialized[DHCP_PACKET_OPTIONS_SIZE];
        memset(serialized, 0, DHCP_PACKET_OPTIONS_SIZE);
        ASSERT_EQ(0, dhcp_options_serialize(parsed_raw_options, serialized));

        ASSERT_MEM_EQ(raw_dhcp_options, serialized, sizeof(raw_dhcp_options));

        dhcp_option_destroy_list(&parsed_raw_options);
        PASS();
skip:
        SKIP();
}

TEST test_retrieve_option_by_tag()
{
        llist_t *options = llist_new();

        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x35);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->tag, DHCP_OPTION_DHCP_MESSAGE_TYPE);

        o = dhcp_option_retrieve(options, 0x32);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->tag, DHCP_OPTION_REQUESTED_IP_ADDRESS);

        dhcp_option_destroy_list(&options);

        PASS();
}

TEST test_add_option_to_linked_list() 
{
        llist_t *options = llist_new();

        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_new();
        o->type = DHCP_OPTION_STRING;
        o->tag = DHCP_OPTION_DOMAIN_NAME_SERVERS;
        strcpy(o->value.string, "192.168.1.1");
        o->lenght = strlen("192.168.1.1");

        dhcp_option_add(options, o);

        o = dhcp_option_retrieve(options, DHCP_OPTION_DOMAIN_NAME_SERVERS);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->tag, DHCP_OPTION_DOMAIN_NAME_SERVERS);
        ASSERT_EQ(o->type, DHCP_OPTION_STRING);
        ASSERT_STR_EQ(o->value.string, "192.168.1.1");

        dhcp_option_destroy_list(&options);

        PASS();
}

TEST test_parsed_option_numeric()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);
        
        dhcp_option_t *o = dhcp_option_retrieve(options, 0x35);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->value.number, 0x03);

        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_parsed_option_ip()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x32);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->value.ip, 0x70605040);

        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_parsed_option_boolean()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x1f);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_BOOL);
        ASSERT_EQ(o->value.boolean, true);

        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_parsed_option_string()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x0c);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_STRING);
        ASSERT_STRN_EQ(o->value.string, "MyDevice", o->lenght);

        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_parsed_option_binary()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);
        uint8_t expected[] = {0x32, 0x1f, 0x0c};

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x37);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_BIN);
        ASSERT_MEM_EQ(o->value.binary_data, expected, o->lenght);

        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_parsed_option_numeric_with_multiple_bytes()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x33);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->value.number, 0x00015180);

        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_parsed_option_ip_trailing_and_leading_zeros()
{
        llist_t *options = llist_new();
        dhcp_option_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x01);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->value.ip, 0x70000000);

        o = dhcp_option_retrieve(options, 0x03);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->value.ip, 0x00005040);
        
        dhcp_option_destroy_list(&options);
        PASS();
}

TEST test_duplicite_option()
{
        uint32_t lease_time = 86400;
        dhcp_option_t *o = dhcp_option_new_values(DHCP_OPTION_IP_ADDRESS_LEASE_TIME,
                                                  4, &lease_time);
        ASSERT_NEQ(NULL, o);
        ASSERT_EQ(DHCP_OPTION_IP_ADDRESS_LEASE_TIME, o->tag);
        ASSERT_EQ(DHCP_OPTION_NUMERIC, o->type);
        ASSERT_EQ(4, o->lenght);
        ASSERT_EQ(lease_time, o->value.number);

        llist_t *option_list = llist_new();
        ASSERT_EQ(0, dhcp_option_add(option_list, o));
        ASSERT_EQ(0, dhcp_option_add(option_list, o));
        ASSERT_EQ(0, dhcp_option_add(option_list, o));
        ASSERT_EQ(0, dhcp_option_add(option_list, o));

        ASSERT_EQ(option_list->first, option_list->last);

        dhcp_option_destroy_list(&option_list);
        PASS();
}

SUITE(dhcp_options)
{
        memset(raw_dhcp_options, 0, sizeof(dhcp_options));
        uint8_t test_options[] = {
                0x35, 0x01, 0x03, //numeric -> DHCP message type
                0x32, 0x04, 0x70, 0x60, 0x50, 0x40, //ip -> Requested IP address
                0x01, 0x04, 0x70, 0x00, 0x00, 0x00, //ip -> subnet mask .0.0.0 at end
                0x03, 0x04, 0x00, 0x00, 0x50, 0x40, //ip -> router option 0.0. at beggining
                0x1f, 0x01, 0x01, //boolean -> DHCP option ip forwarding enable disable with value 1 (true)
                0x0c, 0x08, 'M', 'y', 'D', 'e', 'v', 'i', 'c', 'e', //string -> domain_name
                0x37, 0x03, 0x32, 0x1f, 0x0c, //binary -> requested parameters (the ones in previous lines without 0x35);
                0x33, 0x04, 0x00, 0x01, 0x51, 0x80, // numeric, big number -> lease time, 86400 in dec

                0xff
        };

        memcpy(raw_dhcp_options, test_options, sizeof(test_options));
        
        RUN_TEST(test_new_and_destroy);
        RUN_TEST(test_new_with_parameters);
        RUN_TEST(test_parse_options);
        RUN_TEST(test_serialize_options);
        RUN_TEST(test_retrieve_option_by_tag);
        RUN_TEST(test_add_option_to_linked_list);

        RUN_TEST(test_parsed_option_numeric);
        RUN_TEST(test_parsed_option_ip);
        RUN_TEST(test_parsed_option_boolean);
        RUN_TEST(test_parsed_option_string);
        RUN_TEST(test_parsed_option_binary);
        RUN_TEST(test_parsed_option_numeric_with_multiple_bytes);
        RUN_TEST(test_parsed_option_ip_trailing_and_leading_zeros);
        RUN_TEST(test_duplicite_option);
}


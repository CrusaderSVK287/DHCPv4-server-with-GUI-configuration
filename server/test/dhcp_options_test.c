#include "greatest.h"
#include "tests.h"
#include "utils/xtoy.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <RFC/RFC-2132.h>
#include <dhcp_options.h>

static uint8_t raw_dhcp_options[DHCP_PACKET_OPTIONS_SIZE];

TEST test_new_and_destroy()
{
        dhcp_option_t *o = dhcp_option_new();
        o->tag = DHCP_OPTION_DHCP_MESSAGE_TYPE;
        dhcp_option_destroy(&o);

        ASSERT_EQ(o, NULL);
        PASS();
}

TEST test_parse_raw_options()
{
        llist_t *options = llist_new();
        
        dhcp_option_raw_parse(options, raw_dhcp_options);

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

        llist_destroy(&options);

        PASS();
}

TEST test_retrieve_option_by_tag()
{
        llist_t *options = llist_new();

        dhcp_option_raw_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x35);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->tag, DHCP_OPTION_DHCP_MESSAGE_TYPE);

        o = dhcp_option_retrieve(options, 0x32);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->tag, DHCP_OPTION_REQUESTED_IP_ADDRESS);

        llist_destroy(&options);

        PASS();
}

TEST test_add_option_to_linked_list() 
{
        llist_t *options = llist_new();

        dhcp_option_raw_parse(options, raw_dhcp_options);

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

        llist_destroy(&options);

        PASS();
}

TEST test_parsed_option_numeric()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);
        
        dhcp_option_t *o = dhcp_option_retrieve(options, 0x35);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->value.number, 0x03);

        llist_destroy(&options);
        PASS();
}

TEST test_parsed_option_ip()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x32);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->value.ip, 0x70605040);

        llist_destroy(&options);
        PASS();
}

TEST test_parsed_option_boolean()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x1f);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_BOOL);
        ASSERT_EQ(o->value.boolean, true);

        llist_destroy(&options);
        PASS();
}

TEST test_parsed_option_string()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x0c);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_STRING);
        ASSERT_STRN_EQ(o->value.string, "MyDevice", o->lenght);

        llist_destroy(&options);
        PASS();
}

TEST test_parsed_option_binary()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);
        uint8_t expected[] = {0x32, 0x1f, 0x0c};

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x37);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_BIN);
        ASSERT_MEM_EQ(o->value.binary_data, expected, o->lenght);

        llist_destroy(&options);
        PASS();
}

TEST test_parsed_option_numeric_with_multiple_bytes()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x33);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_NUMERIC);
        ASSERT_EQ(o->value.number, 0x00015180);

        llist_destroy(&options);
        PASS();
}

TEST test_parsed_option_ip_trailing_and_leading_zeros()
{
        llist_t *options = llist_new();
        dhcp_option_raw_parse(options, raw_dhcp_options);

        dhcp_option_t *o = dhcp_option_retrieve(options, 0x01);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->value.ip, 0x70000000);

        o = dhcp_option_retrieve(options, 0x03);
        ASSERT_NEQ(o, NULL);
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->value.ip, 0x00005040);
        
        llist_destroy(&options);
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
        RUN_TEST(test_parse_raw_options);
        RUN_TEST(test_retrieve_option_by_tag);
        RUN_TEST(test_add_option_to_linked_list);

        RUN_TEST(test_parsed_option_numeric);
        RUN_TEST(test_parsed_option_ip);
        RUN_TEST(test_parsed_option_boolean);
        RUN_TEST(test_parsed_option_string);
        RUN_TEST(test_parsed_option_binary);
        RUN_TEST(test_parsed_option_numeric_with_multiple_bytes);
        RUN_TEST(test_parsed_option_ip_trailing_and_leading_zeros);
}


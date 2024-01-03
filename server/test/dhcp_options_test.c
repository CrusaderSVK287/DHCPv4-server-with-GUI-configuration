#include "greatest.h"
#include "tests.h"
#include "../src/RFC/RFC-2132.h"
#include "../src/dhcp_options.h"
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

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
        // printf("%s\n", inet_ntop(AF_INET, &o->value.ip, NULL, sizeof(o->value.ip)));
        ASSERT_EQ(o->type, DHCP_OPTION_IP);
        ASSERT_EQ(o->tag, DHCP_OPTION_REQUESTED_IP_ADDRESS);
        ASSERT_EQ(o->lenght, 4);
        ASSERT_EQ(o->value.ip, 0x70605040);

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

SUITE(dhcp_options)
{
        memset(raw_dhcp_options, 0, sizeof(dhcp_options));

        // Manually setting values at specific indices
        raw_dhcp_options[0] = 0x35; // msg type
        raw_dhcp_options[1] = 0x01; // lenght 1
        raw_dhcp_options[2] = 0x03; // 3 - offer
        raw_dhcp_options[3] = 0x32; // requested ip
        raw_dhcp_options[4] = 0x04; // 4 bytes
        raw_dhcp_options[5] = 0x70; // 112
        raw_dhcp_options[6] = 0x60; // 96
        raw_dhcp_options[7] = 0x50; // 80
        raw_dhcp_options[8] = 0x40; // 64
        raw_dhcp_options[9] = 0xff;
        
        RUN_TEST(test_new_and_destroy);
        RUN_TEST(test_parse_raw_options);
        RUN_TEST(test_retrieve_option_by_tag);
        RUN_TEST(test_add_option_to_linked_list);
}


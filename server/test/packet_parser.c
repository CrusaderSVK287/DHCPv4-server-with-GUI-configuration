#include "RFC/RFC-2131.h"
#include "RFC/RFC-2132.h"
#include "tests.h"
#include "greatest.h"
#include <dhcp_packet.h>
#include <dhcp_options.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

TEST message_create_and_destroy()
{
        dhcp_message_t *m = dhcp_message_new();
        ASSERT_NEQ(NULL, m);

        dhcp_message_destroy(&m);
        ASSERT_EQ(NULL, m);

        PASS();
}

TEST packet_parse_to_message()
{
        dhcp_message_t *m = dhcp_message_new();
        ASSERT_NEQ(NULL, m);

        int fd = open("./test/packet_samples/discover.packet", O_RDONLY);
        if (fd < 0) {
                printf("Failed to open ./test/packet_samples/discover.packet %s , skipping...", strerror(errno));
                SKIP();
        }
        if (read(fd, &m->packet, 576) < 0) {
                printf("Failed to read ./test/packet_samples/discover.packet %s , skipping...", strerror(errno));
                SKIP();
        }

        /* actuall start of test */
        ASSERT_EQ(0, dhcp_packet_parse(m));

        ASSERT_EQ(1, m->opcode);
        ASSERT_EQ(1, m->htype);
        ASSERT_EQ(6, m->hlen);
        ASSERT_EQ(0, m->hops);

        ASSERT_EQ(0x7db596bf, m->xid);
        ASSERT_EQ(0, m->secs);
        ASSERT_EQ(0, m->flags);

        ASSERT_EQ(0, m->ciaddr);
        ASSERT_EQ(0, m->yiaddr);
        ASSERT_EQ(0, m->siaddr);
        ASSERT_EQ(0, m->giaddr);

        uint8_t reference_mac[] = {0xb8, 0x27, 0xeb, 0xb8, 0x84, 0xc7};
        ASSERT_MEM_EQ(reference_mac, m->chaddr, 6);

        uint8_t blank_memory[1000];
        memset(blank_memory, 0, 1000);
        ASSERT_MEM_EQ(blank_memory, m->sname, 64);
        ASSERT_MEM_EQ(blank_memory, m->filename, 128);

        ASSERT_EQ(MAGIC_COOKIE, m->cookie);

        dhcp_option_t *o = dhcp_option_retrieve(m->dhcp_options, DHCP_OPTION_DHCP_MESSAGE_TYPE);
        ASSERT_NEQ(NULL, o);
        ASSERT_EQ(1, o->value.number);
        
        o = dhcp_option_retrieve(m->dhcp_options, DHCP_OPTION_HOST_NAME);
        ASSERT_NEQ(NULL, o);
        ASSERT_EQ(11, o->lenght);
        ASSERT_STR_EQ("raspberrypi", o->value.string);
        
        o = dhcp_option_retrieve(m->dhcp_options, DHCP_OPTION_CLIENT_IDENTIFIER);
        ASSERT_NEQ(NULL, o);
        uint8_t client_id_reference[] = {0x01, 0xb8, 0x27, 0xeb, 0xb8, 0x84, 0xc7};
        ASSERT_MEM_EQ(client_id_reference, o->value.binary_data, o->lenght);

        o = dhcp_option_retrieve(m->dhcp_options, DHCP_OPTION_PARAMETER_REQUEST_LIST);
        ASSERT_NEQ(NULL, o);
        uint8_t requested_parameters_reference[] = {1, 121, 33, 3, 6, 12, 15, 26, 28, 51, 54, 58, 59, 119};
        ASSERT_MEM_EQ(requested_parameters_reference, o->value.binary_data, o->lenght);
        
        close(fd);
        dhcp_message_destroy(&m);
        PASS();
}

SUITE(packet_parser_builder)
{
        RUN_TEST(message_create_and_destroy);
        RUN_TEST(packet_parse_to_message);
}


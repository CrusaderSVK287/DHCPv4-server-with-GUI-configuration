#include "dhcp_options.h"
#include "cclog_macros.h"
#include "logging.h"
#include "RFC/RFC-2132.h"
#include "utils/llist.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// TODO: Finish the switch, only example / common are here now
static enum dhcp_option_type option_tag_to_type(int tag) {
        switch(tag) {
        case DHCP_OPTION_IP_FORWARDING_ENABLE_DISABLE:
        case DHCP_OPTION_NON_LOCAL_SOURCE_ROUTING_ENABLE_DISABLE:
        case DHCP_OPTION_ROUTER_DISCOVERY:
                return DHCP_OPTION_BOOL;

        case DHCP_OPTION_SUBNET_MASK:
        case DHCP_OPTION_ROUTER:
        case DHCP_OPTION_REQUESTED_IP_ADDRESS:
                return DHCP_OPTION_IP;

        case DHCP_OPTION_DHCP_MESSAGE_TYPE:
        case DHCP_OPTION_TIME_OFFSET:
        case DHCP_OPTION_BOOT_FILE_SIZE:
        case DHCP_OPTION_MERIT_DUMP:
        case DHCP_OPTION_MAX_DGRAM_REASSEMBLY_SIZE:
        case DHCP_OPTION_IP_ADDRESS_LEASE_TIME:
                return DHCP_OPTION_NUMERIC;

        case DHCP_OPTION_HOST_NAME:
        case DHCP_OPTION_DOMAIN_NAME:
        case DHCP_OPTION_SWAP_SERVER:
                return DHCP_OPTION_STRING;

        default:
                return DHCP_OPTION_BIN;
        }
}


static void parse_option_value_numeric(dhcp_option_t *o, uint8_t v[DHCP_OPTION_MAX_LENGHT])
{
        o->value.number = 0;
        for (size_t i = 0; i < o->lenght; i++) {
                o->value.number = (o->value.number << 8) | v[i];
        }
}

static void parse_option_value_ip(dhcp_option_t *o, uint8_t v[DHCP_OPTION_MAX_LENGHT])
{
        o->value.ip = 0;
        for (size_t i = 0; i < o->lenght; i++) {
                o->value.ip = (o->value.ip << 8) | v[i];
        }
}

static void parse_option_value_bool(dhcp_option_t *o, uint8_t v[DHCP_OPTION_MAX_LENGHT])
{
        o->value.boolean = !!v[0];
}

static void parse_option_value_string(dhcp_option_t *o, uint8_t v[DHCP_OPTION_MAX_LENGHT])
{
        memset(o->value.string, 0, DHCP_OPTION_MAX_LENGHT);
        memcpy(o->value.string, v, o->lenght);
}

static void parse_option_value_binary(dhcp_option_t *o, uint8_t v[DHCP_OPTION_MAX_LENGHT])
{
        memcpy(o->value.binary_data, v, DHCP_OPTION_MAX_LENGHT - 1);
}

static int parse_option_value(dhcp_option_t *o, uint8_t v[DHCP_OPTION_MAX_LENGHT])
{
        int rv = -1;
        if_null(o, exit);
        if_null(v, exit);

        switch (o->type) {
                case DHCP_OPTION_NUMERIC: parse_option_value_numeric(o, v); break;
                case DHCP_OPTION_IP:      parse_option_value_ip(o, v);      break;
                case DHCP_OPTION_BOOL:    parse_option_value_bool(o, v);    break;
                case DHCP_OPTION_STRING:  parse_option_value_string(o, v);  break;
                case DHCP_OPTION_BIN:     parse_option_value_binary(o, v);  break;
                default:
                        cclog(LOG_ERROR, NULL, "Inalid DHCP option type %d", o->type);
                        goto exit;
        }
        rv = 0;
exit:
        return rv;
}

int dhcp_option_raw_parse(llist_t *dest, uint8_t raw_options[])
{
        int rv = -1;
        if_null_log(dest, exit, LOG_ERROR, NULL, "destination is NULL");
        if_null_log(raw_options, exit, LOG_ERROR, NULL, "raw_options is NULL");
        
        dhcp_option_t *option = NULL;
        uint8_t value_buf[DHCP_OPTION_MAX_LENGHT];

        for (size_t i = 0; i < DHCP_PACKET_OPTIONS_SIZE;) {
                if (raw_options[i] == DHCP_OPTION_PAD)
                        continue;
                if (raw_options[i] == DHCP_OPTION_END)
                        break;

                option = dhcp_option_new();
                option->tag = raw_options[i++];
                option->type = option_tag_to_type(option->tag);
                option->lenght = raw_options[i++];

                memset(value_buf, 0, DHCP_OPTION_MAX_LENGHT);
                memcpy(value_buf, raw_options+i, option->lenght);

                if_failed_log(parse_option_value(option, value_buf), 
                        exit, LOG_WARN, NULL, "Invalid dhcp option %d value", option->tag);
                if_failed_log(dhcp_option_add(dest, option),
                                exit, LOG_ERROR, NULL, "Failed to add dhcp option");

                i += option->lenght;
        }       

        rv = 0;
exit:
        return rv;
}

dhcp_option_t* dhcp_option_retrieve(llist_t *options, uint8_t tag)
{
        if_null(options, error);

        llist_foreach(options, {
                if (((dhcp_option_t*)node->data)->tag == tag)
                        return node->data;
        })

error:
        return NULL;
}

int dhcp_option_add(llist_t *dest, dhcp_option_t *option)
{
        return llist_append(dest, option, true);
}

dhcp_option_t* dhcp_option_new()
{
        dhcp_option_t *o = calloc(1, sizeof(dhcp_option_t));

        if_null_log_ng(o, LOG_ERROR, NULL, "Failed to allocate dhcp_option_t");

        return o;
}

void dhcp_option_destroy(dhcp_option_t **option)
{
        if (!(*option))
                return;

        free(*option);
        *option = NULL;
}


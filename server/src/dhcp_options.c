#include "dhcp_options.h"
#include "RFC/RFC-2131.h"
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
        case DHCP_OPTION_SERVER_IDENTIFIER:
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

static void serialize_option_numeric(dhcp_option_t *o, uint8_t raw_options[])
{
        for (size_t i = 0; i < o->lenght; i++) {
                raw_options[o->lenght - i - 1] = (o->value.number >> (8*i)) & 0xff;
        }
}

static void serialize_option_ip(dhcp_option_t *o, uint8_t raw_options[])
{
        for (size_t i = 0; i < o->lenght; i++) {
                //o->value.ip = (o->value.ip << 8) | v[i];
                raw_options[o->lenght - i - 1] = (o->value.ip >> (8*i)) & 0xff;
        }

}

static int serialize_option_value(dhcp_option_t *o, uint8_t raw_options[])
{
        int rv = -1;
        if_null(o, exit);
        if_null(raw_options, exit);

        switch (o->type) {
                case DHCP_OPTION_NUMERIC: serialize_option_numeric(o, raw_options); break;
                case DHCP_OPTION_IP:      serialize_option_ip(o, raw_options);      break;
                case DHCP_OPTION_BIN:
                case DHCP_OPTION_BOOL:
                case DHCP_OPTION_STRING:
                        memcpy(raw_options, o->value.binary_data, o->lenght);
                        break;
                default:
                        cclog(LOG_ERROR, NULL, "Inalid DHCP option type %d", o->type);
                        goto exit;
        }
        rv = 0;
exit:
        return rv;

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
                case DHCP_OPTION_STRING:  
                case DHCP_OPTION_BIN:
                        memset(o->value.binary_data, 0, DHCP_OPTION_MAX_LENGHT);
                        memcpy(o->value.binary_data, v, o->lenght);
                        break;
                default:
                        cclog(LOG_ERROR, NULL, "Inalid DHCP option type %d", o->type);
                        goto exit;
        }
        rv = 0;
exit:
        return rv;
}

int dhcp_option_parse(llist_t *dest, uint8_t raw_options[])
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

int dhcp_options_serialize(llist_t *options, uint8_t raw_options[])
{
        int rv = -1;
        if_null(options, exit);
        if_null(raw_options, exit);

        dhcp_option_t *o = NULL;
        size_t bytes = 0;

        llist_foreach(options, {
                o = (dhcp_option_t*)node->data;
                if (!o)
                        continue;

                if ((bytes + 2 + o->lenght) > DHCP_PACKET_OPTIONS_SIZE) {
                        cclog(LOG_INFO, NULL, "Cannot serialize option %d and onwards: maximum "
                                        "dhcp options size would be exceeded", o->tag);
                        raw_options[bytes] = 0xff;
                        goto exit;
                }

                raw_options[bytes++] = o->tag;
                raw_options[bytes++] = o->lenght;
                if_failed_log(serialize_option_value(o, raw_options+bytes), exit, LOG_WARN, NULL, 
                                "Invalid DHCP option %d value, cannot serialize", o->tag);

                bytes += o->lenght;
        })

        /* Terminate DHCP options */
        raw_options[bytes] = 0xff;

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
        if (!dest || !option)
                return -1;

        /* Dont add duplicite options */
        if (dhcp_option_retrieve(dest, option->tag)) {
                return 0;
        }

        return llist_append(dest, option, false);
}

dhcp_option_t* dhcp_option_new()
{
        dhcp_option_t *o = calloc(1, sizeof(dhcp_option_t));

        if_null_log_ng(o, LOG_ERROR, NULL, "Failed to allocate dhcp_option_t");

        return o;
}

dhcp_option_t* dhcp_option_new_values(int tag, int lenght, void* value)
{
        dhcp_option_t *o = dhcp_option_new();
        if_null(o, error);

        o->tag = tag;
        o->lenght = lenght;
        memcpy(o->value.binary_data, value, lenght);
        o->type = option_tag_to_type(tag);

        return o;
error:
        return NULL;
}


void dhcp_option_destroy(dhcp_option_t **option)
{
        if (!option || !(*option))
                return;

        free(*option);
        *option = NULL;
}

void dhcp_option_destroy_list(llist_t **ll)
{
        if (!ll)
                return;

        dhcp_option_t *o;

        llist_foreach(*ll, {
                o = (dhcp_option_t*)node->data;
                dhcp_option_destroy(&o);
        })

        llist_destroy(ll);
}

void dhcp_options_dump(llist_t *l)
{
        if_null(l, exit);

        dhcp_option_t *o = NULL;

        llist_foreach(l, {
                o = (dhcp_option_t*)node->data;

                printf("Tag:      %d\n"
                       "Lenght:   %d\n"
                       "Value:    ", o->tag, o->lenght);
                for(int i = 0; i < o->lenght; i++)
                        printf("%02x ", o->value.binary_data[i]);

                printf("\n");
        })
        printf("END OF OPTION DUMP\n");

exit:
        return;
}

static bool is_option_blacklisted(uint8_t tag, uint8_t *blacklist)
{
        if (!blacklist)
                return true;

        while (*blacklist != 0) {
                if (*blacklist == tag)
                        return true;

                blacklist++;
        }

        return false;
}

int dhcp_option_build_required_options(llist_t *dest, uint8_t *requested_options, 
        uint8_t *required_options, uint8_t *blacklisted_options, 
        llist_t *global_options, llist_t *pool_options, enum dhcp_message_type type) 
{
        if (!dest || !requested_options || !required_options || 
                !blacklisted_options || !global_options || !pool_options)
                return -1;

        int rv = -1;

        /* Add message type, this is required always so there is no need to skip it */
        uint8_t msg_type = type;
        dhcp_option_t *o53 = dhcp_option_new_values(DHCP_OPTION_DHCP_MESSAGE_TYPE, 1, &msg_type);
        if_failed_log(dhcp_option_add(dest, o53), exit, LOG_ERROR, NULL,
                        "Failed to add dhcp option 53 during %s building", 
                        rfc2131_dhcp_message_type_to_str(type));

        /* Add mandatory dhcp options */
        dhcp_option_t *option = NULL;
        uint8_t tag = *required_options;
        int i = 0;

        while (tag != 0) {
                if (!is_option_blacklisted(tag, blacklisted_options)) {
                        option = dhcp_option_retrieve(pool_options, tag);
                        if (!option)
                                option = dhcp_option_retrieve(global_options, tag);
                        if_null_log(option, exit, LOG_WARN, NULL, 
                                "Cannot set mandatory option %d for %s because it's not configured", tag,
                                rfc2131_dhcp_message_type_to_str(type));

                        if_failed_log(dhcp_option_add(dest, option), exit, LOG_WARN, NULL, 
                                "Failed to  add mandatory option %d for %s", tag, 
                                rfc2131_dhcp_message_type_to_str(type));
                }

                /* we need to increment the pointer to get the next tag */
                i++;
                tag = *(required_options + i);
        }

        /* Add requested dhcp options */
        option = NULL;
        tag = *requested_options;
        i = 0;

        while (tag != 0) {
                if (!is_option_blacklisted(tag, blacklisted_options)) {
                        option = dhcp_option_retrieve(pool_options, tag);
                        if (!option)
                                option = dhcp_option_retrieve(global_options, tag);
                        if_null_log_ng(option, LOG_INFO, NULL, 
                                "Cannot set option %d for %s because it's not configured", tag,
                                rfc2131_dhcp_message_type_to_str(type));
                        dhcp_option_add(dest, option);
                }
                
                /* we need to increment the pointer to get the next tag */
                i++;
                tag = *(requested_options + i);
        }

        rv = 0;
exit:
        return rv;
}


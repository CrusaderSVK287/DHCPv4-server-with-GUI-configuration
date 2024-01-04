#include "dhcp_options.h"
#include "cclog_macros.h"
#include "logging.h"
#include "RFC/RFC-2132.h"
#include "utils/llist.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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



int dhcp_option_raw_parse(llist_t *dest, uint8_t raw_options[])
{
        int rv = -1;
        if_null_log(dest, exit, LOG_ERROR, NULL, "destination is NULL");
        if_null_log(raw_options, exit, LOG_ERROR, NULL, "raw_options is NULL");
        
        dhcp_option_t *option = NULL;

        for (size_t i = 0; i < DHCP_OPTION_MAX_LENGHT;) {
                if (raw_options[i] == DHCP_OPTION_PAD)
                        continue;
                if (raw_options[i] == DHCP_OPTION_END)
                        break;

                option = dhcp_option_new();
                option->tag = raw_options[i++];
                option->type = option_tag_to_type(option->tag);
                option->lenght = raw_options[i++];

                for (size_t j = 0; j < option->lenght; j++) {
                        option->value.binary_data[j] = raw_options[i++];
                }

                if_failed_log(dhcp_option_add(dest, option),
                                exit, LOG_ERROR, NULL, "Failed to add dhcp option");
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


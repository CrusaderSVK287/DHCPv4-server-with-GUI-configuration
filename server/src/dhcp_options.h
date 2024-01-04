#ifndef __DHCP_OPTIONS_H__
#define __DHCP_OPTIONS_H__

#include "utils/llist.h"
#include "RFC/RFC-2132.h"

#include <stdint.h>
#include <stdio.h>

enum dhcp_option_type {
    DHCP_OPTION_NUMERIC,
    DHCP_OPTION_STRING,
    DHCP_OPTION_IP,
    DHCP_OPTION_BOOL,
    DHCP_OPTION_BIN
};

typedef struct dhcp_option {
    enum dhcp_option_type type;

    uint8_t tag;
    uint8_t lenght;
    union {
        uint32_t number;
        char string[DHCP_OPTION_MAX_LENGHT];
        uint32_t ip;  
        uint8_t boolean;
        uint8_t binary_data[DHCP_OPTION_MAX_LENGHT];
    } value;
} dhcp_option_t;

/**
 * Allocate memory for dhcp_option_t struct */
dhcp_option_t* dhcp_option_new();

/**
 * Frees memory allocated to dhcp_option_t struct. Dont call for options inside 
 * a linked list! Do llist_append(list, option, true) instead or use
 * dhcp_option_add()
 */
void dhcp_option_destroy(dhcp_option_t **option);

/**
 * Parse raw_options[] into multiple dhcp_option_t and put them into the 
 * dest linked list 
 */
int dhcp_option_raw_parse(llist_t *dest, uint8_t raw_options[]);

/**
 * Retrieve a dhcp_option_t from linked list based on tag 
 */
dhcp_option_t* dhcp_option_retrieve(llist_t *options, uint8_t tag);

/**
 * Add dhcp_option_t to dest linked list 
 */
int dhcp_option_add(llist_t *dest, dhcp_option_t *option);

#endif // !__DHCP_OPTIONS_H__

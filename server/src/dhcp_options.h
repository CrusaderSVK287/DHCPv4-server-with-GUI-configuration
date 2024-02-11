#ifndef __DHCP_OPTIONS_H__
#define __DHCP_OPTIONS_H__

#include "RFC/RFC-2131.h"
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

/* Same as dhcp_option_new, but you can specify parameters */
dhcp_option_t* dhcp_option_new_values(int tag, int lenght, void* value);

/**
 * Frees memory allocated to dhcp_option_t struct. Dont call for options inside 
 * a linked list! Do llist_append(list, option, true) instead or use
 * dhcp_option_add()
 */
void dhcp_option_destroy(dhcp_option_t **option);

/**
 * Destroys list of DHCP options 
 */
void dhcp_option_destroy_list(llist_t **ll);

/**
 * Parse raw_options[] into multiple dhcp_option_t and put them into the 
 * dest linked list 
 */
int dhcp_option_parse(llist_t *dest, uint8_t raw_options[]);

/**
 * Serializes parsed dhcp options from linked list to raw_options,
 * which can be sent as a part of DHCP message
 */
int dhcp_options_serialize(llist_t *options, uint8_t raw_options[]);

/**
 * Retrieve a dhcp_option_t from linked list based on tag 
 */
dhcp_option_t* dhcp_option_retrieve(llist_t *options, uint8_t tag);

/**
 * Add dhcp_option_t to dest linked list 
 */
int dhcp_option_add(llist_t *dest, dhcp_option_t *option);

void dhcp_options_dump(llist_t *o);

/*
 * This function is meant to be a unified API for dhcp message handlers 
 * to build requested dhcp options from clients. 
 * Arguments:
 * llist_t *dest - destination linked list with parsed options 
 * uint8_t *requested_options - dhcp options requested by client 
 * uint8_t *required_options  - dhcp options that must be present regardless if 
 *                              client requested them or not
 * uint8_t *blacklisted_options - dhcp options that MUST NOT be present 
 *                              regardless whether client requested them or not
 * llist_t *global_options - globally configured dhcp options (from allocator)
 * llist_t *pool_options   - pool specific dhcp options. If a requested/required 
 *                           option is present in this list, it will be used,
 *                           if not, option from global_options will be used.
 * Note: requested_options, requested_options and blacklisted_options are arrays 
 *       and MUST BE 0 TERMINATED!!! Example: {53, 12, 1, 0}
 *
 * Returns 0 on success, -1 on failure. Log is made stating error occured.
 */
int dhcp_option_build_required_options(llist_t *dest, uint8_t *requested_options, 
        uint8_t *required_options, uint8_t *blacklisted_options, 
        llist_t *global_options, llist_t *pool_options,
        enum dhcp_message_type type);

#endif // !__DHCP_OPTIONS_H__

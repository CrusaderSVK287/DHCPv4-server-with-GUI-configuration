#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__

#include "RFC/RFC-2131.h"
#include "utils/llist.h"
#include "dhcp_packet.h"
#include "timer.h"
#include <stdint.h>

// TODO: implement config 
#define TRANSACTION_TIMEOUT_DEFAULT 60

typedef struct transaction {
    uint32_t transaction_begin;
    uint32_t xid;
    uint8_t num_of_messages;
    llist_t *messages_ll;
    struct timer *timer;
} transaction_t;

/* Allocate space for a new transaction */
transaction_t *trans_new();

/* Free transaction from memory */
void trans_destroy(transaction_t **transaction);

/* Clears data from transaction, does NOT free from memory */
void trans_clear(transaction_t *transaction);

/* Add message to transaction, */
int trans_add(transaction_t *transaction, const dhcp_message_t *message);

/* Retrieve a message with specific index. 
 * Example, if the transaction has DORA messages.
 * Calling this function with index parameter to 2 will return dhcp_message_t
 * corresponding to the DHCPREQUEST message 
 */
dhcp_message_t *trans_get_index(transaction_t *transaction, uint8_t index);

/* Returns first DHCP message that matches the type */
dhcp_message_t *trans_search_for(transaction_t *transaction, enum dhcp_message_type type);

/* Returns last DHCP message that matches the type */
dhcp_message_t *trans_search_for_last(transaction_t *transaction, enum dhcp_message_type type);

int trans_update_timer(transaction_t *transaction);

#endif // !__TRANSACTION_H__


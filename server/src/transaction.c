#include "transaction.h"
#include "cclog_macros.h"
#include "logging.h"
#include "utils/llist.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

transaction_t *trans_new()
{
        transaction_t *t = calloc(1, sizeof(transaction_t));
        if_null(t, error);
        
        t->messages_ll = llist_new();
        if_null(t, error);
        trans_clear(t);

        return t;
error:
        return NULL;
}

void trans_destroy(transaction_t **transaction)
{
        if (!transaction || ! (*transaction))
                return;

        llist_destroy(&((*transaction)->messages_ll));

        free(*transaction);
        *transaction = NULL;
}

void trans_clear(transaction_t *transaction)
{
        if (!transaction || !transaction->messages_ll)
                return;

        transaction->num_of_messages = 0;
        transaction->transaction_begin = 0;
        transaction->xid = 0;
        llist_clear(&(*transaction->messages_ll));
}

int trans_add(transaction_t *transaction, const dhcp_message_t *message)
{
        if (!transaction || !message)
                return -1;

        int rv = -1;

        if (transaction->xid) {
                if_false_log((transaction->xid == message->xid), exit, LOG_WARN, NULL,
                        "Cannot add to transaction, xid mismatch");
        }

        dhcp_message_t *message_copy = malloc(sizeof(dhcp_message_t));
        if_null_log(message_copy, exit, LOG_ERROR, NULL, "Cannot allocated dhcp_message_t");

        memcpy(message_copy, message, sizeof(dhcp_message_t));

        if (transaction->num_of_messages == 0) {
                transaction->xid = message->xid;
                transaction->transaction_begin = time(NULL);
        }

        if_failed_log(llist_append(transaction->messages_ll, message_copy, true), exit, LOG_ERROR,
                        NULL, "Failed to append message to transaction");
        transaction->num_of_messages += 1;

        rv = 0;
exit:
        return rv;
}

dhcp_message_t *trans_get_index(transaction_t *transaction, uint8_t index)
{
        if (!transaction)
                goto error;

        llnode_t *node = llist_get_index(transaction->messages_ll, index);
        if_null(node, error);

        return (dhcp_message_t*)node->data;
error:
        return NULL;
}

dhcp_message_t *trans_search_for(transaction_t *transaction, enum dhcp_message_type type)
{
        if (!transaction)
                goto error;

        dhcp_message_t *message = NULL;
        llist_foreach(transaction->messages_ll, {
                message = (dhcp_message_t*)node->data;

                if (message->type == type)
                        return message;
        })

error:
        return NULL;
}

dhcp_message_t *trans_search_for_last(transaction_t *transaction, enum dhcp_message_type type)
{
        if (!transaction)
                goto error;

        /* Because of implementation of llist, we cannot itterate from the back. 
         * So we need to track the last message that is equal to the type we 
         * desire and always update it when newer message is encountered
         */
        dhcp_message_t *message = NULL;
        dhcp_message_t *last = NULL;

        llist_foreach(transaction->messages_ll, {
                message = (dhcp_message_t*)node->data;

                if (message->type == type)
                        last = message;
        })

        return last;
error:
        return NULL;
}


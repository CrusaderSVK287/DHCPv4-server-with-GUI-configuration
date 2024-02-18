#ifndef __TRANSACTION_CACHE_H__
#define __TRANSACTION_CACHE_H__

#include "RFC/RFC-2131.h"
#include "dhcp_packet.h"
#include "transaction.h"
#include <stdint.h>

#define TRANSACTION_CACHE_DEFAULT_SIZE 15

/*
 * Wrapper structure to hold transaction data.
 * The transaction cache is an array of transactions, preallocated 
 * during initialisation of server. It holds all currently active and 
 * past transaction up to <size> of transactions. 
 * _offset is used to tell which transaction is the newest, so that any new 
 * transactions will be stored after the newest one in case it is needed.
 * When _offset reaches size, it will overflow back to 0
 */
// TODO: implement a timeout for transactions so the transaction entry is not overwritten until the time is reached
typedef struct transaction_cache {
    transaction_t **transactions;
    uint32_t size;
    uint32_t _offset;
} transaction_cache_t;

/* 
 * Initialise a transaction_cache. The cache pointer MUST be NULL 
 * Returns 0 on success and -1 on failure
 */
transaction_cache_t *trans_cache_new(int size);

int trans_cache_add_message(transaction_cache_t *cache, dhcp_message_t *message);

/* Retrieve transaction with specified xid */
transaction_t *trans_cache_retrieve_transaction(transaction_cache_t *cache, uint32_t xid);

/* Retrieve first message of type from transaction in cache */
dhcp_message_t *trans_cache_retrieve_message(transaction_cache_t *cache, 
        uint32_t xid, enum dhcp_message_type type);

/* Retrieve last message of type from transaction in cache */
dhcp_message_t *trans_cache_retrieve_message_last(transaction_cache_t *cache, 
        uint32_t xid, enum dhcp_message_type type);

/* Retrieve message on index from transaction in cache */
dhcp_message_t *trans_cache_retrieve_message_index(transaction_cache_t *cache, uint32_t xid,
        uint32_t index);

/* Remove all transactions from cache */
int trans_cache_purge(transaction_cache_t *cache);

void trans_cache_destroy(transaction_cache_t **cache);

#endif // !__TRANSACTION_CACHE_H__


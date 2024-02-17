#include "transaction_cache.h"
#include "cclog_macros.h"
#include "logging.h"
#include "transaction.h"
#include <stdint.h>

transaction_cache_t *trans_cache_new()
{

        transaction_cache_t *cache = calloc(1, sizeof(transaction_cache_t));
        if_null_log(cache, error, LOG_ERROR, NULL, "Failed to allocate transaction cache");

        // TODO: Configuration transaction cache size 
        cache->size = TRANSACTION_CACHE_DEFAULT_SIZE;
        cache->transactions = malloc(sizeof(transaction_t*) * cache->size);
        if_null_log(cache->transactions, error, LOG_ERROR, NULL, "Failed to allocate transaction cache");

        for (uint32_t i = 0; i < cache->size; i++) {
                cache->transactions[i] = trans_new();
                if_null_log(cache->transactions[i], error, LOG_ERROR, NULL, 
                        "Failed to allocate transaction %u/%u in cache", i, cache->size);
        }

        cclog(LOG_MSG, NULL, "Initialised transaction cache of size %u", cache->size);

        return cache;
error:
        return NULL;
}

static transaction_t* cache_get_next_transaction(transaction_cache_t *cache)
{
        if (!cache)
                goto error;

        cache->_offset += 1;
        if (cache->_offset >= cache->size)
                cache->_offset = 0;

        return cache->transactions[cache->_offset];
error:
        return NULL;
}

int trans_cache_add_message(transaction_cache_t *cache, dhcp_message_t *message)
{
        if (!cache || !message)
                return -1;

        int rv = -1;

        transaction_t *transaction = trans_cache_retrieve_transaction(cache, message->xid);
        if (!transaction) {
                // No transaction, get next transaction
                transaction = cache_get_next_transaction(cache);
                if_null(transaction, exit);
                trans_clear(transaction);
        } 
        
        rv = trans_add(transaction, message);
exit:
        return rv;
}

/* Retrieve transaction with specified xid */
transaction_t *trans_cache_retrieve_transaction(transaction_cache_t *cache, uint32_t xid)
{
        if (!cache)
                return NULL;

        for (uint32_t i = 0; i < cache->size; i++) {
                if (cache->transactions[i]->xid == xid)
                        return cache->transactions[i];
        }

        return NULL;
}

/* Retrieve first message of type from transaction in cache */
dhcp_message_t *trans_cache_retrieve_message(transaction_cache_t *cache, 
        uint32_t xid, enum dhcp_message_type type)
{
        return trans_search_for(trans_cache_retrieve_transaction(cache, xid), type);
}

/* Retrieve last message of type from transaction in cache */
dhcp_message_t *trans_cache_retrieve_message_last(transaction_cache_t *cache, 
        uint32_t xid, enum dhcp_message_type type)
{
        return trans_search_for_last(trans_cache_retrieve_transaction(cache, xid), type);
}

/* Retrieve message on index from transaction in cache */
dhcp_message_t *trans_cache_retrieve_message_index(transaction_cache_t *cache, uint32_t xid,
                uint32_t index)
{
        return trans_get_index(trans_cache_retrieve_transaction(cache, xid), index);
}

/* Clear all transactions from cache */
int trans_cache_purge(transaction_cache_t *cache)
{
        if (!cache)
                return -1;

        for (uint32_t i = 0; i < cache->size; i++) {
                trans_clear(cache->transactions[i]);
        }

        return 0;
}

void trans_cache_destroy(transaction_cache_t **cache)
{
        if (!cache || ! *cache)
                return;

        trans_cache_purge(*cache);
        free(*cache);
        *cache = NULL;
}


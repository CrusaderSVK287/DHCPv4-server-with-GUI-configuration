#include "RFC/RFC-2131.h"
#include "address_pool.h"
#include "allocator.h"
#include "dhcp_packet.h"
#include "dhcp_server.h"
#include "greatest.h"
#include "tests.h"
#include "timer_args.h"
#include "transaction.h"
#include "utils/xtoy.h"
#include <stdint.h>
#include <transaction_cache.h>
#include <stdio.h>
#include <unistd.h>

static transaction_t *setup_transaction() 
{
        transaction_t *t = trans_new();
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg3 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg4 = calloc(1, sizeof(dhcp_message_t));
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        msg2->type = DHCP_OFFER;
        msg2->xid = 0x5555;
        msg3->type = DHCP_REQUEST;
        msg3->xid = 0x5555;
        msg4->type = DHCP_ACK;
        msg4->xid = 0x5555;

        trans_add(t, msg1);
        trans_add(t, msg2);
        trans_add(t, msg3);
        trans_add(t, msg4);

        return t;        
}

TEST test_trans_new_and_destroy()
{
        transaction_t *t = trans_new();
        ASSERT_NEQ(NULL, t);
        ASSERT_NEQ(NULL, t->messages_ll);

        trans_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_transaction_add() {
        transaction_t *t = trans_new();
        ASSERT_NEQ(NULL, t);
        ASSERT_NEQ(NULL, t->messages_ll);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        msg2->type = DHCP_OFFER;
        msg2->xid = 0x5555;

        ASSERT_EQ(0, trans_add(t, msg1));
        ASSERT_EQ(0, trans_add(t, msg2));

        ASSERT_EQ(2, t->num_of_messages);
        ASSERT_EQ(0x5555, t->xid);

        trans_destroy(&t);
        PASS();
}

TEST test_transaction_add_mismatched_xid() {
        transaction_t *t = trans_new();
        ASSERT_NEQ(NULL, t);
        ASSERT_NEQ(NULL, t->messages_ll);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        msg2->type = DHCP_OFFER;
        msg2->xid = 0x6666;

        ASSERT_EQ(0, trans_add(t, msg1));
        ASSERT_EQ(-1, trans_add(t, msg2));

        ASSERT_EQ(1, t->num_of_messages);
        ASSERT_EQ(0x5555, t->xid);

        trans_destroy(&t);
        PASS();
}

TEST test_transaction_get_index()
{
        transaction_t *t = setup_transaction();
        ASSERT_NEQ(NULL, t);

        dhcp_message_t *m = trans_get_index(t, 2);
        ASSERT_EQ(DHCP_REQUEST, m->type);

        trans_destroy(&t);
        PASS();
}

TEST test_tramsaction_search_for()
{
        transaction_t *t = setup_transaction();
        ASSERT_NEQ(NULL, t);

        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        msg1->type = DHCP_ACK;
        msg1->xid = 0x5555;
        msg2->type = DHCP_ACK;
        msg2->xid = 0x5555;

        ASSERT_EQ(0, trans_add(t, msg1));
        ASSERT_EQ(0, trans_add(t, msg2));

        dhcp_message_t *check_ptr = trans_get_index(t, 3);
        dhcp_message_t *value_ptr = trans_search_for(t, DHCP_ACK);
        ASSERT_EQ(DHCP_ACK, check_ptr->type);
        ASSERT_EQ(DHCP_ACK, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        check_ptr = trans_get_index(t, 1);
        value_ptr = trans_search_for(t, DHCP_OFFER);
        ASSERT_EQ(DHCP_OFFER, check_ptr->type);
        ASSERT_EQ(DHCP_OFFER, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        trans_destroy(&t);
        PASS();
}

TEST test_tramsaction_search_for_last()
{
        transaction_t *t = setup_transaction();
        ASSERT_NEQ(NULL, t);

        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        msg1->type = DHCP_ACK;
        msg1->xid = 0x5555;
        msg2->type = DHCP_ACK;
        msg2->xid = 0x5555;

        ASSERT_EQ(0, trans_add(t, msg1));
        ASSERT_EQ(0, trans_add(t, msg2));

        dhcp_message_t *check_ptr = trans_get_index(t, 5);
        dhcp_message_t *value_ptr = trans_search_for_last(t, DHCP_ACK);
        ASSERT_EQ(DHCP_ACK, check_ptr->type);
        ASSERT_EQ(DHCP_ACK, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        trans_destroy(&t);
        PASS();
}

TEST test_cache_init_and_destroy() 
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        for (int i = 0; i < 15; i++)
                ASSERT_EQ(false, cache->transactions[i]->timer->is_running);

        trans_cache_destroy(&cache);
        ASSERT_EQ(NULL, cache);

        PASS();
}

TEST test_cache_add_message() 
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0x5555, cache->transactions[0]->xid);

        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_add_multiple_messages_to_same_transaction()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
       
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        msg1->type = DHCP_ACK;
        msg1->xid = 0x5555;
        msg2->type = DHCP_ACK;
        msg2->xid = 0x5555;
        
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg2));
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(2, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0x5555, cache->transactions[0]->xid);
        
        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_add_multiple_messages_to_different_transactions()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg3 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg4 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        ASSERT_NEQ(NULL, msg3);
        ASSERT_NEQ(NULL, msg4);
        msg1->type = DHCP_RELEASE;
        msg2->type = DHCP_REQUEST;
        msg3->type = DHCP_ACK;
        msg4->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        msg2->xid = 0x6666;
        msg3->xid = 0x6666;
        msg4->xid = 0x7777;
        
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg2));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg3));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg4));
        
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(true, cache->transactions[1]->timer->is_running);
        ASSERT_EQ(true, cache->transactions[2]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(2, cache->transactions[1]->num_of_messages);
        ASSERT_EQ(1, cache->transactions[2]->num_of_messages);
        ASSERT_EQ(0x5555, cache->transactions[0]->xid);
        ASSERT_EQ(0x6666, cache->transactions[1]->xid);
        ASSERT_EQ(0x7777, cache->transactions[2]->xid);
        
        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_overflow_cache_size()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 1000;

        for(int i = 0; i < 15; i++) {
                msg1->xid = 1000 + i;
                ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        }

        msg1->xid = 5000;
        ASSERT_EQ(-1, trans_cache_add_message(cache, msg1));
        msg1->xid = 6000;
        ASSERT_EQ(-1, trans_cache_add_message(cache, msg1));
        
        ASSERT_EQ(1, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(1, cache->transactions[1]->num_of_messages);
        ASSERT_EQ(1, cache->transactions[2]->num_of_messages);
        ASSERT_EQ(1000, cache->transactions[0]->xid);
        ASSERT_EQ(1001, cache->transactions[1]->xid);
        ASSERT_EQ(1002, cache->transactions[2]->xid);
        
        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_retrieve_transaction()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 1000;
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));

        transaction_t *t = trans_cache_retrieve_transaction(cache, 1000);
        ASSERT_NEQ(NULL, t);
        ASSERT_EQ(1, t->num_of_messages);
        ASSERT_EQ(1000, t->xid);
        
        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_retrieve_non_existent_transaction()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 1000;
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));

        transaction_t *t = trans_cache_retrieve_transaction(cache, 9999);
        ASSERT_EQ(NULL, t);
        
        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_retrieve_messages_from_transaction()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg3 = calloc(1, sizeof(dhcp_message_t));
        dhcp_message_t *msg4 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        ASSERT_NEQ(NULL, msg2);
        ASSERT_NEQ(NULL, msg3);
        ASSERT_NEQ(NULL, msg4);
        msg1->type = DHCP_DISCOVER;
        msg2->type = DHCP_OFFER;
        msg3->type = DHCP_REQUEST;
        msg4->type = DHCP_ACK;
        msg1->xid = 0x6666;
        msg2->xid = 0x6666;
        msg3->xid = 0x6666;
        msg4->xid = 0x6666;
        
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg2));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg3));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg4));
        
        dhcp_message_t *check_ptr = trans_cache_retrieve_message(cache, 0x6666, DHCP_REQUEST);
        dhcp_message_t *value_ptr = trans_cache_retrieve_message_index(cache, 0x6666, 4);
        ASSERT_EQ(DHCP_REQUEST, check_ptr->type);
        ASSERT_EQ(DHCP_REQUEST, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        check_ptr = trans_cache_retrieve_message_last(cache, 0x6666, DHCP_DISCOVER);
        value_ptr = trans_cache_retrieve_message_index(cache, 0x6666, 2);
        ASSERT_EQ(DHCP_DISCOVER, check_ptr->type);
        ASSERT_EQ(DHCP_DISCOVER, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_purge()
{
        transaction_cache_t *cache = trans_cache_new(15);
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 1000;

        for(int i = 0; i < 15; i++) {
                msg1->xid = 1000 + i;
                ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        }
        
        trans_cache_purge(cache);
        ASSERT_NEQ(NULL, cache);

        for(int i = 0; i < cache->size; i++) {
                ASSERT_EQ(0, cache->transactions[i]->xid);
                ASSERT_EQ(0, cache->transactions[i]->num_of_messages);
                ASSERT_EQ(0, cache->transactions[i]->transaction_begin);
                ASSERT_EQ(false, cache->transactions[i]->timer->is_running);
        }

        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_wait_until_transaction_is_finished()
{
#ifndef __RUN_TIMER_TESTS__
        SKIP();
#endif
        
        dhcp_server_t server = {0};
        trans_update_args_t args = {0};
        args.server = &server;
        
        server.trans_cache = trans_cache_new(15);
        transaction_cache_t *cache = server.trans_cache;
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0x5555, cache->transactions[0]->xid);

        for (int i = 0; i < TRANSACTION_TIMEOUT_DEFAULT + 5; i++) {
                args.index = 0;
                trans_update_timer(&args);
                sleep(1);

                /* Add another message */
                if (i == 30) {
                        msg1->xid = 0x6666;
                        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
                        ASSERT_EQ(true, cache->transactions[1]->timer->is_running);
                        ASSERT_EQ(1, cache->transactions[1]->num_of_messages);
                        ASSERT_EQ(0x6666, cache->transactions[1]->xid);
                }
                if (i >= 30) {
                        args.index = 1;
                        trans_update_timer(&args);
                }
        }

        ASSERT_EQ(false, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(0, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0, cache->transactions[0]->xid);
        
        ASSERT_EQ(true, cache->transactions[1]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[1]->num_of_messages);
        ASSERT_EQ(0x6666, cache->transactions[1]->xid);

        /* Add third message after first one is expired and second one is not */
        msg1->xid = 0x7777;
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0x7777, cache->transactions[0]->xid);

        trans_cache_destroy(&cache);
        PASS();
}

TEST test_cache_wait_until_transaction_is_finished_return_address_to_pool()
{
#ifndef __RUN_TIMER_TESTS__
        SKIP();
#endif
        dhcp_server_t server = {0};
        server.allocator = address_allocator_new();
        ASSERT_NEQ(NULL, server.allocator);
        ASSERT_EQ(ALLOCATOR_OK, allocator_add_pool(server.allocator, address_pool_new_str("pool", "192.168.1.1", "192.168.1.253", "255.255.255.0")));
        uint32_t addr_buff;
        ASSERT_EQ(ALLOCATOR_OK, allocator_request_this_address_str(server.allocator, "192.168.1.10", &addr_buff));
        ASSERT_EQ(false, allocator_is_address_available_str(server.allocator, "192.168.1.10"));
        trans_update_args_t args = {0};
        args.server = &server;

        server.trans_cache = trans_cache_new(15);
        transaction_cache_t *cache = server.trans_cache;
        ASSERT_NEQ(NULL, cache);
        
        dhcp_message_t *msg1 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg1->type = DHCP_DISCOVER;
        msg1->xid = 0x5555;
        dhcp_message_t *msg2 = calloc(1, sizeof(dhcp_message_t));
        ASSERT_NEQ(NULL, msg1);
        msg2->type = DHCP_OFFER;
        msg2->xid = 0x5555;
        msg2->yiaddr = ipv4_address_to_uint32("192.168.1.10");
        
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(0, trans_cache_add_message(cache, msg2));
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(2, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0x5555, cache->transactions[0]->xid);

        for (int i = 0; i < TRANSACTION_TIMEOUT_DEFAULT + 5; i++) {
                args.index = 0;
                trans_update_timer(&args);
                sleep(1);

                /* Add another message */
                if (i == 30) {
                        msg1->xid = 0x6666;
                        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
                        ASSERT_EQ(true, cache->transactions[1]->timer->is_running);
                        ASSERT_EQ(1, cache->transactions[1]->num_of_messages);
                        ASSERT_EQ(0x6666, cache->transactions[1]->xid);
                }
                if (i >= 30) {
                        args.index = 1;
                        trans_update_timer(&args);
                }
        }

        ASSERT_EQ(false, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(0, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0, cache->transactions[0]->xid);
        /* Address should be available again */
        ASSERT_EQ(true, allocator_is_address_available_str(server.allocator, "192.168.1.10"));
        
        ASSERT_EQ(true, cache->transactions[1]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[1]->num_of_messages);
        ASSERT_EQ(0x6666, cache->transactions[1]->xid);

        /* Add third message after first one is expired and second one is not */
        msg1->xid = 0x7777;
        ASSERT_EQ(0, trans_cache_add_message(cache, msg1));
        ASSERT_EQ(true, cache->transactions[0]->timer->is_running);
        ASSERT_EQ(1, cache->transactions[0]->num_of_messages);
        ASSERT_EQ(0x7777, cache->transactions[0]->xid);

        trans_cache_destroy(&cache);
        PASS();
}

SUITE(transaction) 
{
        RUN_TEST(test_trans_new_and_destroy);
        RUN_TEST(test_transaction_add);
        RUN_TEST(test_transaction_add_mismatched_xid);
        RUN_TEST(test_transaction_get_index);
        RUN_TEST(test_tramsaction_search_for);
        RUN_TEST(test_tramsaction_search_for_last);

        RUN_TEST(test_cache_init_and_destroy);
        RUN_TEST(test_cache_add_message);
        RUN_TEST(test_cache_add_multiple_messages_to_same_transaction);
        RUN_TEST(test_cache_add_multiple_messages_to_different_transactions);
        RUN_TEST(test_cache_overflow_cache_size);
        RUN_TEST(test_cache_retrieve_transaction);
        RUN_TEST(test_cache_retrieve_non_existent_transaction);
        RUN_TEST(test_cache_retrieve_messages_from_transaction);
        RUN_TEST(test_cache_purge);
        RUN_TEST(test_cache_wait_until_transaction_is_finished);
        RUN_TEST(test_cache_wait_until_transaction_is_finished_return_address_to_pool);
}


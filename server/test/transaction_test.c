#include "RFC/RFC-2131.h"
#include "dhcp_packet.h"
#include "greatest.h"
#include "tests.h"
#include "transaction.h"
#include <stdio.h>

static transaction_t *setup_transaction() 
{
        transaction_t *t = transaction_new();
        
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

        transaction_add(t, msg1);
        transaction_add(t, msg2);
        transaction_add(t, msg3);
        transaction_add(t, msg4);

        return t;        
}

TEST test_transaction_new_and_destroy()
{
        transaction_t *t = transaction_new();
        ASSERT_NEQ(NULL, t);
        ASSERT_NEQ(NULL, t->messages_ll);

        transaction_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_transaction_add() {
        transaction_t *t = transaction_new();
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

        ASSERT_EQ(0, transaction_add(t, msg1));
        ASSERT_EQ(0, transaction_add(t, msg2));

        ASSERT_EQ(2, t->num_of_messages);
        ASSERT_EQ(0x5555, t->xid);

        transaction_destroy(&t);
        PASS();
}

TEST test_transaction_add_mismatched_xid() {
        transaction_t *t = transaction_new();
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

        ASSERT_EQ(0, transaction_add(t, msg1));
        ASSERT_EQ(-1, transaction_add(t, msg2));

        ASSERT_EQ(1, t->num_of_messages);
        ASSERT_EQ(0x5555, t->xid);

        transaction_destroy(&t);
        PASS();
}

TEST test_transaction_get_index()
{
        transaction_t *t = setup_transaction();
        ASSERT_NEQ(NULL, t);

        dhcp_message_t *m = transaction_get_index(t, 2);
        ASSERT_EQ(DHCP_REQUEST, m->type);

        transaction_destroy(&t);
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

        ASSERT_EQ(0, transaction_add(t, msg1));
        ASSERT_EQ(0, transaction_add(t, msg2));

        dhcp_message_t *check_ptr = transaction_get_index(t, 3);
        dhcp_message_t *value_ptr = transaction_search_for(t, DHCP_ACK);
        ASSERT_EQ(DHCP_ACK, check_ptr->type);
        ASSERT_EQ(DHCP_ACK, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        check_ptr = transaction_get_index(t, 1);
        value_ptr = transaction_search_for(t, DHCP_OFFER);
        ASSERT_EQ(DHCP_OFFER, check_ptr->type);
        ASSERT_EQ(DHCP_OFFER, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        transaction_destroy(&t);
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

        ASSERT_EQ(0, transaction_add(t, msg1));
        ASSERT_EQ(0, transaction_add(t, msg2));

        dhcp_message_t *check_ptr = transaction_get_index(t, 5);
        dhcp_message_t *value_ptr = transaction_search_for_last(t, DHCP_ACK);
        ASSERT_EQ(DHCP_ACK, check_ptr->type);
        ASSERT_EQ(DHCP_ACK, value_ptr->type);
        ASSERT_EQ(check_ptr, value_ptr);

        transaction_destroy(&t);
        PASS();
}

SUITE(transaction) 
{
        RUN_TEST(test_transaction_new_and_destroy);
        RUN_TEST(test_transaction_add);
        RUN_TEST(test_transaction_add_mismatched_xid);
        RUN_TEST(test_transaction_get_index);
        RUN_TEST(test_tramsaction_search_for);
        RUN_TEST(test_tramsaction_search_for_last);
}


#include "greatest.h"
#include "tests.h"
#include <stdint.h>
#include <timer.h>
#include <stdlib.h>
#include <unistd.h>

TEST test_timer_new_and_destroy() 
{
        struct timer *t = timer_new(TIMER_ONCE, 10, false, NULL);
        ASSERT_NEQ(NULL, t);
        timer_destroy(&t);
        ASSERT_EQ(NULL, t);

        PASS();
}

TEST test_timer_update()
{
        struct timer *t = timer_new(TIMER_ONCE, 10, true, NULL);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(true, t->is_running);

        for (int i = 0; i < 3; i++) {
                sleep(1);
                ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));
        }

        ASSERT_IN_RANGE(6, t->rtime, 8);

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_timer_start()
{
        struct timer *t = timer_new(TIMER_ONCE, 10, false, NULL);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(false, t->is_running);
        ASSERT_EQ(10, t->rtime);

        timer_start(t);
        ASSERT_EQ(true, t->is_running);

        ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));
        sleep(2);
        ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));

        ASSERT_IN_RANGE(7, t->rtime, 9);

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_timer_stop() 
{
        struct timer *t = timer_new(TIMER_ONCE, 10, false, NULL);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(false, t->is_running);
        ASSERT_EQ(10, t->rtime);

        timer_start(t);
        ASSERT_EQ(true, t->is_running);

        ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));
        sleep(2);
        ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));
        timer_stop(t);
        sleep(2);
        ASSERT_EQ(TIMER_STOPPED, timer_update(t, NULL));

        ASSERT_EQ(false, t->is_running);
        ASSERT_IN_RANGE(7, t->rtime, 9);

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_timer_reset() 
{
        struct timer *t = timer_new(TIMER_ONCE, 10, false, NULL);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(false, t->is_running);
        ASSERT_EQ(10, t->rtime);

        timer_start(t);
        ASSERT_EQ(true, t->is_running);

        ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));
        sleep(2);
        ASSERT_EQ(TIMER_NOCB, timer_update(t, NULL));

        timer_reset(t);
        ASSERT_EQ(10, t->rtime);

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

static int callback(uint32_t time, void *data) 
{
        int value = *(int*)data;

        ASSERT_EQ(5, value);

        return 100;
}

TEST test_timer_expire_call_cb_once()
{
        struct timer *t = timer_new(TIMER_ONCE, 5, true, callback);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(true, t->is_running);

        int data = 5;

        for (int i = 0; i < 3; i++) {
                sleep(1);
                ASSERT_EQ(TIMER_NOCB, timer_update(t, &data));
        }

        ASSERT_IN_RANGE(1, t->rtime, 3);
        sleep(3);

        ASSERT_EQ(100, timer_update(t, &data));

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_timer_expire_call_cb_repeat()
{
        struct timer *t = timer_new(TIMER_REPEAT, 2, true, callback);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(true, t->is_running);

        int data = 5;

        for (int i = 0; i < 3; i++) {
                sleep(2);
                ASSERT_EQ(100, timer_update(t, &data));
        }

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

TEST test_timer_expire_call_cb_null()
{
        struct timer *t = timer_new(TIMER_ONCE, 5, true, NULL);
        ASSERT_NEQ(NULL, t);
        
        ASSERT_EQ(true, t->is_running);

        int data = 5;

        for (int i = 0; i < 3; i++) {
                sleep(1);
                ASSERT_EQ(TIMER_NOCB, timer_update(t, &data));
        }

        ASSERT_IN_RANGE(1, t->rtime, 3);
        sleep(3);

        ASSERT_EQ(TIMER_ERROR, timer_update(t, &data));

        timer_destroy(&t);
        ASSERT_EQ(NULL, t);
        PASS();
}

SUITE(timer) {
        RUN_TEST(test_timer_new_and_destroy);
        RUN_TEST(test_timer_start);
        RUN_TEST(test_timer_stop);
        RUN_TEST(test_timer_reset);
        RUN_TEST(test_timer_update);
        RUN_TEST(test_timer_expire_call_cb_once);
        RUN_TEST(test_timer_expire_call_cb_repeat);
        RUN_TEST(test_timer_expire_call_cb_null);
}


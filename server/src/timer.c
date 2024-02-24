#include "timer.h"
#include "cclog.h"
#include "logging.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct timer *timer_new(enum timer_type type, uint32_t dtime, bool is_running, timer_cb_t cb)
{
        if_null_log_ng(cb, LOG_WARN, NULL, "Initialising a timer without callback. "
                        "This is valid operation, make sure it is also a desired one");

        struct timer *t = malloc( sizeof(struct timer));       
        if_null_log(t, error, LOG_ERROR, NULL, "Failed to allocate space for timer");

        t->type = type;
        t->dtime = dtime;
        t->rtime = dtime;
        t->is_running = is_running;
        t->_ltime = time(NULL);
        t->cb = cb;

        return t;
error:
        return NULL;
}

int timer_update(struct timer *t, void *priv)
{
        if (!t)
                return TIMER_ERROR;

        int rv = TIMER_STOPPED;
        if_false(t->is_running, exit);

        rv = TIMER_NOCB;

        /* Calculate time difference for the timer */
        uint32_t current_time = time(NULL);
        uint32_t time_difference = current_time - t->_ltime;
        /* If no time difference, no need to update */
        if_false(time_difference, exit);

        t->_ltime = current_time;

        /* If the remaining time is less or equal to the time difference, call the callback */
        if (t->rtime <= time_difference) {
                t->rtime = t->dtime;
                t->is_running = (t->type == TIMER_REPEAT) ? true : false;

                /* Handle if callback is null */
                rv = TIMER_ERROR;
                if_null_log(t->cb, exit, LOG_WARN, NULL, "Timer expired but callback is null!");

                rv = t->cb(current_time, priv);
        } else {
                /* Update the remaining time */
                t->rtime -= time_difference;
                rv = TIMER_NOCB;
        }

exit:
        return rv;
}

int timer_start(struct timer *t)
{
        if (!t)
                return -1;

        t->is_running = true;
        t->_ltime = time(NULL);
        return 0;
}

int timer_stop(struct timer *t)
{
        if (!t)
                return -1;

        t->is_running = false;
        return 0;
}

int timer_reset(struct timer *t)
{
        if (!t)
                return -1;

        t->is_running = true;
        t->_ltime = time(NULL);
        t->rtime = t->dtime;

        return 0;
 
}

void timer_destroy(struct timer **t)
{
        if (!t || !*t)
                return;

        free(*t);
        *t = NULL;
}


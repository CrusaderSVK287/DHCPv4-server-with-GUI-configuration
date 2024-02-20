#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define TIMER_NOCB INT_MIN
#define TIMER_ERROR (INT_MIN + 1)
#define TIMER_STOPPED (INT_MIN + 2)

enum timer_type {
    TIMER_ONCE,
    TIMER_REPEAT,
};

/* 
 * Callback function prototype. Example of function declaration: 
 * int callback_function(uint32_t call_time, void *priv);
 *
 * call_time is 
 */
typedef int (*timer_cb_t)(uint32_t call_time, void *priv);

/*
 * Struct representing a timer
 * Timer can have 2 types:
 * TIMER_ONCE - when timer hits 0, callback is called
 * TIMER_REPEAT - same as timer once, but keeps repeating
 *
 * dtime: default time, to how many seconds should rtime be set at the beggining of timer cycle
 * rtime: remaining time, time before timer reaches 0
 * is_running: flag indicating whether or not the timer is ticking. rtime is not 
 *      reset when timer is paused
 *
 *  _ltime: this variable should not be accesses from outside, its a last time at 
 *      which the timer was updated
 *
 *  timer_cb_t *cb: pointer to a callback function the timer will call once it 
 *      reaches zero.
 */ 
struct timer {
    enum timer_type type;
    uint32_t dtime;
    uint32_t rtime;
    bool is_running;

    uint32_t _ltime;

    timer_cb_t cb;
};

/* Allocates new timer */
struct timer *timer_new(enum timer_type type, uint32_t dtime, bool is_running, timer_cb_t cb);

/* 
 * Function will update the timer according to the time it has stored in its 
 * _ltime variable. If the rtime will be <= 0 after update, the timer cb will be 
 * called. If the timer is of type TIMER_REPEAT, it will set rtime back to dtime
 * and keep is_running on true. If its TIMER_ONCE, it will NOT set rtime to dtime 
 * and it will set is_running on false. To start the timer again, use timer_start().
 *
 * This function should be called at least once per second on each timer 
 * for the most accurate timings.
 *
 * returns whatever the callback function returns. If no callback was called, 
 * returns TIMER_NOCB
 */
int timer_update(struct timer *t, void *priv);

/* Starts the timer from its current state */
int timer_start(struct timer *t);

/* Stops timer updates in its current state */
int timer_stop(struct timer *t);

/*
 * Resets timers remaining time to default and starts the timer regardless 
 * of its current state 
 */
int timer_reset(struct timer *t);

/* Destroys the allocated timer structure */
void timer_destroy(struct timer **t);

#endif // !__TIMER_H__


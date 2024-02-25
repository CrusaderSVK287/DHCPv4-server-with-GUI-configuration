#ifndef __TIMER_ARGS_H__
#define __TIMER_ARGS_H__

#include "dhcp_server.h"

/*
 * This header files purpose is to define structs for timer_update functions 
 * where the value "priv" has to have more than one argument, which it cannot.
 * As an example, transaction update requires dhcp server for address pools and 
 * index of transaction
 */

typedef struct trans_update_args {
    dhcp_server_t *server;
    int index;
} trans_update_args_t;

#endif // !__TIMER_ARGS_H__

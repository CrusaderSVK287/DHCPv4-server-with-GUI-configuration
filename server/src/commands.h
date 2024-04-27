#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "dhcp_server.h"
#include <cJSON.h>

typedef char* (*command_func)(cJSON *params, dhcp_server_t *server);

typedef struct command{
    const char *name;
    command_func func;
} command_t;

/*
 * TO CREATE A COMMAND:
 *  1. declare it here, it must be of type command_func
 *  2. define it in commands.c 
 *  3. add it to command list in init.c
 *  Commands have to return a json string. Array of strings where each 
 *  one is going to be printed on separate line in the tui
 */
char *command_echo(cJSON *params, dhcp_server_t *server);

#endif // !__COMMANDS_H__


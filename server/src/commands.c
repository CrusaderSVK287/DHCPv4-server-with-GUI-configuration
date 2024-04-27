#include "commands.h"
#include <cJSON.h>
#include <string.h>

char *command_echo(cJSON *params, dhcp_server_t *server)
{
        return cJSON_PrintUnformatted(params);
}


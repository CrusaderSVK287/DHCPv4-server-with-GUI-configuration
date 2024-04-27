#include "commands.h"
#include <cJSON.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "security/dhcp_snooping/dhcp_snoop.h"
#include "utils/llist.h"
#include "utils/xtoy.h"

char *command_echo(cJSON *params, dhcp_server_t *server)
{
        return cJSON_PrintUnformatted(params);
}

char *command_stop(cJSON *params, dhcp_server_t *server)
{
        kill(getpid(), SIGINT);
        return strdup("[\"Stopping server\"]");
}

char *command_rogue_scan(cJSON *params, dhcp_server_t *server)
{
#ifndef  CONFIG_SECURITY_ENABLE_DHCP_SNOOPING
        return strdup("[\"DHCP snooping is not supported. Please read the manual.\"]");
#else 
        char *res = NULL;
        llist_t *ll = llist_new();

        int i = 0;
        cJSON *e;
        cJSON *mac;
        cJSON_ArrayForEach(e, params) {
                if (i == 0) {
                        mac = e;
                } else {
                        uint32_t *mac_entry = malloc(1 * sizeof(uint32_t));
                        if (mac_entry) {
                                *mac_entry = ipv4_address_to_uint32(cJSON_GetStringValue(e));
                                llist_append(ll, mac_entry, true);
                        }
                }

                i++;
        }

        if (dhcp_snooper_perform_scan(server, cJSON_GetStringValue(mac), ll, &res) < 0)
                goto error;

        cJSON *json = cJSON_CreateArray();
        char *res_start = res;
        char *token;
        token = strtok(res, ",");
        while (token != NULL) {
                cJSON_AddItemToArray(json, cJSON_CreateString(token));
                token = strtok(NULL, ",");
        }
        free(res_start);

        return cJSON_PrintUnformatted(json);
error:
        return strdup("[\"Error\"]");
#endif
}


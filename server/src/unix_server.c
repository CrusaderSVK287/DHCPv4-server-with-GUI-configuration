#include "unix_server.h"
#include "commands.h"
#include "dhcp_server.h"
#include "logging.h"
#include "utils/llist.h"
#include <asm-generic/errno.h>
#include <cJSON.h>
#include <cclog_macros.h>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>

#define BACKLOG 10
#define SVR_WILL_KEEP_RUNNING "Server will keep running but will be unable to utilise IPC features."

int unix_server_init(unix_server_t *server)
{
        if (!server)
                return -1;

        server->fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if_failed_log_n(server->fd, exit, LOG_CRITICAL, NULL, 
                "Failed to create socket for UNIX server.");

        struct sockaddr_un saddr;
        saddr.sun_family = AF_UNIX;
        strncpy(saddr.sun_path, UNIX_SOCKET_PATH, sizeof(saddr.sun_path));
        /* Remove any previous sockets with the same path */
        unlink(UNIX_SOCKET_PATH);
        
        if_failed_log_n(bind(server->fd, (struct sockaddr *)&saddr, sizeof(saddr)), exit, 
                LOG_CRITICAL, NULL, "Failed to bind UNIX socket to address.");

        if_failed_log_n(listen(server->fd, BACKLOG), exit, LOG_CRITICAL, NULL, 
                "Failed to listen on UNIX socket");

        cclog(LOG_MSG, NULL, "Initialized UNIX server");
exit:
        return server->fd;
}

static char* handle_command(dhcp_server_t *_dhcp_server, cJSON *json)
{
        if (!_dhcp_server|| !json)
                goto error;
        
        dhcp_server_t *dhcp_server = (dhcp_server_t*)_dhcp_server;
        unix_server_t *server = &dhcp_server->unix_server;
        if_null(server, error);

        const char *command = cJSON_GetStringValue(cJSON_GetObjectItem(json, "command"));
        cJSON *params = cJSON_GetObjectItem(json, "parameters");
        if_null(command, error);
        if_null(params, error);
        
        command_t *cmd;
        llist_foreach(server->commands, {
                cmd = (command_t*)node->data;

                if (strcmp(cmd->name, command) == 0) {
                        /* Commands return formated JSON string */
                        return cmd->func(params, dhcp_server);
                }
        });

        return strdup("[\"ERROR: Unknown command\"]");
error:
        cclog(LOG_UNIX, NULL, "ERROR handling command");
        return NULL;
}

int unix_server_handle(void *_dhcp_server)
{
        if (!_dhcp_server)
                return -1;

        /* This workaround is needed since we cannot have recursive includes in header files */
        dhcp_server_t *dhcp_server = (dhcp_server_t*)_dhcp_server;
        unix_server_t *server = &dhcp_server->unix_server;

        int rv = -1;
        struct sockaddr_un caddr;
        socklen_t caddr_len = sizeof(caddr);
        int client_sock = accept(server->fd, (struct sockaddr *)&caddr, &caddr_len);

        if (client_sock == -1 && errno == EWOULDBLOCK) {
                goto exit_ok;
        } else if (client_sock == -1) {
                cclog(LOG_ERROR, NULL, "Failed to accept UNIX connection: %s", strerror(errno));
                goto exit;
        }

        /* Message received, handle it */

        rv = UNIX_STATUS_ERROR;
        char buffer[BUFSIZ];
        memset(buffer, 0, BUFSIZ);
        int bytes = recv(client_sock, buffer, BUFSIZ, 0);
        if_failed_log_n(bytes, exit, LOG_UNIX, NULL, "Failed to receive request");

        cclog(LOG_UNIX, NULL, "Received message: %s", buffer);
        cJSON *json = cJSON_Parse(buffer);
        if_null_log(json, error_respond, LOG_UNIX, NULL, "Bad request");

        char *response = handle_command(dhcp_server, json);
        cJSON_Delete(json);
        if_null_log(response, error_respond, LOG_UNIX, NULL, "ERROR: Failed to handle command");

        cclog(LOG_UNIX, NULL, "Sending response: %s", response);
        if_failed_log_n(send(client_sock, response, strlen(response), 0), exit, LOG_UNIX, NULL, 
                "Failed to send respnse");

        close(client_sock);
exit_ok:
        rv = UNIX_STATUS_OK;
exit:
        return rv;

/* 
 * Special case, since we need to notify client of an error. This usually means the error is on 
 * clients requrest rather than server.
 */
error_respond:
        if (client_sock >= 0)
                send(client_sock, "[\"ERROR\"]", bytes, 0);
        goto exit_ok;
}

void unix_server_clean(unix_server_t *server) 
{
        if (server && server->fd >= 0) {
                unlink(UNIX_SOCKET_PATH);
                if (server->commands) {
                        llist_destroy(&server->commands);
                }
                close(server->fd);
        }
}


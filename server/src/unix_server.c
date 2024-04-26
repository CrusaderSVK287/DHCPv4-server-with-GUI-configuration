#include "unix_server.h"
#include "logging.h"
#include <asm-generic/errno.h>
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

int unix_server_handle(unix_server_t *server)
{
        if (!server)
                return -1;

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
        /* TODO: temporaty solution, until command API is fully implemented */
        char buffer[BUFSIZ];
        memset(buffer, 0, BUFSIZ);
        int bytes = recv(client_sock, buffer, BUFSIZ, 0);
        if_failed_log_n(bytes, exit, LOG_UNIX, NULL, "Failed to receive request");

        cclog(LOG_UNIX, NULL, "Received message: %s", buffer);

        if_failed_log_n(send(client_sock, buffer, bytes, 0), exit, LOG_UNIX, NULL, 
                "Failed to send respnse");

        close(client_sock);
exit_ok:
        rv = UNIX_STATUS_OK;
exit:
        return rv;
}

void unix_server_clean(unix_server_t *server) 
{
        if (!server && server->fd >= 0) {
                unlink(UNIX_SOCKET_PATH);;
                close(server->fd);
        }
}


#include "dhcp_server.h"
#include "RFC/RFC-2131.h"
#include "cclog.h"
#include "cclog_macros.h"
#include "dhcp_packet.h"
#include "logging.h"
#include "messages/DHCPDECLINE.h"
#include "messages/DHCPDISCOVER.h"
#include "messages/DHCPINFORM.h"
#include "messages/DHCPRELEASE.h"
#include "messages/DHCPREQUEST.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define BACKLOG_SIZE 50

static volatile int server_keep_running = 1;
static void stop_running(int signo)
{
	server_keep_running = 0;
}

int init_dhcp_server(dhcp_server_t *server)
{
	int rv = -1;

	if_null_log(server, exit, LOG_INFO, NULL, "server parameter is null");

	server->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if_failed_log_n(server->sock_fd, exit, LOG_CRITICAL, NULL, 
		"Failed to create socket with return code %d", server->sock_fd);

	/* Setting socket option to reuse address */
	int reuse_addr = 1;
	if_failed_log(setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)), 
		exit, LOG_CRITICAL, NULL, "Failed to set reuse address socket option");

	/* Setting up non-blocking socket */
	int flags = fcntl(server->sock_fd, F_GETFL, 0);
	if_failed_log_n(flags, exit, LOG_CRITICAL, NULL, "Failed to obtain socket fd flags");

	if_failed_log(fcntl(server->sock_fd, F_SETFL, flags | O_NONBLOCK), 
		exit, LOG_CRITICAL, NULL, "Failed to set socket to non-blocking state");

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	/* TODO: Make it possible to specify by user eighter address or interface which to use */
	//addr.sin_addr.s_addr = inet_addr("192.168.1.100");
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(67);

	if_failed_log(bind(server->sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)),
		exit, LOG_CRITICAL, NULL, "Failed to bind socket to address");

	cclog(LOG_MSG, NULL, "Server socket successfully initialised");

	/* Setting up sigint signal for proper shutdown */
	if (signal(SIGINT, stop_running) == SIG_ERR) { 
		cclog(LOG_CRITICAL, NULL, "Failed to set signal handler");
		goto exit;
	}
	cclog(LOG_MSG, NULL, "Signal handler set successfully");

	rv = 0;
	
exit:
	return rv;
}

int uninit_dhcp_server(dhcp_server_t *server)
{
	int rv = -1;

	if_null_log(server, exit, LOG_INFO, NULL, "server parameter is null");

	if_failed_log(close(server->sock_fd), exit, LOG_ERROR, NULL, "Failed to close socket");

	cclog(LOG_MSG, NULL, "Server stoped successfully");
	rv = 0;
exit:
	return rv;
}

int dhcp_server_serve(dhcp_server_t *server)
{
	int rv = -1;

	if_null_log(server, exit, LOG_CRITICAL, NULL, "server parameter is null");

        dhcp_message_t *dhcp_msg = dhcp_message_new();
        if_null_log(dhcp_msg, exit, LOG_ERROR, NULL, "Cannot allocate memory for dhcp_message_t");

	do
	{
		rv = recv(server->sock_fd, &dhcp_msg->packet, sizeof(dhcp_packet_t), 0);
		if (rv < 0 && errno == EAGAIN) {
			continue;
		} else if (rv < 0) {
			cclog(LOG_WARN, NULL, "Failed to receive dhcp packet with return code %d", rv);
			continue;
		}

                /* Parse the packet, errors in packet parsing are handled in the parse function */
                rv = dhcp_packet_parse(dhcp_msg);
                if (rv < 0)
                        continue;

                switch (dhcp_msg->type) {
                        case DHCP_DISCOVER: message_DHCPDISCOVER_handle(server, dhcp_msg); break;
                        case DHCP_REQUEST:  message_DHCPREQUEST_handle(server, dhcp_msg);  break;
                        case DHCP_DECLINE:  message_DHCPDECLINE_handle(server, dhcp_msg);  break;
                        case DHCP_INFORM:   message_DHCPINFORM_handle(server, dhcp_msg);   break;
                        case DHCP_RELEASE:  message_DHCPRELEASE_handle(server, dhcp_msg);  break;

                        case DHCP_OFFER:
                        case DHCP_ACK:
                        case DHCP_NAK:
                                cclog(LOG_INFO, NULL, "Received message of type %d, "
                                        "server cannot handle, dropping", dhcp_msg->type);
                                break;

                        default:
                                cclog(LOG_WARN, NULL, "Invalid DHCP message type received (%d), "
                                                "dropping message", dhcp_msg->type);
                                break;
                }

	} while (server_keep_running);

	rv = 0;
exit:
	return rv;
}

